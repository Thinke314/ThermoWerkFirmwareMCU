#include "app_state.h"

#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t g_lock;
static thermowork_runtime_status_t g_status;

static void lock_state(void)
{
    if (g_lock != NULL) {
        xSemaphoreTake(g_lock, portMAX_DELAY);
    }
}

static void unlock_state(void)
{
    if (g_lock != NULL) {
        xSemaphoreGive(g_lock);
    }
}

void thermowork_app_state_init(void)
{
    g_lock = xSemaphoreCreateMutex();
    memset(&g_status, 0, sizeof(g_status));
    g_status.updated_us = (uint64_t)esp_timer_get_time();
}

void thermowork_app_state_set_status(const thermowork_runtime_status_t *status)
{
    if (status == NULL) {
        return;
    }
    lock_state();
    g_status = *status;
    g_status.updated_us = (uint64_t)esp_timer_get_time();
    unlock_state();
}

thermowork_runtime_status_t thermowork_app_state_get_status(void)
{
    thermowork_runtime_status_t copy;
    lock_state();
    copy = g_status;
    unlock_state();
    return copy;
}

void thermowork_app_state_set_process_values(const thermowork_process_values_t *values)
{
    if (values == NULL) {
        return;
    }
    lock_state();
    g_status.values = *values;
    g_status.updated_us = (uint64_t)esp_timer_get_time();
    unlock_state();
}

thermowork_process_values_t thermowork_app_state_get_process_values(void)
{
    thermowork_process_values_t copy;
    lock_state();
    copy = g_status.values;
    unlock_state();
    return copy;
}

void thermowork_app_state_set_config(const thermowork_control_config_t *config)
{
    if (config == NULL) {
        return;
    }
    lock_state();
    g_status.config = *config;
    g_status.updated_us = (uint64_t)esp_timer_get_time();
    unlock_state();
}

thermowork_control_config_t thermowork_app_state_get_config(void)
{
    thermowork_control_config_t copy;
    lock_state();
    copy = g_status.config;
    unlock_state();
    return copy;
}

void thermowork_app_state_set_wifi_connected(bool connected)
{
    lock_state();
    g_status.wifi_connected = connected;
    g_status.updated_us = (uint64_t)esp_timer_get_time();
    unlock_state();
}

void thermowork_app_state_set_cloud_state(bool enabled, bool connected)
{
    lock_state();
    g_status.cloud_enabled = enabled;
    g_status.cloud_connected = connected;
    g_status.updated_us = (uint64_t)esp_timer_get_time();
    unlock_state();
}

int thermowork_app_state_format_json(char *buffer, uint32_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return -1;
    }

    thermowork_runtime_status_t s = thermowork_app_state_get_status();
    return snprintf(
        buffer,
        buffer_size,
        "{\"type\":\"status\",\"uptime_s\":%lu,\"mode\":%d,\"power_setpoint_w\":%ld,\"duty_permille\":%u,\"outputs_enabled\":%s,\"gpio_level\":%s,\"ssr_gpio_pin\":%d,\"grid_power_w\":%ld,\"pv_power_w\":%ld,\"tank_top_c\":%.1f,\"tank_mid_c\":%.1f,\"tank_bottom_c\":%.1f,\"rx_age_ms\":%lu,\"fault\":\"%s\",\"wifi_connected\":%s,\"cloud_enabled\":%s,\"cloud_connected\":%s}",
        (unsigned long)s.uptime_s,
        (int)s.config.mode,
        (long)s.output.power_setpoint_w,
        (unsigned)s.output.duty_permille,
        s.output.outputs_enabled ? "true" : "false",
        s.gpio_level ? "true" : "false",
        s.config.ssr_gpio_pin,
        (long)s.values.grid_power_w,
        (long)s.values.pv_power_w,
        (double)s.values.tank_top_c,
        (double)s.values.tank_mid_c,
        (double)s.values.tank_bottom_c,
        (unsigned long)s.values.rx_age_ms,
        thermowork_fault_to_string(s.safety.fault),
        s.wifi_connected ? "true" : "false",
        s.cloud_enabled ? "true" : "false",
        s.cloud_connected ? "true" : "false"
    );
}
