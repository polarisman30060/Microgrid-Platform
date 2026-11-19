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
- Assign or automatically change node addresses.
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

The current Coordinator implementation transmits `COORD_HEARTBEAT` and decodes the proposed Battery 0 identity, heartbeat, and telemetry frames. Bidirectional CAN with emulated ordinary module nodes is the development requirement; the Coordinator must not require a simulator-specific mode or protocol. The PC Battery emulator exists, but Coordinator command transmission and end-to-end validation remain unimplemented. The current project-wide timing target is a `100 ms` node/coordinator heartbeat and a `500 ms` timeout; those timings are not a substitute for completing the protocol review.

The Boost instance 0 wire profile is frozen only for isolated simulator and compatible-firmware development; see Section 3.6.1 of [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md). The current Coordinator does not yet decode Boost frames or transmit node commands. The following project-wide protocol questions remain implementation blockers:

- `EMERGENCY_FAULT` uses `0x001 + Node_Type`, so nodes of the same type share an identifier and may attempt simultaneous transmission. Define unique arbitration identifiers for faulting nodes.
- `CMD_ACK` uses `0x740 + GlobalNodeID`, which overlaps the fixed `0x780` remote-provisioning identifier for some node IDs. The frozen Boost 0 profile's `0x760` ACK is not affected.
- Define how an unconfigured node announces its identity before it has a `GlobalNodeID`, and clarify which node-ID values are valid for every node type.
- Specify whether a malformed command is acknowledged or dropped; the current CRC-error rule says both.
- Define command acknowledgement/retry and duplicate-command behavior before enabling state-changing commands.
- Reconcile payload definitions with the intended telemetry and command semantics, then publish one reviewed, versioned protocol as the implementation source of truth.

Until these items are resolved, firmware may develop and test CAN transport and the specifically frozen Boost 0 communications profile, but shall not generalize its command behavior to other nodes or treat illustrative examples as settled safety behavior. The Boost profile does not authorize or test physical power control. See [Coordinator CAN implementation guide](Coordinator_CAN_Implementation.md).

The PC Battery simulator uses ordinary Battery Node messages and remains a separate initial integration milestone. The Nucleo-G474RE Boost simulator shall use the frozen Boost 0 profile, ordinary CAN identifiers, and an external isolated CAN transceiver. Neither simulator shall actuate physical power hardware.

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

- Resolve and approve unique emergency identifiers, project-wide ACK/provisioning identifier overlap, unconfigured-node identity, and malformed-command handling before generalizing the CAN application layer beyond the frozen Boost 0 profile.
- Define the acknowledged command lifecycle, including retries, duplicate-command handling, and safe behavior when acknowledgements are lost.
- Define the persistence format, configuration migration policy, and behavior when the SD card is absent or fails.
- Define operator authorization and audit requirements for remote controls and fault resets.
- Confirm which Coordinator states and transition criteria are normative where architecture documents differ.
- Define the CPU 1 task priorities, stack sizes, and CAN/control scheduling budget when the supervisory control and CAN tasks are implemented.
