# Coordinator CAN Implementation Guide

## Purpose and status

This guide maps the CAN protocol work to the existing ESP-IDF Coordinator firmware. The development target is bidirectional communication with a PC program that emulates normal microgrid CAN nodes through the Waveshare USB-CAN-A adapter. The Coordinator must not need a simulator-specific mode or message set. Start with one simulated Battery Node. CAN is a supervisory channel; node-local control and protection must continue to work without the Coordinator.

## Current implementation

The Coordinator firmware is under `firmware/coordinator_module/`.

- `src/can_bus.c` starts an ESP32-S3 TWAI node in Classical CAN mode at `500 kbit/s` with TX GPIO 20 and RX GPIO 19 configured.
- `waveshare_rgb_lcd_init()` sets the CH422G output to `0x3E`, selecting the CAN route while enabling the LCD backlight.
- A receive callback copies frames into a bounded FreeRTOS queue. A CPU 1 service task sends `COORD_HEARTBEAT` every `100 ms`, decodes Battery 0 identity/heartbeat/fast/slow telemetry, tracks freshness, and handles TWAI bus-off recovery.
- `src/main.c` displays transport status and Battery 0 state/measurements when present; otherwise it shows raw received-frame diagnostics.
- `app_can_status_t` exposes transport counters and a synchronized snapshot of Battery 0 telemetry. Battery commands, ACK tracking, multi-node dispatch, persistence, and full supervisory behavior are not implemented.

This is an initial full-duplex Coordinator slice, not a complete Coordinator CAN stack. It can transmit the coordinator heartbeat and receive Battery 0 using the working registry IDs, but has not yet been tested against the PC simulator. The simulator must not connect to or control real power hardware.

The Nucleo-G474RE simulator projects are grouped under `firmware/nucleo_simulators/`, with a separate PlatformIO project for Boost 0, Battery 0, Solar 0, Generator 0, Grid Interface 0, and Inverter 0. Boost 0 has been reopened and all projects use the current registry draft. No Nucleo/ST-LINK was connected for flashing, and Coordinator support for these nodes remains unimplemented.

## Protocol authority and blockers

The working registry and provisional schedule are in [`../protocol/Microgrid_Communications_Specifications.md`](../protocol/Microgrid_Communications_Specifications.md) and [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md). The Coordinator heartbeat is `100 ms`; missing-node deadlines vary by node type. Boost 0 was reopened and now uses the registry-assigned IDs in CAN definitions Section 3.6.1. Gateway payloads and physical gateway implementation remain open.

Battery 0 uses `GlobalNodeID = 0x01`, telemetry IDs `0x200`/`0x210`, heartbeat `0x601`, identity `0x701`, command `0x121`, and acknowledgement `0x760`. Do not invent simulator-specific identifiers. The current Coordinator implementation still only decodes Battery 0 frames and transmits its heartbeat; it does not yet transmit commands or decode Boost frames. Gateway payloads and other open protocol details must be resolved before implementing Solar gateway behavior or production application behavior.

The PC simulator must send and receive the same frames a Battery Node would use and respond according to its simulated state. Never connect this software model to equipment capable of energizing a real bus.

## Initial Coordinator V0 design

The first firmware increment implements a single configured Battery Node on the normal microgrid CAN protocol. It does not implement a simulator mode, automatic address assignment, multi-node dispatch, or autonomous power startup.

### V0 node profile

| Frame | CAN ID | Direction | Period / handling |
| --- | --- | --- | --- |
| `COORD_HEARTBEAT` | `0x100` | Coordinator → bus | Every `100 ms` |
| `NODE_HEARTBEAT` | `0x601` | Battery 0 → bus | Every `250 ms` |
| `NODE_IDENTITY` | `0x701` | Battery 0 → bus | At startup/on request |
| `TELEM_BATTERY_FAST` | `0x200` | Battery 0 → bus | Every `100 ms` |
| `TELEM_BATTERY_SLOW` | `0x210` | Battery 0 → bus | Every `1 s` |
| `CMD_NODE_CONTROL` | `0x121` | Coordinator → Battery 0 | On change/explicit request |
| `CMD_ACK` | `0x760` | Battery 0 → bus | Response to a state-changing command |

These IDs follow the working registry's Battery instance 0 assignment. All frames use Classical CAN, standard 11-bit identifiers, and a maximum DLC of 8. The PC emulator models the battery state; it must not operate physical contactors or power stages.

### Coordinator runtime

1. Initialize the Waveshare board so CH422G `EXIO5` routes GPIO 19/20 to CAN, then start the TWAI controller at `500 kbit/s`.
2. Keep CAN interrupt/callback work bounded: receive a frame into a fixed-size queue, record queue overflow, and return. A CPU 1 CAN service task validates and dispatches frames.
3. Use a single application-owned node registry for the configured Battery 0. Store its serial, firmware/build ID, reported state, decoded battery measurements, and per-frame last-seen times. Publish a bounded status snapshot for the HMI; the CAN task never calls LVGL.
4. Transmit `COORD_HEARTBEAT` every `100 ms` using a rolling counter. Start in `DISCOVERY`; transition to `READY` after the configured node's identity is valid and its heartbeat reports `READY`. If the required battery is not discovered within the provisional `3 s` deadline, report it missing and enter `FAULT`. Keep all fleet/power enable flags clear in V0.
5. Mark the battery offline after `1 s` without its heartbeat. Mark fast/slow telemetry stale after three missed periods (`300 ms` / `3 s`); stale measurements are not reused as current values.
6. Accept only the configured Battery 0 identity and its expected frame IDs. Validate standard/extended format, data-frame type, exact DLC, node type/instance, and enum ranges. Count and surface invalid, unexpected, stale, and dropped frames.
7. Implement `CMD_NODE_CONTROL` encoding with its specified CRC-8 and `CMD_ACK` validation for the Battery profile. Allow only one state-changing command in flight; match its ACK by source ID and echoed action, and apply a bounded timeout. Transmit a state-changing command only after an explicit request and a valid Battery `READY`/fault-state check. Do not automatically precharge, close contactors, or enable power stages in V0. A missing or rejected ACK is an explicit command failure, never success.
8. Keep Coordinator TX and RX full duplex. The simulator listens to the Coordinator heartbeat, sends ordinary Battery frames, and handles ordinary targeted commands. No simulator-only frame, address, or Coordinator behavior is introduced.

