#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THERMOWORK_CONTROL_MODE_DISABLED = 0,
    THERMOWORK_CONTROL_MODE_PV_SURPLUS,
    THERMOWORK_CONTROL_MODE_MANUAL_POWER,
    THERMOWORK_CONTROL_MODE_BURST_PERCENT,
    THERMOWORK_CONTROL_MODE_TEST
} thermowork_control_mode_t;

typedef struct {
    int32_t grid_power_w;      // Positive import, negative export/surplus
    int32_t pv_power_w;
    int32_t load_power_w;
    float tank_top_c;
    float tank_mid_c;
    float tank_bottom_c;
    float flow_line_c;
    float return_line_c;
    float ambient_c;
    bool temp_valid;
    bool enable;
    bool emergency_stop;
    uint32_t rx_age_ms;
} thermowork_process_values_t;

typedef struct {
    thermowork_control_mode_t mode;
    int32_t max_power_w;
    int32_t min_power_w;
    int32_t manual_power_w;
    int32_t target_grid_power_w;
    uint16_t burst_target_permille;
    float temperature_limit_c;
    uint32_t uart_timeout_ms;
    uint32_t burst_window_ms;
    int ssr_gpio_pin;
    bool output_active_high;
} thermowork_control_config_t;

typedef struct {
    int32_t power_setpoint_w;
    uint16_t duty_permille;
    bool outputs_enabled;
} thermowork_control_output_t;

void thermowork_control_init(void);
void thermowork_control_set_config(const thermowork_control_config_t *config);
void thermowork_control_update_process_values(const thermowork_process_values_t *values);
void thermowork_control_step(thermowork_control_output_t *output);
const thermowork_control_config_t *thermowork_control_get_config(void);
const thermowork_process_values_t *thermowork_control_get_values(void);

#ifdef __cplusplus
}
#endif
