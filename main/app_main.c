#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "control.h"
#include "safety.h"
#include "uart_protocol.h"

#define THERMOWORK_UART_PORT UART_NUM_0
#define THERMOWORK_UART_BAUD_RATE 115200
#define THERMOWORK_UART_RX_BUFFER_SIZE 1024
#define THERMOWORK_LINE_BUFFER_SIZE 512
#define THERMOWORK_STATUS_BUFFER_SIZE 256
#define THERMOWORK_CONTROL_PERIOD_MS 200

static const char *TAG = "ThermoWerk";

static uint64_t g_last_rx_us = 0;
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

static void handle_uart_line(const char *line)
{
    thermowork_uart_message_t message;
    thermowork_uart_msg_type_t type = thermowork_uart_parse_line(line, &message);

    switch (type) {
    case THERMOWORK_UART_MSG_PROCESS_VALUES:
        g_last_values = message.process_values;
        g_last_rx_us = (uint64_t)esp_timer_get_time();
        thermowork_control_update_process_values(&g_last_values);
        ESP_LOGI(TAG, "Process values updated: grid=%ld W pv=%ld W enable=%d",
                 (long)g_last_values.grid_power_w,
                 (long)g_last_values.pv_power_w,
                 g_last_values.enable);
        break;

    case THERMOWORK_UART_MSG_CONFIG:
        thermowork_control_set_config(&message.config);
        ESP_LOGI(TAG, "Config updated: max=%ld W mode=%d",
                 (long)message.config.max_power_w,
                 (int)message.config.mode);
        break;

    case THERMOWORK_UART_MSG_COMMAND:
        ESP_LOGI(TAG, "Command received: enable_outputs=%d", message.command_enable_outputs);
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
            pdMS_TO_TICKS(50)
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

        int written = thermowork_uart_format_status(
            status_buffer,
            sizeof(status_buffer),
            &output,
            thermowork_fault_to_string(safety.fault)
        );

        if (written > 0) {
            uart_write_bytes(THERMOWORK_UART_PORT, status_buffer, strlen(status_buffer));
        }

        // TODO: connect output.duty_permille to burst-fire/full-wave SSR scheduler.
        // For safety, no GPIO output is driven in this first skeleton.

        vTaskDelay(pdMS_TO_TICKS(THERMOWORK_CONTROL_PERIOD_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ThermoWerkFirmwareMCU starting");

    thermowork_control_init();
    thermowork_safety_init();
    thermowork_uart_protocol_init();
    uart_init();

    memset(&g_last_values, 0, sizeof(g_last_values));
    g_last_values.enable = false;
    g_last_values.rx_age_ms = UINT32_MAX;

    xTaskCreate(uart_rx_task, "thermowork_uart_rx", 4096, NULL, 10, NULL);
    xTaskCreate(control_task, "thermowork_control", 4096, NULL, 8, NULL);
}
