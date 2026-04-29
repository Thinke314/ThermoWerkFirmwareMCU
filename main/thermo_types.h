#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

typedef enum { THERMOWORK_MODE_OFF=0, THERMOWORK_MODE_AUTO=1, THERMOWORK_MODE_MANUAL=2, THERMOWORK_MODE_TEST=3 } thermowork_mode_t;
typedef enum { THERMOWORK_FAULT_OK=0, THERMOWORK_FAULT_DISABLED=1, THERMOWORK_FAULT_OVER_TEMPERATURE=2, THERMOWORK_FAULT_INPUT_TIMEOUT=3, THERMOWORK_FAULT_INVALID_CONFIG=4, THERMOWORK_FAULT_MANUAL_STOP=5, THERMOWORK_FAULT_GPIO_ERROR=6 } thermowork_fault_code_t;
typedef struct { int32_t pv_power_w; int32_t house_power_w; int32_t grid_power_w; float temperature_c; bool input_valid; int64_t last_update_ms; } thermowork_inputs_t;
typedef struct { thermowork_mode_t mode; bool enabled; int32_t heater_max_w; int32_t manual_power_w; float target_temperature_c; float max_temperature_c; uint32_t input_timeout_ms; uint32_t ssr_window_ms; gpio_num_t ssr_gpio; bool cloud_enabled; } thermowork_config_t;
typedef struct { int32_t surplus_power_w; int32_t heater_setpoint_w; uint8_t ssr_duty_percent; bool ssr_active; bool ssr_gpio_state; bool fault; int fault_code; char fault_reason[64]; } thermowork_status_t;
typedef struct { bool enabled; bool connected; char device_id[64]; char endpoint_url[128]; char token[128]; int64_t last_sync_ms; } thermowork_cloud_status_t;
