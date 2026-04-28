# ThermoWerk ESP32-S3 Implementation Roadmap

## Milestone 0: Build and flash baseline

Goal: firmware builds and flashes reliably.

Tasks:

- keep ESP-IDF project structure valid
- fix all compile errors
- verify ESP32-S3 target
- verify partition table
- verify local AP boots
- verify `/api/status`
- verify GPIO17 test output

Done when:

- `idf.py set-target esp32s3` works
- `idf.py build` works
- `idf.py flash monitor` works
- AP `ThermoWerk-Setup` starts

## Milestone 1: Product-ready local UI skeleton

Goal: replace engineering-looking page with polished local interface.

Tasks:

- redesign embedded HTML/CSS
- card-based mobile UI
- home dashboard
- control section
- simulated input section
- cloud section
- fault section
- history section
- update section placeholder

Done when:

- UI is usable on phone
- no external web assets required
- all controls show feedback
- fault state is understandable

## Milestone 2: OTA stubs and API

Goal: OTA visible in firmware architecture without yet requiring production OTA.

Tasks:

- add `ota_update.c/.h`
- add OTA status struct
- add `GET /api/ota/status`
- add `POST /api/ota/start` stub
- add `POST /api/ota/rollback` stub
- show OTA state in UI
- document OTA partition migration

Done when:

- OTA API returns JSON
- UI shows firmware/update section
- output remains OFF if OTA start is requested

## Milestone 3: NVS persistent config

Goal: settings survive reboot.

Tasks:

- add `nvs_config.c/.h`
- persist core config
- persist Wi-Fi settings later
- persist cloud endpoint
- persist device name
- add factory reset endpoint

Done when:

- config can be changed in UI
- reboot keeps config
- factory reset restores defaults

## Milestone 4: Wi-Fi station setup

Goal: device can join existing Wi-Fi while keeping fallback AP.

Tasks:

- add scan endpoint
- add connect endpoint
- store credentials in NVS
- AP fallback if STA fails
- show IP address in UI

Done when:

- phone can configure Wi-Fi
- device joins router
- local UI remains reachable

## Milestone 5: Real sensor and meter integration

Goal: stop relying on simulated inputs for real installation.

Tasks:

- temperature driver abstraction
- Modbus TCP client
- Modbus RTU option if hardware supports RS485
- meter freshness supervision
- sensor fault handling

Done when:

- grid power comes from real meter
- temperatures come from real sensors or external bus
- stale values force safe state

## Milestone 6: Zero-cross and hardware safety

Goal: make output stage more production-like.

Tasks:

- add zero-cross input module
- sync output to mains wave
- detect missing zero-cross
- add hardware fault input
- add optional contactor/relay output

Done when:

- zero-cross loss disables output
- hardware fault disables output
- SSR command is deterministic

## Milestone 7: Real OTA

Goal: firmware updates from local/cloud URL.

Tasks:

- move partition table to OTA layout
- implement HTTPS OTA
- implement progress reporting
- implement rollback validation
- mark firmware valid after self-test
- add version metadata

Done when:

- firmware updates successfully from URL
- failed image does not brick device
- rollback works

## Milestone 8: Cloud product layer

Goal: support fleet telemetry and remote support.

Tasks:

- cloud device identity
- telemetry schema
- cloud health status
- remote support mode
- OTA metadata endpoint
- cloud command safety path if required

Done when:

- cloud observes device
- cloud failure does not affect local control
- OTA metadata can be checked

## Milestone 9: Production hardening

Goal: product-readiness path.

Tasks:

- watchdog strategy
- brownout behavior
- boot self-test
- logs and fault counters
- manufacturing test mode
- security review
- secure boot plan
- flash encryption plan
- EMC/safety documentation hooks

Done when:

- device has predictable failure behavior
- production test procedure exists
- safety-relevant limitations are documented
