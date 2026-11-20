# System State Machine

## Purpose

This document defines the supervisory state machine executed by the Coordinator Node, the standardized local state machine executed by individual functional nodes, and the deterministic startup and degraded-mode transitions across the microgrid.

## Design Philosophy

- **Sequenced Bus Energization:** Power buses are brought online in strict upstream-to-downstream order (`30V 50kHz Auxiliary Bus` ➔ `96VDC Energy Bus` ➔ `380VDC Inverter Bus` ➔ `AC Output`).
- **Local Safety Autonomy:** Any node may transition immediately into `FAULT` or `LOCKOUT` without Coordinator permission.
- **Graceful Degradation:** Loss of Coordinator communications transitions active modules into `DEGRADED` mode rather than causing an immediate blackout.

## Assumptions

- At least one Battery Node is healthy enough to energize the `30V 50kHz` Common Auxiliary Power Bus during a cold `Black Start`.
- Expected system topology (`Required`, `Optional`, and `Ignored` nodes) has been saved to Coordinator non-volatile storage after initial commissioning.

## Constraints

- System startup shall be inhibited if any `Required` node is missing, in `FAULT`/`LOCKOUT`, or involved in a duplicate Node ID conflict.
- Pre-charge equalization (`ΔV ≤ 1.5V`) must complete before any module closes its main isolation contactor (`Q_ISOL`).
- Battery fleet states (`CHARGING`, `DISCHARGING`, `IDLE`) are mutually exclusive across the fleet; the Coordinator shall never command one Battery Node to charge while another discharges.

---

## Requirements

### 1. Coordinator Supervisory States

| State | Description | Entry / Exit Criteria |
| :--- | :--- | :--- |
| **OFF** | Unpowered state prior to `30V 50kHz` auxiliary bus energization. | Exits to `STARTUP` upon receiving `5.0V` rail power from the `30V 50kHz` lifeline. |
| **STARTUP** | Bootstraps ESP32-S3 peripherals, sets `CH422G EXIO5 = HIGH` (`CAN_SEL`), mounts storage, and initializes UI. | Exits to `DISCOVERY` once TWAI CAN bus and stored configuration load cleanly. |
| **DISCOVERY** | Collects node heartbeats and identity messages and checks them against the expected node list. The discovery deadline and identity-request behavior must follow the approved CAN contract. | Exits to `READY` when all `Required` nodes report `READY` with zero ID conflicts. Transitions to `FAULT` if a conflict or missing `Required` node persists past the approved discovery deadline. |
| **READY** | All `Required` nodes are verified online and healthy; main power buses remain de-energized awaiting auto-start or operator command. | Exits to `BUS_PRECHARGE` when startup is commanded. |
| **BUS_PRECHARGE** | Sequences bus bring-up: **Step 1:** Battery Nodes pre-charge and close onto `96VDC` bus. **Step 2:** Boost Nodes pre-charge and regulate `380VDC` bus. | Exits to `RUNNING` once `96VDC` (`86.4V–105.6V`) and `380VDC` buses stabilize within nominal windows. |
| **RUNNING** | Normal coordinated operation. Dispatches Solar, Grid, and Generator inputs, manages Battery Fleet state, balances Boost/Inverter loads, and enables AC outputs. | Transitions to `DEGRADED` if an `Optional` node drops offline or capacity is constrained; transitions to `FAULT` if bus regulation fails or a `Required` capability is lost. |
| **DEGRADED** | Reduced-capacity or fault-ride-through operation (e.g., partial node loss, low SOC load-shedding, or CAN bus errors). | Returns to `RUNNING` when full capability is restored; transitions to `FAULT` if critical load support cannot be maintained. |
| **FAULT** | Protective supervisory halt. Commands orderly shutdown of downstream AC outputs and upstream converters while keeping `30V 50kHz` lifeline active. | Requires fault clearance and operator/auto recovery criteria to return to `DISCOVERY`. |
| **LOCKOUT** | Latched supervisory inhibit caused by severe configuration errors (e.g., duplicate `Required` Node ID) or repeated critical faults. | Requires explicit operator confirmation via local touchscreen or web HMI to reset. |

---

### 2. Standardized Module Local States

Every functional node (`0x100–0x6FF`) implements the following local state machine:

1. **OFF:** Module unpowered.
2. **STARTUP:** MCU initializes clocks, ADCs, `HRTIM1` safe-off states, isolated CAN transceiver, and internal subsystem links (e.g., DALY BMS UART/CAN).
3. **READY:** Self-test passed, contactors open (`Q_PRE = OPEN`, `Q_ISOL = OPEN`), transmitting the node-type-specific heartbeat, awaiting Coordinator command.
4. **RUNNING:** Pre-charge sequence completed (`Q_ISOL = CLOSED`), active power conversion or harvesting enabled under local closed-loop control and Coordinator dispatch limits.
5. **DEGRADED:** Coordinator heartbeat timeout (`>500 ms`) or local thermal derating active:
   - **Battery Nodes:** Hold `96VDC` bus regulation within safe voltage droop bounds using last valid limits.
   - **Solar Nodes:** Continue autonomous MPPT with local `96VDC` overvoltage curtailment.
   - **Boost Nodes:** Primary Boost Node maintains `380VDC` regulation; Secondary Boost Nodes enter standby.
   - **Inverter Nodes:** Maintain `60Hz` AC output while `380VDC` bus remains valid and local `I²t` limits are respected.
   - **Generator / Grid Nodes:** Ramp down power and isolate safely (`Generator` executes cooldown before engine stop; `Grid Interface` opens contactors).
6. **FAULT:** Local safety limit exceeded; PWM gated off immediately and `Q_ISOL` opened (`30V 50kHz` auxiliary supply remains active on Battery Nodes). May auto-recover to `READY` if fault is transient and clearable.
7. **LOCKOUT:** Latched hardware trip (e.g., welded contactor, short-circuit latch, or max retry count exceeded). Requires explicit operator reset command or power-cycle service.

---

## Open Questions

- Exact voltage-droop deadbands for autonomous Battery Node transitions between charge and discharge if Coordinator loss persists over multiple hours.
- Time delay before automatically shedding non-critical AC Inverter Nodes during low-SOC `DEGRADED` operation.
