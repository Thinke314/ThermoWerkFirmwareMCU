#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THERMOWORK_FAULT_NONE = 0,
    THERMOWORK_FAULT_DISABLED,
    THERMOWORK_FAULT_EMERGENCY_STOP,
    THERMOWORK_FAULT_UART_TIMEOUT,
    THERMOWORK_FAULT_SENSOR_INVALID,
    THERMOWORK_FAULT_OVERTEMPERATURE,
    THERMOWORK_FAULT_INVALID_CONFIG,
    THERMOWORK_FAULT_INTERNAL
} thermowork_fault_t;

typedef struct {
    thermowork_fault_t fault;
    bool outputs_allowed;
} thermowork_safety_status_t;

void thermowork_safety_init(void);
thermowork_safety_status_t thermowork_safety_evaluate(
    const thermowork_control_config_t *config,
    const thermowork_process_values_t *values
);
const char *thermowork_fault_to_string(thermowork_fault_t fault);

#ifdef __cplusplus
}
#endif
