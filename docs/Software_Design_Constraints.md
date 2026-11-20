# Software Design Constraints

Status: Draft

## Purpose

This document records the software-level boundaries and constraints for the Microgrid Platform. It guides the Coordinator firmware now and provides shared constraints for future module firmware. It does not replace node definitions, hardware specifications, the system state machine, or protocol documents.

## Design Philosophy

- Local modules own immediate electrical and thermal safety; the Coordinator owns system-level coordination and resource allocation.
- Communication loss, Coordinator reboot, or HMI failure shall not disable a module's local protections.
- Software shall make operating state and failures observable. It shall not report successful initialization or operation after a required step has failed.
- Firmware behavior shall be traceable to an agreed project requirement; unresolved requirements must remain explicit rather than being silently guessed.

## Scope and Responsibilities

### Coordinator firmware

The Coordinator may:

- Discover and monitor nodes, compare them with the commissioned topology, and report missing or conflicting nodes.
- Sequence supervisory operating states and issue bounded system-level commands.
- Aggregate telemetry, present system status, and record events and faults.
- Provide local and remote operator interfaces, subject to the same system-state and command interlocks.

The Coordinator shall not:

- Perform PWM generation, fast voltage/current regulation, or module-level protection.
- Override a module's local FAULT or LOCKOUT decision.
- Automatically assign or change node addresses; explicit commissioning-time provisioning may be added when its protocol and authorization rules are defined.
- Assume a CAN command was acted on without an acknowledgement or confirming telemetry where the protocol requires confirmation.

### Module firmware

Each power module retains authority over its local control loops, hardware trips, contactors, and transition to FAULT or LOCKOUT. Coordinator commands are supervisory requests and shall not weaken local protection limits.

## System Constraints

### State and startup

- Coordinator state transitions shall follow the agreed system state machine. Invalid or unavailable inputs shall not be treated as evidence that a prerequisite is satisfied.
- Startup shall remain inhibited when a required node is absent, unhealthy, or implicated in a duplicate address conflict.
- Bus energization and downstream enablement shall follow the documented upstream-to-downstream sequence and module-local precharge interlocks.
- FAULT and LOCKOUT handling shall preserve the auxiliary control-power lifeline where the hardware supports it.
- Remote HMI requests shall not bypass the same state, fault, and topology checks applied to local requests.

### Failure handling

- Loss of the Coordinator shall leave module-local safety functions active. Modules shall follow the agreed degraded-mode behavior using only valid, bounded prior settings.
- Loss of a required capability, invalid configuration, communication failure, or persistence failure shall be visible to the operator and represented in system status.
- Software shall not automatically clear latched faults, reassign addresses, or retry hazardous actions without an explicit requirement and defined limits.
- No single software failure shall be assumed to provide the emergency shutdown function; that function belongs to local hardware and module control.

### Data and persistence

- Commissioned topology and operating configuration shall be validated before use. Corrupt, incompatible, or missing required configuration shall inhibit dependent operation and be reported.
- Events and faults should include a monotonic time or sequence reference when a trustworthy wall clock is unavailable.
- Persistent logging shall not block time-critical control or communication work.
- Telemetry values shall retain their defined units, sign conventions, validity, and freshness; stale or invalid measurements shall not be presented as current.

## Communications Contract Gate

The current Coordinator implementation transmits `COORD_HEARTBEAT` and decodes Battery 0 identity, heartbeat, and telemetry using the working registry IDs. Bidirectional CAN with emulated ordinary module nodes is the development requirement; the Coordinator must not require a simulator-specific mode or protocol. Coordinator command transmission and end-to-end validation remain unimplemented. The provisional schedule uses a `100 ms` Coordinator heartbeat, type-specific node heartbeat periods, and type-specific missing-node deadlines.

Boost instance 0 has been reopened to use the working registry; see Section 3.6.1 of [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md). The current Coordinator does not yet decode Boost frames or transmit node commands. The following protocol questions remain implementation blockers:

- Define the compact Solar gateway child-control payload, routed reply/ACK semantics, and command correlation for IDs `0x1A0`–`0x1BF`.
- Define Solar gateway summary and child-detail payloads for IDs `0x430`–`0x45F`.
- Define local Solar child emergency reporting and gateway forwarding payloads.
- Define unconfigured Solar child discovery and how provisioning (`0x7C0`) reaches the correct gateway.
- Define command acknowledgement/retry and duplicate-command behavior before enabling state-changing commands.
- Validate the provisional bus load against the 500-Solar-node target and real payload sizes.
- Define and validate the parallel Inverter `20 ms` current-sharing profile, including feedback freshness, missed-update behavior, and bus load before implementing it. Non-paralleled Inverters do not require that update rate.

