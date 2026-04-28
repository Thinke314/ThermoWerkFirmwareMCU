#pragma once

#include <stdint.h>

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THERMOWORK_HISTORY_CAPACITY 240

typedef struct {
    uint32_t uptime_s;
    int32_t grid_power_w;
    int32_t pv_power_w;
    int32_t power_setpoint_w;
    uint16_t duty_permille;
    float tank_top_c;
    int fault;
} thermowork_history_sample_t;

void thermowork_history_store_init(void);
void thermowork_history_store_add(const thermowork_runtime_status_t *status);
int thermowork_history_store_format_json(char *buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif
