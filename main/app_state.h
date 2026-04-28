#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "control.h"
#include "safety.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    thermowork_control_output_t output;
    thermowork_control_config_t config;
    thermowork_process_values_t values;
    thermowork_safety_status_t safety;
    bool gpio_level;
    bool wifi_connected;
    bool cloud_enabled;
    bool cloud_connected;
    uint32_t uptime_s;
    uint64_t updated_us;
} thermowork_runtime_status_t;

void thermowork_app_state_init(void);
void thermowork_app_state_set_status(const thermowork_runtime_status_t *status);
thermowork_runtime_status_t thermowork_app_state_get_status(void);
void thermowork_app_state_set_process_values(const thermowork_process_values_t *values);
thermowork_process_values_t thermowork_app_state_get_process_values(void);
void thermowork_app_state_set_config(const thermowork_control_config_t *config);
thermowork_control_config_t thermowork_app_state_get_config(void);
void thermowork_app_state_set_wifi_connected(bool connected);
void thermowork_app_state_set_cloud_state(bool enabled, bool connected);
int thermowork_app_state_format_json(char *buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif
