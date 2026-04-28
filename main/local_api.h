#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Starts the ESP32-local HTTP API and embedded web interface.
// Endpoints are implemented in local_api.c.
void thermowork_local_api_start(void);

#ifdef __cplusplus
}
#endif
