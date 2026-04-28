#include "control.h"

#include <string.h>

static thermowork_control_config_t g_config;
static thermowork_process_values_t g_values;

static int32_t clamp_i32(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static uint16_t clamp_u16(uint16_t value, uint16_t max_value)
{
    return value > max_value ? max_value : value;
}

void thermowork_control_init(void)
{
    memset(&g_config, 0, sizeof(g_config));
    memset(&g_values, 0, sizeof(g_values));

    g_config.mode = THERMOWORK_CONTROL_MODE_DISABLED;
    g_config.max_power_w = 3500;
    g_config.min_power_w = 0;
    g_config.manual_power_w = 0;
    g_config.target_grid_power_w = 0;
    g_config.burst_target_permille = 0;
    g_config.temperature_limit_c = 85.0f;
    g_config.uart_timeout_ms = 3000;
    g_config.burst_window_ms = 1000;
    g_config.ssr_gpio_pin = 17;
    g_config.output_active_high = true;

    g_values.enable = false;
    g_values.temp_valid = false;
    g_values.rx_age_ms = UINT32_MAX;
}

void thermowork_control_set_config(const thermowork_control_config_t *config)
{
    if (config == NULL) {
        return;
    }

    thermowork_control_config_t next = *config;

    if (next.max_power_w < 0) {
        next.max_power_w = 0;
    }
    if (next.min_power_w < 0) {
        next.min_power_w = 0;
    }
    if (next.min_power_w > next.max_power_w) {
        next.min_power_w = next.max_power_w;
    }
    if (next.manual_power_w < 0) {
        next.manual_power_w = 0;
    }
    if (next.manual_power_w > next.max_power_w) {
        next.manual_power_w = next.max_power_w;
    }
    next.burst_target_permille = clamp_u16(next.burst_target_permille, 1000);
    if (next.temperature_limit_c < 30.0f) {
        next.temperature_limit_c = 30.0f;
    }
    if (next.temperature_limit_c > 120.0f) {
        next.temperature_limit_c = 120.0f;
    }
    if (next.uart_timeout_ms < 250) {
        next.uart_timeout_ms = 250;
    }
    if (next.burst_window_ms < 20) {
        next.burst_window_ms = 20;
    }
    if (next.burst_window_ms > 60000) {
        next.burst_window_ms = 60000;
    }

    g_config = next;
}

void thermowork_control_update_process_values(const thermowork_process_values_t *values)
{
    if (values == NULL) {
        return;
    }

    g_values = *values;
}

void thermowork_control_step(thermowork_control_output_t *output)
{
    if (output == NULL) {
        return;
    }

    output->power_setpoint_w = 0;
    output->duty_permille = 0;
    output->outputs_enabled = false;

    if (!g_values.enable || g_values.emergency_stop) {
        return;
    }

    if (g_values.rx_age_ms > g_config.uart_timeout_ms) {
        return;
    }

    if (!g_values.temp_valid) {
        return;
    }

    if (g_values.tank_top_c >= g_config.temperature_limit_c ||
        g_values.tank_mid_c >= g_config.temperature_limit_c ||
        g_values.tank_bottom_c >= g_config.temperature_limit_c ||
        g_values.flow_line_c >= g_config.temperature_limit_c ||
        g_values.return_line_c >= g_config.temperature_limit_c) {
        return;
    }

    int32_t requested_power_w = 0;
    uint16_t duty_permille = 0;

    switch (g_config.mode) {
    case THERMOWORK_CONTROL_MODE_PV_SURPLUS:
        // Convention: grid_power_w > 0 means import, grid_power_w < 0 means export/surplus.
        requested_power_w = -(g_values.grid_power_w - g_config.target_grid_power_w);
        requested_power_w = clamp_i32(requested_power_w, g_config.min_power_w, g_config.max_power_w);
        if (g_config.max_power_w > 0) {
            duty_permille = (uint16_t)((requested_power_w * 1000) / g_config.max_power_w);
        }
        break;

    case THERMOWORK_CONTROL_MODE_MANUAL_POWER:
        requested_power_w = clamp_i32(g_config.manual_power_w, g_config.min_power_w, g_config.max_power_w);
        if (g_config.max_power_w > 0) {
            duty_permille = (uint16_t)((requested_power_w * 1000) / g_config.max_power_w);
        }
        break;

    case THERMOWORK_CONTROL_MODE_BURST_PERCENT:
        duty_permille = clamp_u16(g_config.burst_target_permille, 1000);
        requested_power_w = (int32_t)((g_config.max_power_w * (int32_t)duty_permille) / 1000);
        requested_power_w = clamp_i32(requested_power_w, g_config.min_power_w, g_config.max_power_w);
        break;

    case THERMOWORK_CONTROL_MODE_TEST:
        requested_power_w = g_config.max_power_w / 10;
        if (g_config.max_power_w > 0) {
            duty_permille = 100;
        }
        break;

    case THERMOWORK_CONTROL_MODE_DISABLED:
    default:
        return;
    }

    if (requested_power_w <= 0 || duty_permille == 0 || g_config.max_power_w <= 0) {
        return;
    }

    output->power_setpoint_w = requested_power_w;
    output->duty_permille = clamp_u16(duty_permille, 1000);
    output->outputs_enabled = true;
}

const thermowork_control_config_t *thermowork_control_get_config(void)
{
    return &g_config;
}

const thermowork_process_values_t *thermowork_control_get_values(void)
{
    return &g_values;
}
