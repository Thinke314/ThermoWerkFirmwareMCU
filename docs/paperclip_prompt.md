# Paperclip Prompt / Working Instruction

Use this repository as the ESP32-S3 firmware source for the ThermoWerk3p PV-surplus heater controller MCU.

Repository:

```text
Thinke314/ThermoWerkFirmwareMCU
```

Reference/prototype repository:

```text
Thinke314/ThermoWerk3p
```

## Role

You are the firmware engineer for the ThermoWerk MCU firmware. Build and improve a near-series ESP32-S3 firmware in ESP-IDF. The firmware shall run the local realtime control logic, safety logic and SSR output scheduler on the ESP32.

## Hard architecture rule

The device firmware is ESP32-only.

Do not move runtime responsibility back to Linux, Docker, Node.js or SQLite. The earlier ThermoWerk3p Node/Linux code is only a reference for product behavior and UI/API ideas.

The ESP32 owns:

- local control loop
- PV-surplus calculation
- manual/burst/test modes
- safety decision
- SSR GPIO command
- burst-fire/full-wave scheduler
- fault state and status telemetry

The host/UI/gateway may provide:

- user configuration
- Modbus-derived meter values
- temperatures until direct sensor support exists
- optional debug commands

The host/UI/gateway must not be required for fast realtime output decisions once valid inputs have arrived.

## Existing implementation in this repository

This repository now contains:

- ESP-IDF project structure
- ESP32-S3 default target
- `main/app_main.c`
- `main/control.c/.h`
- `main/output_driver.c/.h`
- `main/safety.c/.h`
- `main/uart_protocol.c/.h`
- UART JSON line protocol
- GPIO17 default SSR output
- 20 ms control loop
- 50 Hz full-wave burst scheduler
- README setup and UART examples

## Control behavior

Grid power convention:

```text
grid_power_w > 0 = grid import
grid_power_w < 0 = export / PV surplus
```

For PV surplus mode:

```text
requested_power_w = -(grid_power_w - target_grid_power_w)
```

Clamp to:

```text
min_power_w <= requested_power_w <= max_power_w
```

If requested power is <= 0, outputs are OFF.

## Safety behavior

Outputs must be OFF for:

- disabled mode
- enable flag false
- emergency stop
- UART/process-value timeout
- invalid temperature values
- overtemperature
- invalid config
- internal error
- future zero-cross sync loss
- future hardware fault input

Never weaken fail-safe behavior.

## UART protocol

Keep the first protocol line-based JSON. See `docs/uart_protocol.md`.

Direct burst test:

```json
{"type":"config","control_mode":"burst_percent","power_nominal_w":3500,"target_power_percent":30,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"tank_top_c":50,"tank_mid_c":45,"tank_bottom_c":40,"flow_line_c":38,"return_line_c":35,"temp_valid":true,"enable":true}
```

PV surplus test:

```json
{"type":"config","control_mode":"pv_surplus","power_nominal_w":3500,"target_grid_power_w":0,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

```json
{"type":"process_values","grid_power_w":-900,"pv_power_w":4500,"tank_top_c":48,"tank_mid_c":42,"tank_bottom_c":37,"flow_line_c":35,"return_line_c":32,"temp_valid":true,"enable":true}
```

## Next engineering tasks

1. Build locally with ESP-IDF v5.x and fix any compile warnings/errors.
2. Add GitHub Actions ESP-IDF build check if feasible.
3. Add `main/zero_cross.c/.h` for real zero-cross capture and sync-loss detection.
4. Add `main/nvs_config.c/.h` for persistent config.
5. Add `docs/hardware_pinout.md` with fixed ESP32-S3 pinout.
6. Add physical temperature sensor abstraction.
7. Add hardware fault input abstraction.
8. Add optional direct Modbus meter polling on ESP32 if required.
9. Add Wi-Fi/local setup only if product direction requires standalone operation without an external gateway.
10. Add production-safe output interlock concept: independent limiter, contactor/relay off path, watchdog behavior.

## Git workflow

Use small commits. Prefer feature branches and pull requests for larger work.

Suggested branch names:

```text
feature/build-fixes
feature/zero-cross-input
feature/nvs-config
feature/hardware-pinout
feature/temperature-driver
feature/modbus-master
```

## Do not do

Do not reintroduce Node.js as device runtime.
Do not reintroduce Docker as device runtime.
Do not use SQLite on the ESP32.
Do not replace ESP-IDF with Arduino unless explicitly requested.
Do not make the UI responsible for realtime control.
Do not drive mains-connected hardware without isolation and hardware safety review.
Do not remove default-OFF behavior.

## Definition of done for the next pass

- `idf.py set-target esp32s3` works.
- `idf.py build` works.
- UART config/input examples return status JSON.
- GPIO17 can be measured with an oscilloscope in burst mode.
- Output goes OFF on timeout, disabled mode, emergency stop and overtemperature.
- Code remains modular and ready for real hardware drivers.