The `3 s` discovery deadline and three-period telemetry freshness thresholds are initial software defaults, not ratified system-wide requirements. Keep them localized and covered by tests so they can be revised easily.

## Implementation sequence

1. **Keep transport isolated.** Preserve CAN driver setup, RX queueing, TX submission, and status reporting independently of application logic. Report initialization failures, TX failures, and queue drops; do not silently treat an unavailable interface as healthy.
2. **Validate the Battery 0 profile.** Review its working IDs, payload fields, scaling, state transitions, CRC behavior, and ACK outcomes against the protocol definition.
3. **Implement full-duplex Coordinator handling.** Add the bounded TX path, periodic coordinator heartbeat, RX validation, Battery 0 codecs, registry, and explicit command/ACK lifecycle.
4. **Build the PC Battery Node simulator.** Connect the Waveshare USB-CAN-A to the same isolated CAN bus as the Coordinator. Send ordinary Battery 0 frames, receive Coordinator frames, and respond according to simulated battery state.
5. **Prove the two-way node lifecycle.** Verify startup discovery, telemetry updates, explicit Coordinator commands, acknowledgements, heartbeat timeout, simulated node loss, and recovery. The Coordinator must process the simulated node like a real one.
6. **Resolve remaining wire details.** Define gateway child-control/reply and summary/detail payloads, local Solar emergency reporting, unconfigured Solar discovery/provisioning, malformed-command acknowledgement behavior, retries, and version compatibility. The main-bus ID collisions have been removed by the working registry; Solar child frames remain isolated from the main bus.
7. **Build the Nucleo Boost 0 simulator.** The standalone Nucleo-G474RE PlatformIO firmware builds with the working registry. Flashing and bench verification remain pending; use an external isolated CAN transceiver and never configure or drive power-stage outputs.
8. **Add Boost 0 Coordinator support and verify end-to-end.** Decode the simulator's telemetry, heartbeat, and identity frames and implement bounded command/ACK handling. Keep all power-enable flags clear; this is a communications test, not permission to control a power stage. Verify on an isolated CAN bench.
9. **Connect presentation last.** Publish bounded snapshots or events to the UI. CAN/control tasks must not call LVGL or block on Wi-Fi, display, or storage work; follow the core ownership constraints in [`Software_Design_Constraints.md`](Software_Design_Constraints.md).

## Verification gates

- Build the configured PlatformIO target after firmware changes.
- On an isolated bench CAN bus, verify the simulated Battery Node's normal bidirectional frame lifecycle, timeout, node loss, and recovery. Confirm the Coordinator requires no simulator-specific code path.
- Unit-test each approved codec with valid boundary values, invalid DLC/ID, invalid enum values, and corrupted integrity bytes where applicable.
- Test RX queue saturation, initialization failure, stale telemetry, missing heartbeat, duplicate identity, and recovery without energized power hardware.
- Compare simulator-produced frames and parsed Coordinator state against the same approved protocol fixtures.
- Exercise Battery Node command frames against the PC emulator only on an isolated CAN bench with no power hardware attached. This verifies communications and Coordinator logic, not contactor operation or electrical safety.
- Require separate hardware-in-the-loop testing and safety review before connecting command-capable Coordinator firmware to real power hardware.

## Out of scope

The Coordinator does not run converter switching loops, provide hardware overcurrent protection, or replace local module interlocks. CAN loss must not disable module-local protection. PC and Nucleo emulators never operate physical contactors or power stages.

## Waveshare USB-CAN-A PC interface

Waveshare documents the USB-CAN-A as a **USB-to-serial-to-CAN** adapter. On Windows, install the CH341SER driver and access the adapter through its enumerated COM port; do not treat it as a native PC CAN interface. Keep the USB serial settings distinct from the CAN bus settings:

| Setting | Initial value |
| --- | --- |
| USB serial | `2,000,000` baud, 8 data bits, no parity, 1 stop bit |
| CAN bus | `500 kbit/s`, standard 11-bit frames, normal mode |
| Serial-to-CAN framing | Variable-length protocol (recommended initial choice) |

Waveshare also supports a fixed 20-byte serial protocol. The two serial framing modes are mutually exclusive and the adapter configuration and PC parser must agree. In variable-length mode, a packet uses `0xAA` as its header and `0x55` as its terminator; the type byte encodes standard/extended, data/remote, and DLC, followed by the CAN ID and data. Implement a streaming parser because serial reads need not align to packet boundaries. See Waveshare's [USB-CAN-A documentation](https://www.waveshare.com/wiki/USB-CAN-A) and [serial-to-CAN protocol definition](https://www.waveshare.com/wiki/Secondary_Development_Serial_Conversion_Definition_of_CAN_Protocol) for the full packet formats and configuration commands.