The main-bus ID registry and node quotas are approved working assignments; unresolved gateway payloads do not invalidate those assignments. Firmware may develop and test transport and node profiles, but shall not treat illustrative payloads or behaviors as settled safety requirements. The registry does not authorize or test physical power control. See [Coordinator CAN implementation guide](Coordinator_CAN_Implementation.md).

The PC Battery simulator uses ordinary Battery Node messages and remains a separate initial integration milestone. The Nucleo-G474RE Boost simulator uses the working Boost 0 registry profile and an external isolated CAN transceiver. Solar child frames belong on isolated gateway-local segments. Neither simulator shall actuate physical power hardware.

## Implementation Constraints

- Coordinator firmware targets the Waveshare ESP32-S3-Touch-LCD-7 Rev 1.2 and the ESP-IDF-based project under `firmware/coordinator_module/`.
- On the dual-core ESP32-S3, CPU 0 owns the LCD/touch UI, Wi-Fi driver, TCP/IP, event loop, and HTTP server. Coordinator state processing, CAN communications, and system monitoring shall run in dedicated CPU 1 task(s); avoid unpinned application tasks for either workload.
- CPU 1 logic shall not call LVGL or perform blocking network, display, or storage work. Cross-core exchange shall use bounded queues or other synchronization with defined ownership.
- ESP-IDF and hardware system services may still use either core as required; the affinity rule applies to application workloads and configured network/display tasks.
- Hardware initialization shall respect board-specific shared-pin routing, including selecting the CAN path before using the TWAI interface, as specified by `hardware/System_Coordinator_Hardware.md`.
- CAN runs at 500 kbit/s on CPU 1. Coordinator heartbeat TX and Battery 0 RX/decoding are implemented as the first firmware slice. The PC Battery Node emulator exists, and the standalone Nucleo Boost 0 project builds; physical Nucleo flashing, isolated-bus tests, and Coordinator support for Boost RX and command/ACK exchange remain pending. Simulators must not control real power hardware.
- Wi-Fi credentials entered through the LCD shall be persisted only after a successful connection. Passwords shall be masked on screen and must not be logged.
- Production deployment shall define NVS/flash encryption for stored Wi-Fi credentials before the device is exposed to physical access.
- The LCD shall report Wi-Fi connection state and the web-server address so initial setup can continue from a browser on the same network.
- The static dashboard in `firmware/coordinator_module/web/` is packaged into the `web_a` SPIFFS partition and included in a normal PlatformIO upload. `web_b` is reserved for a future asset-update workflow; web and firmware OTA updates remain unimplemented.
- Separate hardware drivers, protocol encoding/decoding, supervisory logic, persistence, and HMI presentation sufficiently to test supervisory behavior without energizing hardware.
- Communication, storage, and UI work shall not block local event handling or periodic supervisory tasks indefinitely. Timeouts and bounded queues shall be used where applicable.
- Resource use shall account for the display framebuffer and other PSRAM consumers; allocation and peripheral initialization failures shall be reported explicitly.
- Credentials and private keys shall not be committed to the repository or exposed in logs.

## Verification Expectations

- Build the firmware for its configured target after changes to the Coordinator project.
- Test state transitions, startup interlocks, timeout handling, malformed or stale inputs, and failure paths without requiring live power hardware.
- Verify CAN payload encoding and decoding against the reconciled protocol definitions, including boundary values and invalid frames.
- Commands may be exercised against the PC or Nucleo module emulators only on an isolated communications bench with no power hardware attached. Hardware-in-the-loop tests and safety review are required before using command-capable Coordinator firmware on a real power system.

## Open Questions

- Define Solar gateway payloads, local emergency reporting, provisioning discovery, and malformed-command handling before implementing gateway application behavior.
- Define the acknowledged command lifecycle, including retries, duplicate-command handling, and safe behavior when acknowledgements are lost.
- Define the persistence format, configuration migration policy, and behavior when the SD card is absent or fails.
- Define operator authorization and audit requirements for remote controls and fault resets.
- Confirm which Coordinator states and transition criteria are normative where architecture documents differ.
- Define the CPU 1 task priorities, stack sizes, and CAN/control scheduling budget when the supervisory control and CAN tasks are implemented.
