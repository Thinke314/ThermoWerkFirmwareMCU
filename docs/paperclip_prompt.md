# Paperclip Prompt / Working Instruction

Use this repository as the firmware source for the ThermoWerk PV-surplus heater controller MCU.

Repository:

```text
Thinke314/ThermoWerkFirmwareMCU
```

## Role

You are the firmware engineer for the ThermoWerk MCU firmware. Build a near-series ESP32 firmware in ESP-IDF. The firmware shall run the local realtime control logic on the ESP32. The external UI/host software shall only send setpoints, configuration and bus/process values to the ESP32 over UART in the first phase.

## Product context

ThermoWerk is a PV-surplus controller / Leistungssteller for resistive loads such as heating rods. The product direction is a flexible, robust alternative to common PV heater controllers. The firmware shall be structured cleanly enough that later real hardware drivers can be added without rewriting the control architecture.

## Important architecture decision

Do not implement the main control loop in the host UI. The ESP32 must own the local control logic.

The host/UI sends:

- configuration
- enable flag
- Modbus-derived bus values / meter values
- grid power
- PV power
- temperatures while hardware sensors are not yet implemented
- optional manual/test setpoints

The ESP32 calculates:

- requested heater power
- duty_permille
- output enable decision
- local safety decision
- status and fault state

## Existing starter implementation

This repository already contains:

- ESP-IDF CMake skeleton
- `app_main.c`
- UART JSON line parser
- control module
- safety module
- firmware specification
- UART protocol documentation

## First tasks

1. Make the ESP-IDF project compile cleanly with ESP-IDF v5.x.
2. Fix any compile errors caused by headers, component requirements or ESP-IDF API differences.
3. Keep outputs OFF by default.
4. Add unit-testable pure C functions where practical.
5. Add a hardware abstraction for the output driver, but do not drive real GPIOs until the pinout is explicitly defined.
6. Add a burst-fire/full-wave scheduler module that converts `duty_permille` into on/off full-wave decisions.
7. Add a simulated zero-cross mode for firmware testing.
8. Add a hardware zero-cross interface later.
9. Add persistent config via NVS after the protocol is stable.
10. Add clear serial logs and protocol examples.

## Control behavior

Use grid power convention:

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
- UART timeout
- overtemperature
- invalid config
- internal error
- future zero-cross sync loss
- future hardware fault input

## UART protocol

Keep the first protocol line-based JSON. See `docs/uart_protocol.md`.

Example process values:

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"load_power_w":0,"temp_1_c":30,"temp_2_c":31,"enable":true}
```

Example config:

```json
{"type":"config","control_mode":"pv_surplus","max_power_w":3500,"min_power_w":0,"target_grid_power_w":0,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

Expected status:

```json
{"type":"status","power_setpoint_w":1200,"duty_permille":342,"outputs_enabled":true,"fault":"none"}
```

## Branching / Git workflow

Use small commits. Prefer feature branches and pull requests if possible.

Suggested branch names:

```text
feature/build-fixes
feature/output-driver
feature/burst-fire-scheduler
feature/nvs-config
feature/hardware-pinout
```

## Do not do yet

Do not assume final high-voltage hardware pinout.
Do not drive mains-connected hardware without explicit pinout and safety review.
Do not remove safety defaults.
Do not replace ESP-IDF with Arduino unless explicitly requested.
Do not make the UI responsible for realtime control.

## Definition of done for first Paperclip pass

- Repo builds with `idf.py build`.
- `README.md` setup works.
- UART examples can be sent and status is returned.
- All outputs remain stubbed/off unless explicitly compiled in for test mode.
- Code is split into clear modules.
- TODOs are documented for hardware integration.
