#include "uart_protocol.h"

#include <stdio.h>
#include <string.h>

#include "cJSON.h"

static bool json_get_bool_default(const cJSON *root, const char *name, bool default_value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return default_value;
}

static int32_t json_get_i32_default(const cJSON *root, const char *name, int32_t default_value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (cJSON_IsNumber(item)) {
        return (int32_t)item->valuedouble;
    }
    return default_value;
}

static uint16_t json_get_permille_default(const cJSON *root, const char *permille_name, const char *percent_name, uint16_t default_value)
{
    const cJSON *permille = cJSON_GetObjectItemCaseSensitive(root, permille_name);
    if (cJSON_IsNumber(permille)) {
        int32_t v = (int32_t)permille->valuedouble;
        if (v < 0) return 0;
        if (v > 1000) return 1000;
        return (uint16_t)v;
    }

    const cJSON *percent = cJSON_GetObjectItemCaseSensitive(root, percent_name);
    if (cJSON_IsNumber(percent)) {
        int32_t v = (int32_t)(percent->valuedouble * 10.0);
        if (v < 0) return 0;
        if (v > 1000) return 1000;
        return (uint16_t)v;
    }

    return default_value;
}

static float json_get_float_default(const cJSON *root, const char *name, float default_value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (cJSON_IsNumber(item)) {
        return (float)item->valuedouble;
    }
    return default_value;
}

static thermowork_control_mode_t parse_control_mode(const char *mode)
{
    if (mode == NULL) {
        return THERMOWORK_CONTROL_MODE_DISABLED;
    }
    if (strcmp(mode, "pv_surplus") == 0 || strcmp(mode, "auto") == 0) {
        return THERMOWORK_CONTROL_MODE_PV_SURPLUS;
    }
    if (strcmp(mode, "manual_power") == 0 || strcmp(mode, "manual") == 0) {
        return THERMOWORK_CONTROL_MODE_MANUAL_POWER;
    }
    if (strcmp(mode, "burst_percent") == 0 || strcmp(mode, "burst") == 0) {
        return THERMOWORK_CONTROL_MODE_BURST_PERCENT;
    }
    if (strcmp(mode, "test") == 0) {
        return THERMOWORK_CONTROL_MODE_TEST;
    }
    return THERMOWORK_CONTROL_MODE_DISABLED;
}

void thermowork_uart_protocol_init(void)
{
}

