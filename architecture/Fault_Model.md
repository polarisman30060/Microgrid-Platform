# System Fault Model

Status: Draft v0.1 — defines architectural fault severity tiers, protection ownership, fault lifecycles, and recovery boundaries. Concrete wire-level fault codes and bitmasks are defined in [`../protocol/Fault_Codes.md`](../protocol/Fault_Codes.md) and [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md).

---

## Purpose

This document defines how electrical, thermal, mechanical, communication, and configuration faults are classified, contained, reported, and cleared across the Microgrid Platform so that local module safety, Coordinator supervision, and operator recovery follow one consistent contract.

---

## Design Philosophy

- **Local Modules Own Immediate Safety:** Hardware comparators, `HRTIM1` fault inputs, and local module firmware always act first to protect power stages, batteries, and wiring without waiting for Coordinator permission or CAN availability.
- **Fail-Safe Physical Isolation:** Any unsafe power-stage condition terminates PWM immediately and opens the module's physical isolation contactor(s) (`Q_ISOL`, and AC contactors where fitted).
- **Preserve the Control Lifeline:** A local module fault or E-Stop shall never shut down `AUX_LOGIC_24V` (the un-interruptible side of the Split Diode-OR'd `24 VDC` auxiliary bus) so the Coordinator and surviving nodes remain online to log and diagnose the event. (`AUX_COIL_24V` drops immediately on E-Stop/GFDI.)
- **No Silent Failures or Blind Retries:** Every fault is observable in local state, CAN telemetry/fault reporting, and persistent Coordinator logs. Latched hazardous faults never auto-reclose onto a potential short circuit.
- **Graceful Degradation Over Total Blackout:** Loss of an `Optional` node or loss of Coordinator communications transitions the system to bounded `DEGRADED` operation rather than tripping healthy critical loads.

---

## Assumptions

- Every power-stage host board implements hardware default-off interlocks so loss of control-card power, MCU reset, or card removal forces gate drivers and contactor coils to a safe de-energized state.
- Every main-bus node has a unique commissioned `GlobalNodeID` (`0x01–0x47`), and every Solar child operates behind an isolated Solar Segment Gateway (`slot 0–31`).
- The Coordinator maintains a commissioned topology classifying each expected node as `Required`, `Optional`, or `Ignored`.

---

## Constraints

- Local hardware trips (`Layer 0`) shall inhibit switching independently of CPU execution, RTOS scheduling, CAN traffic, and HMI operation.
- The Coordinator shall never override or mask a module's local `FAULT` or `LOCKOUT` state.
- A module shall not accept a `Run` or `Precharge` command while an active fault or lockout condition remains uncleared.
- Automatic retry after a transient `FAULT` shall be strictly bounded (count and cooldown window) and shall escalate to `LOCKOUT` if repeated trips occur.
- No contactor shall close across an uncharged bus (`ΔV > 1.5 V` precharge interlock) during fault recovery.

---

## Requirements

### 1. Fault Severity Tiers

| Tier | Local Node State | Electrical Action | Reporting Mechanism | Recovery Policy |
| :--- | :--- | :--- | :--- | :--- |
| **Tier 0: Warning / Derate** | `RUNNING` or `DEGRADED` | Continue operating with local thermal, current, or droop derating. | `NODE_HEARTBEAT` Byte 3 (`Active_Warning_Flags`) + slow telemetry diagnostics. | Clears automatically once measurement returns inside hysteresis band. |
| **Tier 1: Recoverable Fault** | `FAULT` (`0x05`) | Gate off PWM immediately; open `Q_ISOL` (and AC disconnect if applicable); preserve auxiliary lifeline. | Immediate `EMERGENCY_FAULT` broadcast (`GlobalNodeID`, `Fault_State = 0x05`) + `NODE_HEARTBEAT` state update. | Returns to `READY` only after fault cause clears and either bounded cooldown elapses (for approved transient faults) or Coordinator sends `Clear Recoverable Faults` / `CMD_NODE_CONTROL (0x05)`. |
| **Tier 2: Latched Lockout** | `LOCKOUT` (`0x06`) | Gate off PWM via hardware latch; open all power contactors immediately; block all auto-restart. | Immediate `EMERGENCY_FAULT` broadcast (`GlobalNodeID`, `Fault_State = 0x06`) + persistent `NODE_HEARTBEAT` `LOCKOUT`. | Requires explicit operator inspection and confirmed reset command (`CMD_NODE_CONTROL 0x05` after interlock verification) or physical service/power cycle. |

### 2. Protection Layer Ownership

| Layer | Domain | Typical Triggers | Mandatory Outcome |
| :--- | :--- | :--- | :--- |
| **Layer 0: Hardware Interlock** | Host board comparators, gate-driver desat/fault, current-sensor OC, thermal snap discs, `HRTIM1_FLT` | Hard short circuit, phase overcurrent, bus overvoltage crowbar, card removal/power loss | Asynchronous hardware PWM shutdown (`< 1 µs` target), hardware latch, contactor drop-out. |
| **Layer 1/2: Module Firmware** | Local STM32G474 MCU | Sustained overvoltage/undervoltage, `I²t` thermal overload, precharge timeout, contactor mirror mismatch, BMS critical alarm, engine overspeed/oil loss, Coordinator timeout (`> 500 ms`) | Immediate local state transition (`DEGRADED`, `FAULT`, or `LOCKOUT`), contactor open, fault broadcast. |
| **Layer 3: Supervisory Coordinator** | ESP32-S3 (CPU 1) | Missing `Required` node, duplicate `GlobalNodeID` conflict, bus regulation collapse, critical low fleet SoC, storage failure | Inhibit startup or sequence orderly downstream-to-upstream shutdown (`AC → Boost → Sources`), log pre/post-fault snapshot, alert HMI. |

### 3. Fault Event Lifecycle

1. **Detection & Containment:** Local hardware/firmware transitions the node into `FAULT` (`0x05`) or `LOCKOUT` (`0x06`), terminates switching, and opens isolation contactors.
2. **Snapshot Capture:** The faulting node latches the electrical snapshot (`Snapshot_Voltage`, `Snapshot_Current`, `Fault_Bitmask`, local uptime) at the moment of trip before bus voltages collapse.
3. **High-Priority Broadcast:** Main-bus nodes transmit `EMERGENCY_FAULT` (`0x001–0x047`, highest CAN priority). Solar children report locally to their Solar Segment Gateway, which forwards an upstream emergency frame under the gateway's own emergency ID (child-slot payload TBD).
4. **Coordinator Reaction:**
   - **Optional node fault:** Coordinator logs the fault, raises an HMI alarm, rebalances remaining capacity, and enters `DEGRADED` if system capacity is reduced.
   - **Required node or bus-integrity fault:** Coordinator transitions to `FAULT` (or `LOCKOUT` for severe configuration/repeated trips) and commands orderly shutdown of dependent downstream stages.
   - **Pre/Post-Fault Logging:** Coordinator persists a timestamped/monotonic-sequence fault record to non-volatile storage without blocking CPU 1 CAN processing.
5. **Clearance & Controlled Re-Entry:**
   - Software breaker trips (Inverter `I²t` overload or remote trip) require explicit operator confirmation via local touchscreen or authenticated web HMI before issuing `CMD_NODE_CONTROL (0x05)`.
   - Hardware short-circuit trips, welded-contactor mirror mismatches, isolation/ground faults, and BMS critical cutoffs default to `LOCKOUT` (`0x06`).
   - Re-energization always re-enters through `READY` → `BUS_PRECHARGE` (`ΔV ≤ 1.5 V`) before returning to `RUNNING`.

### 4. Communication Loss vs. Electrical Fault

Loss of CAN communication (`COORD_HEARTBEAT` absent `> 500 ms`) is a **supervisory degradation**, not an immediate electrical trip:
- **Battery, Solar, Primary Boost, and Inverter Nodes** enter `DEGRADED` (`0x04`) and maintain safe local regulation/protection using their last valid bounded limits ([System_State_MAchine.md](System_State_MAchine.md)).
- **Secondary Boost Nodes** enter standby to avoid parallel voltage fighting without Coordinator balancing tokens.
- **Generator and Grid Interface Nodes** execute safe controlled ramp-down/cooldown and isolate.

---

## Open Questions

- Exact auto-recovery retry counts and cooldown timers per transient fault class (e.g., utility grid sag vs. bus undervoltage vs. crank timeout) before escalating to `LOCKOUT`.
- Exact pre-fault ring-buffer depth and post-fault capture duration stored on the Coordinator MicroSD card.
- Final Solar Gateway child-fault forwarding payload encoding (originating `SolarLocalSlot` + child fault bitmask).
- Hardwired E-Stop interaction with local `LOCKOUT` latches and post-E-Stop manual re-arming workflow.
