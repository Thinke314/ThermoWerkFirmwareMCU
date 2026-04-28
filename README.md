# ThermoWerkFirmwareMCU

ESP32-S3 firmware repository for the ThermoWerk3p PV-surplus heater controller / Leistungssteller MCU.

This repository is the MCU-only firmware version of the earlier ThermoWerk3p Linux/Node prototype. The product direction is now: one ESP32-S3 runs the local control loop, safety logic and SSR/burst-fire output. Linux, Docker, SQLite and Node.js are not required for the device firmware.

## Target architecture

- ESP32-S3 as the main controller.
- Local realtime control on the ESP32.
- UART line-based JSON protocol for initial host/debug input.
- Process values can come from an external gateway, Modbus bridge, UI board or later directly from ESP32 interfaces.
- ESP32 decides locally how to drive the SSR output.
- Default SSR output: GPIO17.
- Default control period: 20 ms, matching one full 50 Hz mains cycle.
- Output method: burst-fire / full-wave packet control for zero-cross SSRs.
- Outputs fail safe to OFF.

## Implemented in this first ESP32 version

- ESP-IDF project structure for VS Code.
- ESP32-S3 default target.
- UART JSON input parser.
- ThermoWerk3p-style modes:
  - `disabled`
  - `pv_surplus`
  - `manual_power`
  - `burst_percent`
  - `test`
- PV-surplus power calculation.
- Manual power command.
- Burst-percent command compatible with the former prototype idea.
- Temperature safety with tank/flow/return channels.
- Emergency stop flag.
- UART timeout watchdog.
- GPIO SSR output driver.
- Bresenham-style full-wave distribution over a burst window.
- JSON status output every 500 ms.

## Repository structure

```text
.
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── firmware_spec.md
│   ├── paperclip_prompt.md
│   └── uart_protocol.md
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c
│   ├── control.c
│   ├── control.h
│   ├── output_driver.c
│   ├── output_driver.h
│   ├── safety.c
│   ├── safety.h
│   ├── uart_protocol.c
│   └── uart_protocol.h
└── sdkconfig.defaults
```

## Quick setup in VS Code / ESP-IDF

Clone:

```bash
git clone https://github.com/Thinke314/ThermoWerkFirmwareMCU.git
cd ThermoWerkFirmwareMCU
```

Set target:

```bash
idf.py set-target esp32s3
```

Build:

```bash
idf.py build
```

Flash and monitor on Windows:

```bash
idf.py -p COMx flash monitor
```

Flash and monitor on Linux:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Exit monitor:

```text
Ctrl + ]
```

## First functional test over UART monitor

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
- With `target_power_percent:30`, the output is ON for roughly 30% of the full waves within the configured burst window.

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
- UART process values timeout.
- Temperatures are invalid.
- Any main temperature channel exceeds `temperature_limit_c`.
- Configuration is invalid.
- Control mode is `disabled`.

## Hardware warning

This firmware is for development of a mains-voltage power controller. GPIO output must only drive suitable isolated SSR/opto hardware. Do not connect ESP32 GPIO directly to mains circuitry. Final hardware requires proper electrical design, creepage/clearance, fusing, thermal design, EMC consideration and compliance testing.
