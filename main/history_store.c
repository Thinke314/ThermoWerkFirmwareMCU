#include "history_store.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t g_lock;
static thermowork_history_sample_t g_samples[THERMOWORK_HISTORY_CAPACITY];
static uint32_t g_next = 0;
static uint32_t g_count = 0;

void thermowork_history_store_init(void)
{
    g_lock = xSemaphoreCreateMutex();
    memset(g_samples, 0, sizeof(g_samples));
    g_next = 0;
    g_count = 0;
}

void thermowork_history_store_add(const thermowork_runtime_status_t *status)
{
    if (status == NULL) {
        return;
    }

    thermowork_history_sample_t sample = {
        .uptime_s = status->uptime_s,
        .grid_power_w = status->values.grid_power_w,
        .pv_power_w = status->values.pv_power_w,
        .power_setpoint_w = status->output.power_setpoint_w,
        .duty_permille = status->output.duty_permille,
        .tank_top_c = status->values.tank_top_c,
        .fault = (int)status->safety.fault,
    };

    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_samples[g_next] = sample;
    g_next = (g_next + 1U) % THERMOWORK_HISTORY_CAPACITY;
    if (g_count < THERMOWORK_HISTORY_CAPACITY) {
        g_count++;
    }
    xSemaphoreGive(g_lock);
}

int thermowork_history_store_format_json(char *buffer, uint32_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return -1;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    int written = snprintf(buffer, buffer_size, "{\"samples\":[");
    for (uint32_t i = 0; i < g_count && written > 0 && (uint32_t)written < buffer_size; i++) {
        uint32_t idx = (g_next + THERMOWORK_HISTORY_CAPACITY - g_count + i) % THERMOWORK_HISTORY_CAPACITY;
        int n = snprintf(
            buffer + written,
            buffer_size - (uint32_t)written,
            "%s{\"uptime_s\":%lu,\"grid_power_w\":%ld,\"pv_power_w\":%ld,\"power_setpoint_w\":%ld,\"duty_permille\":%u,\"tank_top_c\":%.1f,\"fault\":%d}",
            i == 0 ? "" : ",",
            (unsigned long)g_samples[idx].uptime_s,
            (long)g_samples[idx].grid_power_w,
            (long)g_samples[idx].pv_power_w,
            (long)g_samples[idx].power_setpoint_w,
            (unsigned)g_samples[idx].duty_permille,
            (double)g_samples[idx].tank_top_c,
            g_samples[idx].fault
        );
        if (n < 0) {
            written = -1;
            break;
        }
        written += n;
    }
    if (written > 0 && (uint32_t)written < buffer_size) {
        written += snprintf(buffer + written, buffer_size - (uint32_t)written, "]}");
    }
    xSemaphoreGive(g_lock);
    return written;
}
