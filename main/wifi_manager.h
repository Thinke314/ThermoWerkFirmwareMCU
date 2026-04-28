#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Starts Wi-Fi in AP+STA capable mode. The first implementation always starts
// a local fallback access point so the ESP32 can be configured without Linux.
void thermowork_wifi_manager_start(void);

#ifdef __cplusplus
}
#endif
