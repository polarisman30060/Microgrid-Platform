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

This is an initial full-duplex Coordinator slice, not a complete Coordinator CAN stack. It can transmit the coordinator heartbeat and receive the proposed Battery 0 profile, but has not yet been tested against the PC simulator. The simulator must not connect to or control real power hardware.

The standalone Nucleo-G474RE Boost 0 simulator has been added under `firmware/nucleo_boost_simulator/` and its PlatformIO build succeeds. No Nucleo/ST-LINK was connected for flashing, and Coordinator Boost decoding/command handling remains unimplemented.

## Protocol authority and blockers

The proposed protocol is in [`../protocol/Microgrid_Communications_Specifications.md`](../protocol/Microgrid_Communications_Specifications.md) and [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md). The project timing target is a `100 ms` heartbeat and `500 ms` timeout. The configured Boost 0 communications profile is frozen for isolated simulator and compatible-firmware development only; its IDs and limited command/ACK rules are in CAN definitions Section 3.6.1. This does not freeze other Boost instances or the project-wide protocol.

The initial Battery 0 profile remains proposed: `GlobalNodeID = 0x10`, telemetry IDs `0x200`/`0x210`, heartbeat `0x610`, identity `0x710`, command `0x130`, and acknowledgement `0x750`. Do not invent simulator-specific identifiers. The current Coordinator implementation still only decodes Battery 0 frames and transmits its heartbeat; it does not yet transmit commands or decode Boost frames. Resolve the remaining system-wide protocol issues in [`Software_Design_Constraints.md`](Software_Design_Constraints.md) before enabling other node profiles or production application behavior.

The PC simulator must send and receive the same frames a Battery Node would use and respond according to its simulated state. Never connect this software model to equipment capable of energizing a real bus.

## Initial Coordinator V0 design

The first firmware increment implements a single configured Battery Node on the normal microgrid CAN protocol. It does not implement a simulator mode, automatic address assignment, multi-node dispatch, or autonomous power startup.

### V0 node profile

| Frame | CAN ID | Direction | Period / handling |
| --- | --- | --- | --- |
| `COORD_HEARTBEAT` | `0x100` | Coordinator → bus | Every `100 ms` |
| `NODE_HEARTBEAT` | `0x610` | Battery 0 → bus | Every `100 ms` |
| `NODE_IDENTITY` | `0x710` | Battery 0 → bus | Every `1000 ms` and at startup |
| `TELEM_BATTERY_FAST` | `0x200` | Battery 0 → bus | Every `50 ms` |
| `TELEM_BATTERY_SLOW` | `0x210` | Battery 0 → bus | Every `250 ms` |
| `CMD_NODE_CONTROL` | `0x130` | Coordinator → Battery 0 | On explicit request only |
| `CMD_ACK` | `0x750` | Battery 0 → bus | Response to a state-changing command |

These IDs follow the current draft's `Battery`, `Instance = 0`, `GlobalNodeID = 0x10` assignment. All frames use Classical CAN, standard 11-bit identifiers, and a maximum DLC of 8. The PC emulator models the battery state; it must not operate physical contactors or power stages.

### Coordinator runtime

1. Initialize the Waveshare board so CH422G `EXIO5` routes GPIO 19/20 to CAN, then start the TWAI controller at `500 kbit/s`.
2. Keep CAN interrupt/callback work bounded: receive a frame into a fixed-size queue, record queue overflow, and return. A CPU 1 CAN service task validates and dispatches frames.
3. Use a single application-owned node registry for the configured Battery 0. Store its serial, firmware/build ID, reported state, decoded battery measurements, and per-frame last-seen times. Publish a bounded status snapshot for the HMI; the CAN task never calls LVGL.
4. Transmit `COORD_HEARTBEAT` every `100 ms` using a rolling counter. Start in `DISCOVERY`; transition to `READY` after the configured node's identity is valid and its heartbeat reports `READY`. If the required battery is not discovered within the provisional `3 s` deadline, report it missing and enter `FAULT`. Keep all fleet/power enable flags clear in V0.
5. Mark the battery offline after `500 ms` without its heartbeat. Mark fast/slow telemetry stale after three missed periods (`150 ms` / `750 ms`); stale measurements are not reused as current values.
6. Accept only the configured Battery 0 identity and its expected frame IDs. Validate standard/extended format, data-frame type, exact DLC, node type/instance, and enum ranges. Count and surface invalid, unexpected, stale, and dropped frames.
7. Implement `CMD_NODE_CONTROL` encoding with its specified CRC-8 and `CMD_ACK` validation for the Battery profile. Allow only one state-changing command in flight; match its ACK by source ID and echoed action, and apply a bounded timeout. Transmit a state-changing command only after an explicit request and a valid Battery `READY`/fault-state check. Do not automatically precharge, close contactors, or enable power stages in V0. A missing or rejected ACK is an explicit command failure, never success.
8. Keep Coordinator TX and RX full duplex. The simulator listens to the Coordinator heartbeat, sends ordinary Battery frames, and handles ordinary targeted commands. No simulator-only frame, address, or Coordinator behavior is introduced.

The `3 s` discovery deadline and three-period telemetry freshness thresholds are initial software defaults, not ratified system-wide requirements. Keep them localized and covered by tests so they can be revised easily.

## Implementation sequence

1. **Keep transport isolated.** Preserve CAN driver setup, RX queueing, TX submission, and status reporting independently of application logic. Report initialization failures, TX failures, and queue drops; do not silently treat an unavailable interface as healthy.
2. **Freeze the Battery 0 profile.** Review the V0 IDs, payload fields, scaling, state transitions, CRC behavior, and ACK outcomes against the protocol draft.
3. **Implement full-duplex Coordinator handling.** Add the bounded TX path, periodic coordinator heartbeat, RX validation, Battery 0 codecs, registry, and explicit command/ACK lifecycle.
4. **Build the PC Battery Node simulator.** Connect the Waveshare USB-CAN-A to the same isolated CAN bus as the Coordinator. Send ordinary Battery 0 frames, receive Coordinator frames, and respond according to simulated battery state.
5. **Prove the two-way node lifecycle.** Verify startup discovery, telemetry updates, explicit Coordinator commands, acknowledgements, heartbeat timeout, simulated node loss, and recovery. The Coordinator must process the simulated node like a real one.
6. **Resolve the full wire contract.** Before generalizing beyond the frozen Boost 0 profile, resolve unique emergency IDs, the project-wide `0x740+GlobalNodeID` / `0x780` overlap, unconfigured-node identity, malformed-command acknowledgement behavior, retries, and version compatibility.
7. **Build the Nucleo Boost 0 simulator.** The standalone Nucleo-G474RE PlatformIO firmware now builds. Flashing and bench verification remain pending; use an external isolated CAN transceiver and never configure or drive power-stage outputs.
8. **Add Boost 0 Coordinator support and verify end-to-end.** Decode the simulator's frozen telemetry, heartbeat, and identity frames and implement bounded command/ACK handling. Keep all power-enable flags clear; this is a communications test, not permission to control a power stage. Verify on an isolated CAN bench.
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
