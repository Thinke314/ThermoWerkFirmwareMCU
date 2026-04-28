#include "output_driver.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "tw_output";

static thermowork_output_config_t g_config;
static thermowork_output_state_t g_state;

static bool is_valid_gpio(int gpio)
{
    return gpio >= 0 && gpio < GPIO_NUM_MAX;
}

static void write_output(bool on)
{
    if (!is_valid_gpio(g_config.ssr_gpio_pin)) {
        return;
    }

    bool electrical_level = g_config.output_active_high ? on : !on;
    gpio_set_level((gpio_num_t)g_config.ssr_gpio_pin, electrical_level ? 1 : 0);
    g_state.output_level = on;
}

static bool scheduler_decide_on(uint16_t duty_permille)
{
    if (duty_permille == 0) {
        return false;
    }
    if (duty_permille >= 1000) {
        return true;
    }

    uint32_t full_wave_ms = g_config.full_wave_ms > 0 ? g_config.full_wave_ms : 20;
    uint32_t window_ms = g_config.burst_window_ms >= full_wave_ms ? g_config.burst_window_ms : full_wave_ms;
    uint32_t waves_per_window = window_ms / full_wave_ms;
    if (waves_per_window == 0) {
        waves_per_window = 1;
    }

    uint32_t on_waves = ((uint32_t)duty_permille * waves_per_window + 500U) / 1000U;
    if (on_waves == 0) {
        return false;
    }
    if (on_waves >= waves_per_window) {
        return true;
    }

    uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000ULL;
    uint32_t elapsed_ms = (uint32_t)(now_ms % window_ms);
    uint32_t wave_index = elapsed_ms / full_wave_ms;
    if (wave_index >= waves_per_window) {
        wave_index = waves_per_window - 1;
    }

    // Bresenham-style distribution from the former ThermoWerk3p prototype.
    uint32_t prev = (wave_index * on_waves) / waves_per_window;
    uint32_t next = ((wave_index + 1U) * on_waves) / waves_per_window;
    return next > prev;
}

void thermowork_output_driver_init(const thermowork_output_config_t *config)
{
    memset(&g_config, 0, sizeof(g_config));
    memset(&g_state, 0, sizeof(g_state));

    g_config.ssr_gpio_pin = 17;
    g_config.output_active_high = true;
    g_config.burst_window_ms = 1000;
    g_config.full_wave_ms = 20;

    if (config != NULL) {
        thermowork_output_driver_update_config(config);
    }

    if (!is_valid_gpio(g_config.ssr_gpio_pin)) {
        ESP_LOGE(TAG, "Invalid SSR GPIO: %d", g_config.ssr_gpio_pin);
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << g_config.ssr_gpio_pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    write_output(false);
    ESP_LOGI(TAG, "SSR output initialized on GPIO%d", g_config.ssr_gpio_pin);
}

void thermowork_output_driver_update_config(const thermowork_output_config_t *config)
{
    if (config == NULL) {
        return;
    }

    bool gpio_changed = g_config.ssr_gpio_pin != config->ssr_gpio_pin;

    g_config = *config;
    if (g_config.burst_window_ms < 20) {
        g_config.burst_window_ms = 20;
    }
    if (g_config.full_wave_ms < 10) {
        g_config.full_wave_ms = 20;
    }

    g_state.burst_window_ms = g_config.burst_window_ms;
    g_state.ssr_gpio_pin = g_config.ssr_gpio_pin;

    if (gpio_changed && is_valid_gpio(g_config.ssr_gpio_pin)) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << g_config.ssr_gpio_pin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        write_output(false);
        ESP_LOGI(TAG, "SSR output moved to GPIO%d", g_config.ssr_gpio_pin);
    }
}

void thermowork_output_driver_apply(const thermowork_control_output_t *output)
{
    if (output == NULL || !output->outputs_enabled) {
        thermowork_output_driver_force_off();
        return;
    }

    g_state.scheduler_enabled = true;
    g_state.duty_permille = output->duty_permille;
    write_output(scheduler_decide_on(output->duty_permille));
}

thermowork_output_state_t thermowork_output_driver_get_state(void)
{
    return g_state;
}

void thermowork_output_driver_force_off(void)
{
    g_state.scheduler_enabled = false;
    g_state.duty_permille = 0;
    write_output(false);
}