thermowork_uart_msg_type_t thermowork_uart_parse_line(const char *line, thermowork_uart_message_t *message)
{
    if (line == NULL || message == NULL) {
        return THERMOWORK_UART_MSG_INVALID;
    }

    memset(message, 0, sizeof(*message));
    message->type = THERMOWORK_UART_MSG_INVALID;

    cJSON *root = cJSON_Parse(line);
    if (root == NULL) {
        return THERMOWORK_UART_MSG_INVALID;
    }

    const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (!cJSON_IsString(type) || type->valuestring == NULL) {
        cJSON_Delete(root);
        return THERMOWORK_UART_MSG_INVALID;
    }

    if (strcmp(type->valuestring, "process_values") == 0 || strcmp(type->valuestring, "inputs") == 0) {
        message->type = THERMOWORK_UART_MSG_PROCESS_VALUES;
        message->process_values.grid_power_w = json_get_i32_default(root, "grid_power_w", json_get_i32_default(root, "site_import_export_w", 0));
        message->process_values.pv_power_w = json_get_i32_default(root, "pv_power_w", json_get_i32_default(root, "pv_surplus_w", 0));
        message->process_values.load_power_w = json_get_i32_default(root, "load_power_w", 0);
        message->process_values.tank_top_c = json_get_float_default(root, "tank_top_c", json_get_float_default(root, "temp_1_c", 20.0f));
        message->process_values.tank_mid_c = json_get_float_default(root, "tank_mid_c", json_get_float_default(root, "temp_2_c", 20.0f));
        message->process_values.tank_bottom_c = json_get_float_default(root, "tank_bottom_c", 20.0f);
        message->process_values.flow_line_c = json_get_float_default(root, "flow_line_c", 20.0f);
        message->process_values.return_line_c = json_get_float_default(root, "return_line_c", 20.0f);
        message->process_values.ambient_c = json_get_float_default(root, "ambient_c", 20.0f);
        message->process_values.temp_valid = json_get_bool_default(root, "temp_valid", true);
        message->process_values.enable = json_get_bool_default(root, "enable", false);
        message->process_values.emergency_stop = json_get_bool_default(root, "emergency_stop", false);
        message->process_values.rx_age_ms = 0;
    } else if (strcmp(type->valuestring, "config") == 0 || strcmp(type->valuestring, "settings") == 0) {
        message->type = THERMOWORK_UART_MSG_CONFIG;
        const cJSON *mode = cJSON_GetObjectItemCaseSensitive(root, "control_mode");
        if (!cJSON_IsString(mode)) {
            mode = cJSON_GetObjectItemCaseSensitive(root, "mode");
        }
        message->config.mode = parse_control_mode(cJSON_IsString(mode) ? mode->valuestring : NULL);
        message->config.max_power_w = json_get_i32_default(root, "max_power_w", json_get_i32_default(root, "power_nominal_w", 3500));
        message->config.min_power_w = json_get_i32_default(root, "min_power_w", 0);
        message->config.manual_power_w = json_get_i32_default(root, "manual_power_w", 0);
        message->config.target_grid_power_w = json_get_i32_default(root, "target_grid_power_w", 0);
        message->config.burst_target_permille = json_get_permille_default(root, "burst_target_permille", "target_power_percent", 0);
        message->config.temperature_limit_c = json_get_float_default(root, "temperature_limit_c", 85.0f);
        message->config.uart_timeout_ms = (uint32_t)json_get_i32_default(root, "uart_timeout_ms", 3000);
        message->config.burst_window_ms = (uint32_t)json_get_i32_default(root, "burst_window_ms", json_get_i32_default(root, "window_ms", 1000));
        message->config.ssr_gpio_pin = json_get_i32_default(root, "ssr_gpio_pin", 17);
        message->config.output_active_high = json_get_bool_default(root, "output_active_high", true);
    } else if (strcmp(type->valuestring, "command") == 0) {
        message->type = THERMOWORK_UART_MSG_COMMAND;
        message->command_enable_outputs = json_get_bool_default(root, "enable_outputs", false);
        message->command_emergency_stop = json_get_bool_default(root, "emergency_stop", false);
        message->command_reset_fault = json_get_bool_default(root, "reset_fault", false);
    }

    thermowork_uart_msg_type_t result = message->type;
    cJSON_Delete(root);
    return result;
}

int thermowork_uart_format_status(
    char *buffer,
    size_t buffer_size,
    const thermowork_control_output_t *output,
    const thermowork_control_config_t *config,
    const thermowork_process_values_t *values,
    const char *fault_string,
    bool gpio_level
)
{
    if (buffer == NULL || buffer_size == 0 || output == NULL || config == NULL || values == NULL) {
        return -1;
    }

    if (fault_string == NULL) {
        fault_string = "unknown";
    }

    return snprintf(
        buffer,
        buffer_size,
        "{\"type\":\"status\",\"mode\":%d,\"power_setpoint_w\":%ld,\"duty_permille\":%u,\"outputs_enabled\":%s,\"gpio_level\":%s,\"ssr_gpio_pin\":%d,\"grid_power_w\":%ld,\"pv_power_w\":%ld,\"rx_age_ms\":%lu,\"fault\":\"%s\"}\n",
        (int)config->mode,
        (long)output->power_setpoint_w,
        (unsigned)output->duty_permille,
        output->outputs_enabled ? "true" : "false",
        gpio_level ? "true" : "false",
        config->ssr_gpio_pin,
        (long)values->grid_power_w,
        (long)values->pv_power_w,
        (unsigned long)values->rx_age_ms,
        fault_string
    );
}
