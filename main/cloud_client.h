#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool enabled;
    char endpoint_url[160];
    char device_id[48];
    unsigned publish_interval_s;
} thermowork_cloud_config_t;

void thermowork_cloud_client_init(void);
void thermowork_cloud_client_set_config(const thermowork_cloud_config_t *config);
thermowork_cloud_config_t thermowork_cloud_client_get_config(void);
void thermowork_cloud_client_start(void);

#ifdef __cplusplus
}
#endif
