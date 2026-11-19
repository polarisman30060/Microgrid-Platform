# Microgrid Communications Specification

Version: 1.0
Status: Draft overall. The configured Boost instance 0 communications profile is frozen for isolated simulator and compatible-firmware development only; all other profiles and project-wide behavior remain draft. This does not approve electrical behavior or production deployment.

## 1. Physical & Data-Link Layer

The Microgrid Communications Bus shall use:

- **Protocol Standard:** Classical CAN 2.0B (Non-FD). All STM32G474QE FDCAN controllers must be configured strictly in Classic CAN mode (`FDCAN_FRAME_CLASSIC`) to ensure 100% compatibility with the Coordinator's ESP32-S3 TWAI controller.
- **Identifier Format:** Standard 11-bit CAN Identifiers (`0x000`–`0x7FF`) for all real-time control, telemetry, discovery, and fault traffic. Extended 29-bit frames are unused for real-time operation in Revision 1.0 and reserved strictly for future offline firmware OTA or bulk diagnostics.
- **Nominal Bitrate:** `500 kbps` with galvanic isolation (`> 2.5 kVrms`) at every node and `120 Ohm` termination at both physical bus ends.
- **Maximum Payload:** `8 bytes` per frame (Little-Endian / Intel byte order).
- **Wire-Level Frame Definitions:** See [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md) for exact bitmasks, scaling, and payload layouts.

## 2. Priority-Based CAN Identifier Allocation

CAN identifiers are allocated by **message priority and function** so critical hardware faults and supervisory dispatch frames always win bus arbitration over routine telemetry:

- `0x001` – `0x01F`: **Highest Priority** — Emergency Hardware Faults (Event-driven)
- `0x100` – `0x11F`: **Very High Priority** — Coordinator Heartbeat & Global State (`100 ms`)
- `0x120` – `0x1FF`: **High Priority** — Supervisory Control Setpoints & Current Balancing Tokens (`20 ms`–`100 ms`)
- `0x200` – `0x2FF`: **Medium-High Priority** — Battery / BMS Node Telemetry (`50 ms` fast / `250 ms` slow)
- `0x300` – `0x3FF`: **Medium Priority** — HV Boost Converter Telemetry (`50 ms` fast / `250 ms` slow)
- `0x400` – `0x4FF`: **Medium Priority** — Renewable & Fuel Source Telemetry: Solar MPPT, Generator, Grid Interface (`50 ms`–`250 ms`)
- `0x500` – `0x5FF`: **Medium-Low Priority** — AC Output Inverter & Software Breaker Telemetry (`50 ms` fast / `250 ms` slow)
- `0x600` – `0x6FF`: **Low Priority** — Node Heartbeats & Local State Status (`100 ms`)
- `0x700` – `0x7FF`: **Lowest Priority** — Node Identity (Serial Numbers), Discovery, Configuration & Command Acknowledgements (`1000 ms` / On-demand)

## 3. Timing & Watchdog Hierarchy

Node heartbeats are transmitted every `100 ms`. A node that misses the Coordinator heartbeat for more than `500 ms` enters its locally defined degraded behavior. The frame format and detailed timing rules are specified in [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md).

CAN is supervisory communication only. Fast cycle-by-cycle current loops and hardware fault protection shall remain local to each power module and shall not depend on CAN timing or availability. Bulk bus capacitance does not replace local control or protection.

- Upon Coordinator timeout, active nodes transition to `DEGRADED_AUTONOMOUS` mode using their last valid operating limits:
  - **Battery Nodes:** Hold `96.0 VDC` Energy Bus regulation within autonomous V-I droop bounds; keep `30V 50kHz` Auxiliary Lifeline active.
  - **Solar MPPT Nodes:** Continue autonomous MPPT harvesting with local `96 VDC` overvoltage curtailment (`<= 100.8 VDC`).
  - **Multiple Boost Converter Systems:** A designated **Primary Boost Node** maintains `380.0 VDC` Inverter Bus regulation via local droop control, while **Secondary Boost Nodes** enter `Standby` so parallel converters do not fight over bus voltage without Coordinator current-balancing tokens.
  - **Inverter Nodes:** Maintain `60 Hz` AC output while `380 VDC` remains within valid bounds (`360 V`–`400 V`) and local `I²t` / short-circuit protection remains active.
  - **Generator & Grid Interface Nodes:** Safely ramp down power import/generation and isolate (`Generator` completes engine cooldown before shutdown; `Grid Interface` opens AC and DC contactors).

The configured Boost instance 0 wire profile is frozen for communications-simulator development; see [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md), Section 3.6.1. This is a node-specific exception, not approval of the remaining project-wide application protocol.
