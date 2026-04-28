#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "control.h"
#include "output_driver.h"
#include "safety.h"
#include "uart_protocol.h"

#define THERMOWORK_UART_PORT UART_NUM_0
#define THERMOWORK_UART_BAUD_RATE 115200
#define THERMOWORK_UART_RX_BUFFER_SIZE 2048
#define THERMOWORK_LINE_BUFFER_SIZE 768
#define THERMOWORK_STATUS_BUFFER_SIZE 512
#define THERMOWORK_CONTROL_PERIOD_MS 20
#define THERMOWORK_STATUS_PERIOD_MS 500
#define THERMOWORK_FULL_WAVE_MS_50HZ 20

static const char *TAG = "ThermoWerk";

static uint64_t g_last_rx_us = 0;
static uint64_t g_last_status_us = 0;
static thermowork_process_values_t g_last_values;

static uint32_t age_ms_since(uint64_t timestamp_us)
{
    if (timestamp_us == 0) {
        return UINT32_MAX;
    }

    uint64_t now_us = (uint64_t)esp_timer_get_time();
    if (now_us <= timestamp_us) {
        return 0;
    }

    uint64_t diff_ms = (now_us - timestamp_us) / 1000ULL;
    if (diff_ms > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (uint32_t)diff_ms;
}

static bool period_elapsed(uint64_t *last_us, uint32_t period_ms)
{
    uint64_t now_us = (uint64_t)esp_timer_get_time();
    if (*last_us == 0 || ((now_us - *last_us) / 1000ULL) >= period_ms) {
        *last_us = now_us;
        return true;
    }
    return false;
}

static void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = THERMOWORK_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        THERMOWORK_UART_PORT,
        THERMOWORK_UART_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0
    ));
    ESP_ERROR_CHECK(uart_param_config(THERMOWORK_UART_PORT, &uart_config));
}

static void output_driver_sync_from_config(const thermowork_control_config_t *config)
{
    thermowork_output_config_t output_config = {
        .ssr_gpio_pin = config->ssr_gpio_pin,
        .output_active_high = config->output_active_high,
        .burst_window_ms = config->burst_window_ms,
        .full_wave_ms = THERMOWORK_FULL_WAVE_MS_50HZ,
    };
    thermowork_output_driver_update_config(&output_config);
}

static void handle_uart_line(const char *line)
{
    thermowork_uart_message_t message;
    thermowork_uart_msg_type_t type = thermowork_uart_parse_line(line, &message);

    switch (type) {
    case THERMOWORK_UART_MSG_PROCESS_VALUES:
        g_last_values = message.process_values;
        g_last_rx_us = (uint64_t)esp_timer_get_time();
        thermowork_control_update_process_values(&g_last_values);
        ESP_LOGI(TAG, "Inputs: grid=%ld W pv=%ld W enable=%d temp_valid=%d",
                 (long)g_last_values.grid_power_w,
                 (long)g_last_values.pv_power_w,
                 g_last_values.enable,
                 g_last_values.temp_valid);
        break;

    case THERMOWORK_UART_MSG_CONFIG:
        thermowork_control_set_config(&message.config);
        output_driver_sync_from_config(thermowork_control_get_config());
        ESP_LOGI(TAG, "Config: max=%ld W mode=%d gpio=%d window=%lu ms",
                 (long)message.config.max_power_w,
                 (int)message.config.mode,
                 message.config.ssr_gpio_pin,
                 (unsigned long)message.config.burst_window_ms);
        break;

    case THERMOWORK_UART_MSG_COMMAND:
        if (message.command_emergency_stop) {
            g_last_values.emergency_stop = true;
            g_last_values.enable = false;
            thermowork_output_driver_force_off();
            thermowork_control_update_process_values(&g_last_values);
            ESP_LOGW(TAG, "Emergency stop command received");
        }
        if (message.command_reset_fault) {
            g_last_values.emergency_stop = false;
            thermowork_control_update_process_values(&g_last_values);
            ESP_LOGI(TAG, "Fault reset command received");
        }
        if (message.command_enable_outputs) {
            g_last_values.enable = true;
            thermowork_control_update_process_values(&g_last_values);
            ESP_LOGI(TAG, "Enable outputs command received");
        }
        break;

    case THERMOWORK_UART_MSG_INVALID:
    case THERMOWORK_UART_MSG_NONE:
    default:
        ESP_LOGW(TAG, "Invalid UART frame: %s", line);
        break;
    }
}

