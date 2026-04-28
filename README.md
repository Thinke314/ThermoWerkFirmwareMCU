# ThermoWerkFirmwareMCU

ESP32-S3 firmware repository for the ThermoWerk3p PV-surplus heater controller / Leistungssteller MCU.

This repository is the MCU-only product firmware line. One ESP32-S3 shall run the local control loop, safety logic, SSR/burst-fire output, local web interface and optional cloud telemetry. Linux, Docker, SQLite and Node.js are not required on the device.

## Product direction

The current product requirements are written into the repository and should guide all future implementation work:

```text
docs/product_requirements.md        Full product requirements from the boiler/heater controller concept
docs/ui_requirements.md             Polished local ESP32 UI requirements
docs/ota_update_requirements.md     OTA update requirements and safety rules
docs/implementation_roadmap.md      Milestone plan for build, UI, OTA, NVS, Wi-Fi, Modbus, safety
docs/firmware_spec.md               Firmware architecture and runtime responsibilities
docs/uart_protocol.md               UART/JSON protocol
docs/paperclip_prompt.md            Agent working instruction
```

## Target architecture

- ESP32-S3 as the main controller.
- Local realtime control on the ESP32.
- Local Wi-Fi setup access point.
- Local embedded web interface served directly from the ESP32.
- Local REST API for status, configuration, process values, commands, history and cloud telemetry setup.
- UART line-based JSON protocol remains available for debug, gateway or production test.
- Optional HTTP cloud telemetry client.
- Future OTA update support is required.
- Control remains local even when cloud is offline.
- Default SSR output: GPIO17.
- Default control period: 20 ms, matching one full 50 Hz mains cycle.
- Output method: burst-fire / full-wave packet control for zero-cross SSRs.
- Outputs fail safe to OFF.

## Implemented now

- ESP-IDF project structure for VS Code.
- ESP32-S3 default target.
- Local AP: `ThermoWerk-Setup`, password `thermowerk`.
- Embedded local web interface on port 80.
- REST endpoints:
  - `GET /`
  - `GET /api/status`
  - `GET /api/history`
  - `POST /api/config`
  - `POST /api/inputs`
  - `POST /api/command`
  - `POST /api/cloud`
- UART JSON input parser.
- Runtime state module shared by UART, web API, control loop and cloud task.
- Local RAM ring-buffer history.
- Optional HTTP cloud telemetry task.
- ThermoWerk3p-style modes:
  - `disabled`
  - `pv_surplus`
  - `manual_power`
  - `burst_percent`
  - `test`
- PV-surplus power calculation.
- Manual power command.
- Burst-percent command.
- Temperature safety with tank/flow/return channels.
- Emergency stop flag.
- UART/process-value timeout watchdog.
- GPIO SSR output driver.
- Bresenham-style full-wave distribution over a burst window.
- JSON status output every 500 ms over UART.

## Required next features

These are not optional for the product direction:

- build and flash baseline must be kept working
- polished mobile-first local UI
- OTA update module and API
- OTA-capable partition layout
- NVS persistent configuration
- Wi-Fi station setup with fallback AP
- real meter input: Modbus TCP and/or Modbus RTU
- real temperature input abstraction
- zero-cross input and sync-loss fault
- hardware safety inputs
- factory reset path
- production/recovery documentation

## Repository structure

```text
.
├── CMakeLists.txt
├── README.md
├── partitions.csv
├── sdkconfig.defaults
├── docs/
│   ├── firmware_spec.md
│   ├── implementation_roadmap.md
│   ├── ota_update_requirements.md
│   ├── paperclip_prompt.md
│   ├── product_requirements.md
│   ├── ui_requirements.md
│   └── uart_protocol.md
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c
│   ├── app_state.c
│   ├── app_state.h
│   ├── cloud_client.c
│   ├── cloud_client.h
│   ├── control.c
│   ├── control.h
│   ├── history_store.c
│   ├── history_store.h
│   ├── local_api.c
│   ├── local_api.h
│   ├── output_driver.c
│   ├── output_driver.h
│   ├── safety.c
│   ├── safety.h
│   ├── uart_protocol.c
│   ├── uart_protocol.h
│   ├── wifi_manager.c
│   └── wifi_manager.h
└── .github/workflows/esp-idf-build.yml
```

