#include "state_manager.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static thermowork_inputs_t g_in; static thermowork_config_t g_cfg; static thermowork_status_t g_st; static thermowork_cloud_status_t g_cloud; static SemaphoreHandle_t g_m;
#define LOCK() xSemaphoreTake(g_m,portMAX_DELAY)
#define UNLOCK() xSemaphoreGive(g_m)
esp_err_t state_manager_init(const thermowork_config_t *cfg){ g_m=xSemaphoreCreateMutex(); if(!g_m) return ESP_FAIL; memset(&g_in,0,sizeof(g_in)); memset(&g_st,0,sizeof(g_st)); memset(&g_cloud,0,sizeof(g_cloud)); g_cfg=*cfg; return ESP_OK; }
void state_manager_get_inputs(thermowork_inputs_t *v){LOCK();*v=g_in;UNLOCK();}
void state_manager_set_inputs(const thermowork_inputs_t *v){LOCK();g_in=*v;UNLOCK();}
void state_manager_get_config(thermowork_config_t *v){LOCK();*v=g_cfg;UNLOCK();}
void state_manager_set_config(const thermowork_config_t *v){LOCK();g_cfg=*v;UNLOCK();}
void state_manager_get_status(thermowork_status_t *v){LOCK();*v=g_st;UNLOCK();}
void state_manager_set_status(const thermowork_status_t *v){LOCK();g_st=*v;UNLOCK();}
void state_manager_get_cloud(thermowork_cloud_status_t *v){LOCK();*v=g_cloud;UNLOCK();}
void state_manager_set_cloud(const thermowork_cloud_status_t *v){LOCK();g_cloud=*v;UNLOCK();}
