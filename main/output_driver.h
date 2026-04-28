#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int ssr_gpio_pin;
    bool output_active_high;
    uint32_t burst_window_ms;
    uint32_t full_wave_ms;
} thermowork_output_config_t;

typedef struct {
    bool output_level;
    bool scheduler_enabled;
    uint16_t duty_permille;
    uint32_t burst_window_ms;
    int ssr_gpio_pin;
} thermowork_output_state_t;

void thermowork_output_driver_init(const thermowork_output_config_t *config);
void thermowork_output_driver_update_config(const thermowork_output_config_t *config);
void thermowork_output_driver_apply(const thermowork_control_output_t *output);
thermowork_output_state_t thermowork_output_driver_get_state(void);
void thermowork_output_driver_force_off(void);

#ifdef __cplusplus
}
#endif
