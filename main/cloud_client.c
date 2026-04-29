#include "cloud_client.h"
#include "state_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void cloud_client_task(void *p){(void)p; while(1){thermowork_cloud_status_t c; state_manager_get_cloud(&c); c.connected=false; state_manager_set_cloud(&c); vTaskDelay(pdMS_TO_TICKS(5000));}}
