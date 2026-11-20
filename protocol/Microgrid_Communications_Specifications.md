# Microgrid Communications Specification

Version: 1.1
Status: Working CAN ID registry and provisional traffic schedule approved for implementation. Solar gateway payloads, local emergency reporting, discovery/provisioning details, and electrical behavior remain draft. Boost 0 has been reopened; no profile is a production approval.

## 1. Physical & Data-Link Layer

The Microgrid Communications Bus shall use:

- **Protocol Standard:** Classical CAN 2.0B (Non-FD). All STM32G474QE FDCAN controllers must be configured strictly in Classic CAN mode (`FDCAN_FRAME_CLASSIC`) to ensure 100% compatibility with the Coordinator's ESP32-S3 TWAI controller.
- **Identifier Format:** Standard 11-bit CAN Identifiers (`0x000`–`0x7FF`) for all real-time control, telemetry, discovery, and fault traffic. Extended 29-bit frames are unused for real-time operation in Revision 1.0 and reserved strictly for future offline firmware OTA or bulk diagnostics.
- **Nominal Bitrate:** `500 kbps` with galvanic isolation (`> 2.5 kVrms`) at every node and `120 Ohm` termination at both physical bus ends.
- **Maximum Payload:** `8 bytes` per frame (Little-Endian / Intel byte order).
- **Wire-Level Frame Definitions:** See [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md) for exact bitmasks, scaling, and payload layouts.

## 2. Priority-Based CAN Identifier Allocation

CAN identifiers are allocated by **message priority and function** so critical hardware faults and supervisory dispatch frames always win bus arbitration over routine telemetry:

- `0x001` – `0x047`: **Highest Priority** — Main-bus Emergency Hardware Faults (`GlobalNodeID`)
- `0x048` – `0x0FF`: Reserved emergency identifiers
- `0x100` – `0x11F`: **Very High Priority** — Coordinator Heartbeat & Global State (`100 ms`)
- `0x120` – `0x167`: **High Priority** — Addressed main-bus control (`0x120 + GlobalNodeID`)
- `0x1A0` – `0x1AF`: **High Priority** — Solar gateway child-control routing (payload TBD)
- `0x1B0` – `0x1BF`: **High Priority** — Solar gateway routed replies (payload TBD)
- `0x200` – `0x2FF`: **Medium-High Priority** — Battery telemetry (`100 ms` fast / `1 s` slow)
- `0x300` – `0x3FF`: **Medium Priority** — Boost telemetry (`50 ms`)
- `0x400` – `0x4FF`: **Medium Priority** — Solar-local telemetry, Generator/Grid telemetry, Solar gateway summaries
- `0x500` – `0x5FF`: **Medium-Low Priority** — Inverter telemetry (`200 ms`)
- `0x600` – `0x6FF`: **Low Priority** — Node heartbeats (type-specific periods)
- `0x700` – `0x7FF`: **Lowest Priority** — Node identity, provisioning, and ACKs

The allocation table describes the main bus unless explicitly marked gateway-local. Solar child identifiers may overlap main-bus IDs because their segments are isolated; a Solar gateway aggregates/filters upstream and is not a transparent repeater.

## 3. Timing & Watchdog Hierarchy

The Coordinator heartbeat is `100 ms`. Node heartbeats use type-specific periods: Boost/Solar child `100 ms`, Battery/Inverter `250 ms`, Generator/Grid/Display `1 s`, and Solar gateway `100 ms`. Missing-node deadlines are Boost/Solar gateway `500 ms`, Battery/Inverter `1 s`, and Generator/Grid/Display `3 s`; Solar children are monitored by their gateway at `500 ms`. The frame format and detailed timing rules are specified in [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md).

CAN is supervisory communication only. Fast cycle-by-cycle current loops and hardware fault protection shall remain local to each power module and shall not depend on CAN timing or availability. Bulk bus capacitance does not replace local control or protection.

- Upon Coordinator timeout, active nodes transition to `DEGRADED_AUTONOMOUS` mode using their last valid operating limits:
  - **Battery Nodes:** Hold `96.0 VDC` Energy Bus regulation within autonomous V-I droop bounds; keep `30V 50kHz` Auxiliary Lifeline active.
  - **Solar MPPT Nodes:** Continue autonomous MPPT harvesting with local `96 VDC` overvoltage curtailment (`<= 100.8 VDC`).
  - **Multiple Boost Converter Systems:** A designated **Primary Boost Node** maintains `380.0 VDC` Inverter Bus regulation via local droop control, while **Secondary Boost Nodes** enter `Standby` so parallel converters do not fight over bus voltage without Coordinator current-balancing tokens.
  - **Inverter Nodes:** Maintain `60 Hz` AC output while `380 VDC` remains within valid bounds (`360 V`–`400 V`) and local `I²t` / short-circuit protection remains active.
  - **Generator & Grid Interface Nodes:** Safely ramp down power import/generation and isolate (`Generator` completes engine cooldown before shutdown; `Grid Interface` opens AC and DC contactors).

The working Boost 0 profile uses its registry-assigned IDs; see [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md), Section 3.6.1. Solar gateway payloads and physical gateway implementation remain open. The CAN channel remains supervisory; local control and protection must work without it.
