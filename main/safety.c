#include "safety.h"

void thermowork_safety_init(void)
{
}

static bool temp_over_limit(float value, float limit)
{
    return value >= limit;
}

thermowork_safety_status_t thermowork_safety_evaluate(
    const thermowork_control_config_t *config,
    const thermowork_process_values_t *values
)
{
    thermowork_safety_status_t status = {
        .fault = THERMOWORK_FAULT_INTERNAL,
        .outputs_allowed = false,
    };

    if (config == NULL || values == NULL) {
        return status;
    }

    if (config->max_power_w < 0 ||
        config->min_power_w < 0 ||
        config->min_power_w > config->max_power_w ||
        config->burst_target_permille > 1000 ||
        config->burst_window_ms < 20 ||
        config->ssr_gpio_pin < 0) {
        status.fault = THERMOWORK_FAULT_INVALID_CONFIG;
        return status;
    }

    if (values->emergency_stop) {
        status.fault = THERMOWORK_FAULT_EMERGENCY_STOP;
        return status;
    }

    if (!values->enable || config->mode == THERMOWORK_CONTROL_MODE_DISABLED) {
        status.fault = THERMOWORK_FAULT_DISABLED;
        return status;
    }

    if (values->rx_age_ms > config->uart_timeout_ms) {
        status.fault = THERMOWORK_FAULT_UART_TIMEOUT;
        return status;
    }

    if (!values->temp_valid) {
        status.fault = THERMOWORK_FAULT_SENSOR_INVALID;
        return status;
    }

    if (temp_over_limit(values->tank_top_c, config->temperature_limit_c) ||
        temp_over_limit(values->tank_mid_c, config->temperature_limit_c) ||
        temp_over_limit(values->tank_bottom_c, config->temperature_limit_c) ||
        temp_over_limit(values->flow_line_c, config->temperature_limit_c) ||
        temp_over_limit(values->return_line_c, config->temperature_limit_c)) {
        status.fault = THERMOWORK_FAULT_OVERTEMPERATURE;
        return status;
    }

    status.fault = THERMOWORK_FAULT_NONE;
    status.outputs_allowed = true;
    return status;
}

const char *thermowork_fault_to_string(thermowork_fault_t fault)
{
    switch (fault) {
    case THERMOWORK_FAULT_NONE:
        return "none";
    case THERMOWORK_FAULT_DISABLED:
        return "disabled";
    case THERMOWORK_FAULT_EMERGENCY_STOP:
        return "emergency_stop";
    case THERMOWORK_FAULT_UART_TIMEOUT:
        return "uart_timeout";
    case THERMOWORK_FAULT_SENSOR_INVALID:
        return "sensor_invalid";
    case THERMOWORK_FAULT_OVERTEMPERATURE:
        return "overtemperature";
    case THERMOWORK_FAULT_INVALID_CONFIG:
        return "invalid_config";
    case THERMOWORK_FAULT_INTERNAL:
    default:
        return "internal";
    }
}
