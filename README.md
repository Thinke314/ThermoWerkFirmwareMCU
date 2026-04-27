# ThermoWerkFirmwareMCU

ESP32 firmware repository for the ThermoWerk PV-surplus heater controller / Leistungssteller MCU.

This repository is intended as the firmware-side starting point for a near-series ESP32 implementation. Paperclip or another coding agent can use this repo directly as the working base and push changes into it.

## Product direction

ThermoWerk is intended as a smart PV-surplus controller for resistive loads, especially heating rods / Heizstab applications.

Target architecture:

- ESP32 runs the local realtime control logic.
- External UI / host software sends setpoints and bus/measured values to the ESP32.
- Communication between host and ESP32 initially happens over UART.
- Values received over UART can include Modbus-derived grid power, PV power, load power, temperatures, enable states and configuration.
- ESP32 decides locally how to drive the power stage.
- Later hardware values such as zero-cross detection, temperature inputs and SSR / gate outputs will be implemented directly on the ESP32.

## Control concept

The ESP32 firmware shall eventually implement:

- PV-surplus control for resistive loads
- full-wave / burst-fire control for zero-cross SSRs
- optional 1-phase or 3-phase operation
- 3-phase star wiring with either 3 SSRs or cost-optimized 2-SSR topology, depending on hardware variant
- local safety state machine
- temperature limit channels
- watchdog and fail-safe output disable
- UART command protocol
- persistent configuration via NVS
- debug logging over serial

## Current repository state

Initial ESP-IDF starter structure with firmware skeleton and documentation.

## Recommended development environment

- ESP-IDF v5.x
- VS Code
- Espressif IDF Extension
- GitHub access configured for Paperclip / coding agent

## Quick setup for developer / Paperclip

```bash
git clone https://github.com/Thinke314/ThermoWerkFirmwareMCU.git
cd ThermoWerkFirmwareMCU
```

Set the target depending on selected MCU:

```bash
idf.py set-target esp32
```

or for ESP32-S3:

```bash
idf.py set-target esp32s3
```

Build:

```bash
idf.py build
```

Flash and monitor:

```bash
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

## Initial UART protocol direction

The first firmware version should support a simple line-based JSON protocol over UART.

Example host-to-MCU message:

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"load_power_w":800,"temp_1_c":54.2,"temp_2_c":38.7,"enable":true}
```

Example config message:

```json
{"type":"config","max_power_w":3500,"min_power_w":0,"control_mode":"pv_surplus","temperature_limit_c":85}
```

Example MCU response:

```json
{"type":"status","state":"run","power_setpoint_w":1200,"duty_permille":343,"outputs_enabled":true,"fault":"none"}
```

## Safety principle

Outputs must default to OFF.

Any of the following shall force outputs OFF:

- UART timeout
- invalid command frame
- overtemperature
- disabled enable flag
- watchdog fault
- internal control fault
- zero-cross sync loss once hardware zero-crossing is implemented

## Repo structure

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
│   ├── safety.c
│   ├── safety.h
│   ├── uart_protocol.c
│   └── uart_protocol.h
└── sdkconfig.defaults
```
