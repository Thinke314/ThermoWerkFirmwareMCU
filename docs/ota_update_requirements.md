# OTA Update Requirements

## Purpose

ThermoWerk must support firmware updates without opening the enclosure. OTA must be designed from the beginning because the product is intended to be installed near electrical equipment and should be maintainable in the field.

## Current status

OTA is not yet implemented in code. This document defines the target behavior and implementation plan.

## Required ESP-IDF features

Use native ESP-IDF OTA mechanisms:

- `esp_https_ota`
- OTA partition table
- `otadata`
- `esp_ota_mark_app_valid_cancel_rollback`
- rollback support
- image validation

## Required partition table

Move to OTA-capable partitioning before first product test firmware:

```csv
# Name,    Type, SubType, Offset,   Size
nvs,       data, nvs,     0x9000,   0x6000
otadata,   data, ota,     0xf000,   0x2000
phy_init,  data, phy,     0x11000,  0x1000
ota_0,     app,  ota_0,   0x20000,  0x180000
ota_1,     app,  ota_1,             0x180000
storage,   data, spiffs,           0x100000
```

Exact sizes must be validated against real binary size and selected flash size.

## OTA state machine

```text
IDLE
  -> CHECK_AVAILABLE
  -> READY
  -> PREPARE_SAFE_STATE
  -> DOWNLOAD
  -> VERIFY
  -> SET_BOOT_PARTITION
  -> REBOOT_PENDING
  -> BOOT_SELF_TEST
  -> MARK_VALID
  -> DONE
```

Fault states:

```text
FAILED_NETWORK
FAILED_IMAGE
FAILED_VERIFY
FAILED_UNSAFE_STATE
FAILED_ROLLBACK
```

## Safety rules

Before OTA starts:

- force control mode disabled
- force SSR output OFF
- reject update if output is active
- reject update if emergency stop is active, unless update is a recovery update
- reject update if supply condition is unsafe once supply monitoring exists
- store previous mode/config if restart restore is allowed

During OTA:

- output must remain OFF
- control loop must not re-enable output
- UI must show progress
- watchdog must remain serviced

After OTA:

- boot into new image
- keep output OFF
- run self-test
- only then mark app valid
- if self-test fails, rollback

## Local UI requirements

The Settings / Update page must show:

- current firmware version
- build date/time
- chip model
- flash size if available
- current partition
- update source URL
- update progress
- last update result
- rollback availability

Actions:

- check for update
- start update
- reboot after update
- rollback if supported

## Local API requirements

Future endpoints:

```text
GET  /api/ota/status
POST /api/ota/check
POST /api/ota/start
POST /api/ota/reboot
POST /api/ota/rollback
```

Example status:

```json
{
  "state":"idle",
  "firmware_version":"0.1.0",
  "build":"dev",
  "running_partition":"factory",
  "rollback_available":false,
  "progress_percent":0,
  "last_error":"none"
}
```

## Cloud OTA requirements

Cloud can later provide metadata:

```json
{
  "version":"0.2.0",
  "url":"https://updates.example.com/thermowerk/0.2.0.bin",
  "sha256":"...",
  "mandatory":false,
  "notes":"Improved burst scheduler"
}
```

The ESP32 should not blindly install. It must verify:

- hardware compatibility
- version compatibility
- image integrity
- signature later

## Security requirements

Product firmware should evolve toward:

- HTTPS only
- signed firmware images
- secure boot
- flash encryption
- per-device credentials
- anti-rollback versioning

## Implementation plan

1. Add `main/ota_update.h` and `main/ota_update.c` as stubs.
2. Add OTA status struct.
3. Add local API status endpoint.
4. Change partition table to OTA layout.
5. Implement local URL OTA.
6. Add progress reporting.
7. Add rollback validation.
8. Add cloud metadata check.
9. Add signed firmware / secure boot plan.

## Definition of done

OTA milestone is done when:

- firmware builds with OTA partitions
- local UI shows OTA status
- local API exposes OTA status
- OTA from HTTPS URL works
- output is forced OFF during update
- failed update rolls back
- successful boot marks image valid