static void uart_rx_task(void *arg)
{
    (void)arg;

    uint8_t byte = 0;
    char line_buffer[THERMOWORK_LINE_BUFFER_SIZE];
    size_t line_len = 0;

    while (true) {
        int len = uart_read_bytes(
            THERMOWORK_UART_PORT,
            &byte,
            1,
            pdMS_TO_TICKS(20)
        );

        if (len <= 0) {
            continue;
        }

        if (byte == '\n' || byte == '\r') {
            if (line_len > 0) {
                line_buffer[line_len] = '\0';
                handle_uart_line(line_buffer);
                line_len = 0;
            }
            continue;
        }

        if (line_len < (sizeof(line_buffer) - 1)) {
            line_buffer[line_len++] = (char)byte;
        } else {
            line_len = 0;
            ESP_LOGW(TAG, "UART line buffer overflow, dropping frame");
        }
    }
}

static void control_task(void *arg)
{
    (void)arg;

    char status_buffer[THERMOWORK_STATUS_BUFFER_SIZE];

    while (true) {
        g_last_values.rx_age_ms = age_ms_since(g_last_rx_us);
        thermowork_control_update_process_values(&g_last_values);

        const thermowork_control_config_t *config = thermowork_control_get_config();
        const thermowork_process_values_t *values = thermowork_control_get_values();
        thermowork_safety_status_t safety = thermowork_safety_evaluate(config, values);

        thermowork_control_output_t output;
        thermowork_control_step(&output);

        if (!safety.outputs_allowed) {
            output.power_setpoint_w = 0;
            output.duty_permille = 0;
            output.outputs_enabled = false;
        }

        thermowork_output_driver_apply(&output);
        thermowork_output_state_t output_state = thermowork_output_driver_get_state();

        if (period_elapsed(&g_last_status_us, THERMOWORK_STATUS_PERIOD_MS)) {
            int written = thermowork_uart_format_status(
                status_buffer,
                sizeof(status_buffer),
                &output,
                config,
                values,
                thermowork_fault_to_string(safety.fault),
                output_state.output_level
            );

            if (written > 0) {
                uart_write_bytes(THERMOWORK_UART_PORT, status_buffer, strlen(status_buffer));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(THERMOWORK_CONTROL_PERIOD_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ThermoWerk3p ESP32-S3 firmware starting");

    thermowork_control_init();
    thermowork_safety_init();
    thermowork_uart_protocol_init();
    uart_init();

    memset(&g_last_values, 0, sizeof(g_last_values));
    g_last_values.enable = false;
    g_last_values.temp_valid = false;
    g_last_values.rx_age_ms = UINT32_MAX;
    g_last_values.tank_top_c = 20.0f;
    g_last_values.tank_mid_c = 20.0f;
    g_last_values.tank_bottom_c = 20.0f;
    g_last_values.flow_line_c = 20.0f;
    g_last_values.return_line_c = 20.0f;
    g_last_values.ambient_c = 20.0f;

    thermowork_control_update_process_values(&g_last_values);

    thermowork_output_config_t output_config = {
        .ssr_gpio_pin = thermowork_control_get_config()->ssr_gpio_pin,
        .output_active_high = thermowork_control_get_config()->output_active_high,
        .burst_window_ms = thermowork_control_get_config()->burst_window_ms,
        .full_wave_ms = THERMOWORK_FULL_WAVE_MS_50HZ,
    };
    thermowork_output_driver_init(&output_config);

    xTaskCreate(uart_rx_task, "thermowork_uart_rx", 4096, NULL, 10, NULL);
    xTaskCreate(control_task, "thermowork_control", 4096, NULL, 8, NULL);
}
