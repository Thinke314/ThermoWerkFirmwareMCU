# ThermoWerk MCU Firmware Specification

## Goal

Create the near-series ESP32 firmware for a PV-surplus heater controller / Leistungssteller. The ESP32 shall run the local control logic and safety logic independently. The external UI or main controller only provides setpoints, configuration and measured bus values over UART in the first implementation step.

## First implementation phase

The first firmware phase shall focus on the firmware architecture, communication interface and local control logic.

The hardware-specific parts are intentionally stubbed at first:

- SSR output driver
- zero-cross input capture
- temperature ADC / digital temperature sensors
- 3-phase topology detection
- relay / contactor driver
- hardware fault inputs

## Runtime responsibilities on ESP32

The ESP32 owns:

- control loop
- PV-surplus power calculation
- power limiting
- safety state machine
- timeout supervision
- output enable decision
- status reporting
- later: burst-fire scheduling synchronized to zero-cross detection

The host/UI owns:

- user interface
- high-level configuration
- optional Modbus polling from external meters or EMS
- sending process values to ESP32 over UART
- reading ESP32 status over UART

## Grid power convention

Use this sign convention:

- `grid_power_w > 0`: import from grid
- `grid_power_w < 0`: export / PV surplus

For PV-surplus mode, the first simple formula is:

```text
requested_power_w = -(grid_power_w - target_grid_power_w)
```

Example:

- grid_power_w = -1200 W
- target_grid_power_w = 0 W
- requested_power_w = 1200 W

## Control modes

### disabled

All outputs OFF.

### pv_surplus

Use surplus power based on received grid power and target grid power.

### manual_power

Use configured manual power setpoint.

### test

Low-power test mode. Must never be used as a replacement for safety validation.

## Safety behavior

Outputs must default to OFF.

Outputs must be OFF when:

- enable flag is false
- firmware is in disabled mode
- UART/process-value timeout occurs
- temperature limit is reached
- invalid configuration is received
- internal fault occurs
- later: zero-cross sync is missing
- later: hardware fault input is active

## First target hardware assumption

Use ESP32 or ESP32-S3.

Initial UART:

- UART0
- 115200 baud
- 8N1
- line-based JSON messages

No GPIO output is driven in the initial skeleton. Output generation must be implemented only after the power stage topology and pinout are fixed.

## Planned hardware abstraction layers

Recommended next files/modules:

```text
main/output_driver.c/.h
main/zero_cross.c/.h
main/temperature.c/.h
main/nvs_config.c/.h
main/topology.c/.h
```

## Output driver target behavior

Later implementation should convert `duty_permille` into a burst-fire/full-wave pattern.

Example for 1-phase 50 Hz:

- 50 full waves per second
- 1000 permille = all full waves ON
- 500 permille = approximately every second full wave ON
- 100 permille = 5 full waves per second ON

For SSRs with zero-cross switching, firmware should trigger SSR input before the desired full wave and release according to topology-specific timing.

## Important product constraints

This firmware is not safety-certified. For a sold product, independent hardware safety elements are required, for example:

- thermal cutoff
- fuse / overcurrent protection
- proper creepage/clearance on PCB
- enclosure design
- EMC strategy
- external certified safety limiter where required
- watchdog/failsafe output stage