## Quick setup in VS Code / ESP-IDF

```bash
git clone https://github.com/Thinke314/ThermoWerkFirmwareMCU.git
cd ThermoWerkFirmwareMCU
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

On Linux:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Exit monitor:

```text
Ctrl + ]
```

## Local web interface

After flashing, the ESP32 starts a fallback access point:

```text
SSID: ThermoWerk-Setup
Password: thermowerk
```

Connect to this Wi-Fi and open the ESP32 AP address in the browser, typically:

```text
http://192.168.4.1/
```

The local UI allows first testing of:

- current status
- control mode
- nominal power
- manual power
- burst percentage
- simulated grid/PV/temperature inputs
- emergency stop
- cloud telemetry endpoint

## Local API examples

Status:

```bash
curl http://192.168.4.1/api/status
```

History:

```bash
curl http://192.168.4.1/api/history
```

Config, 30% burst test:

```bash
curl -X POST http://192.168.4.1/api/config \
  -d '{"type":"config","control_mode":"burst_percent","power_nominal_w":3500,"target_power_percent":30,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}'
```

Inputs / enable:

```bash
curl -X POST http://192.168.4.1/api/inputs \
  -d '{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"tank_top_c":50,"tank_mid_c":45,"tank_bottom_c":40,"flow_line_c":38,"return_line_c":35,"ambient_c":22,"temp_valid":true,"enable":true}'
```

Emergency stop:

```bash
curl -X POST http://192.168.4.1/api/command \
  -d '{"type":"command","emergency_stop":true}'
```

Cloud telemetry setup:

```bash
curl -X POST http://192.168.4.1/api/cloud \
  -d '{"enabled":true,"endpoint_url":"https://example.com/telemetry","device_id":"thermowerk-esp32","publish_interval_s":30}'
```

## UART test

Send config:

```json
{"type":"config","control_mode":"burst_percent","power_nominal_w":3500,"target_power_percent":30,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

Send process values / enable:

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"load_power_w":0,"tank_top_c":50,"tank_mid_c":45,"tank_bottom_c":40,"flow_line_c":38,"return_line_c":35,"ambient_c":22,"temp_valid":true,"enable":true}
```

Expected result:

- Status JSON appears every 500 ms.
- `outputs_enabled` becomes `true` if safety is OK.
- GPIO17 toggles according to the burst scheduler.
- With `target_power_percent:30`, output is ON for roughly 30% of the full waves within the configured burst window.

## PV-surplus mode

Config:

```json
{"type":"config","control_mode":"pv_surplus","power_nominal_w":3500,"target_grid_power_w":0,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

Input example:

```json
{"type":"process_values","grid_power_w":-900,"pv_power_w":4500,"tank_top_c":48,"tank_mid_c":42,"tank_bottom_c":37,"flow_line_c":35,"return_line_c":32,"temp_valid":true,"enable":true}
```

Convention:

- `grid_power_w > 0` = grid import.
- `grid_power_w < 0` = export / surplus.
- With `target_grid_power_w = 0`, the ESP32 tries to consume the exported surplus up to `power_nominal_w`.

## Safety behavior

Output is forced OFF when any of these conditions are true:

- `enable` is false.
- `emergency_stop` is true.
- UART/process-value timeout.
- Temperatures are invalid.
- Any main temperature channel exceeds `temperature_limit_c`.
- Configuration is invalid.
- Control mode is `disabled`.

Cloud failure does not stop the local control loop. Cloud is telemetry only in this version.

## Hardware warning

This firmware is for development of a mains-voltage power controller. GPIO output must only drive suitable isolated SSR/opto hardware. Do not connect ESP32 GPIO directly to mains circuitry. Final hardware requires proper electrical design, creepage/clearance, fusing, thermal design, EMC consideration and compliance testing.
