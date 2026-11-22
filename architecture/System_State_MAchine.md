# System State Machine

## Purpose

This document defines the supervisory state machine executed by the Coordinator Node, the standardized local state machine executed by individual functional nodes, and the deterministic startup and degraded-mode transitions across the microgrid.

## Design Philosophy

- **Sequenced Bus Energization:** Power buses are brought online in strict upstream-to-downstream order (`AUX_LOGIC_24V` / `AUX_COIL_24V` ➔ `96VDC Energy Bus` via Battery DAB soft-start ➔ `400VDC (±200V) Inverter Bus` via Boost FB-LLC PSM soft-start ➔ `AC Output`).
- **Local Safety Autonomy:** Any node may transition immediately into `FAULT` or `LOCKOUT` without Coordinator permission.
- **Graceful Degradation:** Loss of Coordinator communications transitions active modules into `DEGRADED` mode rather than causing an immediate blackout.

## Assumptions

- At least one Battery Node is healthy enough to energize the `AUX_LOGIC_24V` Common Auxiliary Power Bus during a cold `Black Start`.
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
| **OFF** | Unpowered state prior to `AUX_LOGIC_24V` auxiliary bus energization. | Exits to `STARTUP` upon receiving `5.0V` rail power from `AUX_LOGIC_24V`. |
| **STARTUP** | Bootstraps ESP32-S3 peripherals, sets `CH422G EXIO5 = HIGH` (`CAN_SEL`), mounts storage, and initializes UI. | Exits to `DISCOVERY` once TWAI CAN bus and stored configuration load cleanly. |
| **DISCOVERY** | Collects node heartbeats and identity messages and checks them against the expected node list. The discovery deadline and identity-request behavior must follow the approved CAN contract. | Exits to `READY` when all `Required` nodes report `READY` with zero ID conflicts. Transitions to `FAULT` if a conflict or missing `Required` node persists past the approved discovery deadline. |
| **READY** | All `Required` nodes are verified online and healthy; main power buses remain de-energized awaiting auto-start or operator command. | Exits to `BUS_PRECHARGE` when startup is commanded. |
| **BUS_PRECHARGE** | Sequences bus bring-up: **Step 1:** Battery DAB closes onto de-energized `96VDC` bus (`330 mF`) and soft-starts cleanly to `96.0 V`. **Step 2:** Boost FB-LLC closes `Q_ISOL` and `PSM` soft-starts the isolated `400VDC` (`±200V`, `60 mF`/rail) split bus. | Exits to `RUNNING` once `96VDC` (`91.2V–100.8V`) and `400VDC` (`380V–415V`, balanced `±200V`) buses stabilize within nominal windows. |
| **RUNNING** | Normal coordinated operation. Dispatches Solar, Grid, and Generator inputs, manages Battery Fleet state, balances Boost/Inverter loads, and enables AC outputs. | Transitions to `DEGRADED` if an `Optional` node drops offline or capacity is constrained; transitions to `FAULT` if bus regulation fails or a `Required` capability is lost. |
| **DEGRADED** | Reduced-capacity or fault-ride-through operation (e.g., partial node loss, low SOC load-shedding, or CAN bus errors). | Returns to `RUNNING` when full capability is restored; transitions to `FAULT` if critical load support cannot be maintained. |
| **FAULT** | Protective supervisory halt. Commands orderly shutdown of downstream AC outputs and upstream converters while keeping `AUX_LOGIC_24V` lifeline active. | Requires fault clearance and operator/auto recovery criteria to return to `DISCOVERY`. |
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
   - **Boost Nodes:** Primary Boost Node maintains `400VDC` (`±200V`) regulation; Secondary Boost Nodes enter standby.
   - **Inverter Nodes:** Maintain `60Hz` AC output while `400VDC` (`±200V`) bus remains valid and local `I²t` limits are respected.
   - **Generator / Grid Nodes:** Ramp down power and isolate safely (`Generator` executes cooldown before engine stop; `Grid Interface` opens contactors).
6. **FAULT:** Local safety limit exceeded; PWM gated off immediately and `Q_ISOL` opened (`AUX_LOGIC_24V` auxiliary supply remains active on Battery Nodes). May auto-recover to `READY` if fault is transient and clearable.
7. **LOCKOUT:** Latched hardware trip (e.g., welded contactor, short-circuit latch, E-Stop drop while energized, DC GFDI trip, or max retry count exceeded). Requires explicit operator reset command or power-cycle service.

---

### 3. Cold Start ("Black Start") Deterministic Order of Operations

Because high-power modules are never hot-swapped, cold system energization executes in seven deterministic stages from `0 V` across all power buses:

#### Stage 0 — `OFF` (Completely De-Energized)
- `96 VDC` bus (`330 mF`) = `0 V` (`NC` discharge relay spring-closed across `33 Ω / 200 W`).
- `400 VDC` (`±200 V`, `60 mF`/rail) split bus = `0 V` (`NC` discharge relays spring-closed across `100 Ω / 200 W`).
- `AUX_LOGIC_24V = 0 V`, `AUX_COIL_24V = 0 V`; all contactors (`Q_PRE`, `Q_ISOL`, Inverter 2-pole DC & AC line contactors) are spring-return `OPEN`.
- **Black-Start Wake:** Operator activates Battery 0 (or any healthy Battery Node). Its internal isolated `48 V → 24.0 VDC` supply energizes `AUX_LOGIC_24V` via Ideal-Diode OR (all other healthy Battery Nodes join seamlessly in parallel).

#### Stage 1 — `STARTUP` (Control Brains Boot; High Power Still Locked Out)
- `AUX_LOGIC_24V` boots the ESP32-S3 Coordinator, all `STM32G474QET6` Control Cards (via `1–3 A` per-tap fuses/eFuses), isolated CAN transceivers (`ISO1042DWV`), and Waveshare LCDs. **`AUX_COIL_24V` remains `0 V`**.
- **Pre-Re-Arm `ESTOP_OK` (`HRTIM1_FLT3`) Rule:** While a module is in `STARTUP` or `READY` prior to operator Safety Re-Arm, `ESTOP_OK = LOW` (`AUX_COIL_24V = 0 V`) is treated as **"Safety Interlock Disarmed (Waiting for Re-Arm)"**—it inhibits contactor/PWM commands without latching a permanent `LOCKOUT` on boot.
- **Local Self-Tests:** Every STM32 sets `PWR_CR3.UCPD1_DBDIS = 1`, verifies `HRTIM1` outputs OFF (`10 kΩ` pull-downs) and `GATE_EN = LOW`, confirms all contactor mirror feedbacks (`CONT_FB1..3`) read `OPEN` (welded-contact pre-flight check), confirms buses read `< 5 V`, verifies DALY BMS / sensor health, brings up `500 kbps` Classic CAN, and transitions to `READY` (`0x02`).
- Coordinator sets `CH422G EXIO5 = HIGH`, starts TWAI CAN, loads commissioned topology (`Required`/`Optional`/`Ignored`), broadcasts `COORD_HEARTBEAT` (`0x100`, `100 ms`, all enable flags `0`), and enters `DISCOVERY`.

#### Stage 2 — `DISCOVERY` (Roll Call & Pre-Flight Verification)
- Online main-bus nodes transmit `NODE_IDENTITY` (`0x700+`), periodic `NODE_HEARTBEAT` (`0x600+`, `State = READY`, `Aux ≈ 24000 mV`), and baseline telemetry (`0 V` buses, open contactors, Battery SoC, `BMS Discharge Allowed = 1`).
- Coordinator validates within discovery deadline (`3 s` provisional): all `Required` nodes `READY`, zero duplicate `GlobalNodeID` serial conflicts, zero `FAULT`/`LOCKOUT` nodes, and ≥1 healthy Battery with discharge allowed → enters `READY` (`0x02`).

#### Stage 3 — `READY` (Manual Safety Relay Re-Arm & Operator Start)
- Operator confirms E-Stop mushroom buttons are released and the `1–5 A` `96 V` DC GFDI breaker is closed, then presses the physical **Manual Safety Re-Arm** pushbutton → Latched Safety Relay closes → **`AUX_COIL_24V = 24.0 VDC`**.
- `ESTOP_OK` (`HRTIM1_FLT3`) goes `HIGH` on every module; NEC 690.12 rooftop Solar keep-alive goes `HIGH` (Solar MPPTs wake on local gateway segments in `READY`, output FETs still `OPEN`).
- Bus `NC` discharge relays remain `CLOSED` (held off until explicitly energized open via `CONT_CMD3` in Stage 4/5).
- Operator taps **START SYSTEM** (or auto-start triggers) → Coordinator enters `BUS_PRECHARGE` (`0x03`). From this point forward, any loss of `ESTOP_OK` (`AUX_COIL_24V → 0 V`) immediately triggers `< 1 µs` `HRTIM1_FLT3` PWM kill, `EMERGENCY_FAULT (Bit 15)`, and latched `LOCKOUT`.

#### Stage 4 — `BUS_PRECHARGE` Step A: Lossless `96 VDC` Bus Soft-Start (`0 V → 96.0 V`, `~2–3 s`)
- Coordinator sets `Fleet Allow Discharge = 1` and commands startup Battery 0 (`0x121`, `Action = 0x01`): Battery 0 verifies `V_BUS96 < 5 V`, energizes the `96 V` `NC` discharge relay **OPEN** (coil fed by `AUX_COIL_24V`, gated by `CONT_CMD3`), closes `Q_ISOL` onto the dead `0 V` bus (`ΔV = 0 V`, zero arc, zero pre-charge resistor heat), verifies mirror feedback, and ACKs (`0x760`).
- Coordinator sends `Action = 0x02` (`Target = 96.00 V`): Battery 0 releases `GATE_EN` and ramps DAB phase shift `φ` from `0°` under `10–15 A` constant-current limit, smoothly charging the `330 mF` `96 V` bank in `~2.1–3.2 s` (aborting in `< 200 ms` on abnormal `dV/dt` short-circuit detection).
- Remaining Battery DABs (`Battery 1..N`) pre-charge their internal `96 V` port caps to match `V_BUS96` (`ΔV ≤ 1.5 V`), close `Q_ISOL`, and join parallel `96.0 V` regulation. Coordinator verifies `91.2 V ≤ V_96 ≤ 100.8 V` stable.

#### Stage 5 — `BUS_PRECHARGE` Step B: Isolated `400 VDC` (`±200 V`) Split-Bus Soft-Start (`0 V → ±200 V`, `~2 s`)
- Coordinator sets `Allow HV 400V Boost = 1` and commands Primary Boost 0 (`0x126`, `Action = 0x01`): Boost 0 closes `Q_PRE` (`22 Ω / 50 W`) to charge only its small local `~6 mF` `96 V` input cap (`< 30 J`, `~0.5 s`), confirms `ΔV ≤ 1.5 V`, closes `Q_ISOL`, opens `Q_PRE`, and ACKs (`0x765`).
- Coordinator sends `Action = 0x02` (`Target = 400.00 V` [`8000`]): Boost 0 energizes the centralized `±200 V` `NC` discharge relays **OPEN** (`CONT_CMD3` on `AUX_COIL_24V`), clears its hardware fault latch (`GPIO_PF4`), releases `GATE_EN`, and runs **`SoftStart_PSM`** (`f_sw = 150 kHz`, `φ: 160° → 0°`, `5–8 A` DC output limit) to ramp `+200.0 V` and `-200.0 V` (`60 mF`/rail) symmetrically in `~1.5–2.5 s` while verifying `dV/dt` and `|V_HV+ − |V_HV-|| ≤ 10 V`, then transitions seamlessly into **`Normal_PFM`** / `Burst_LightLoad`.
- Secondary Boosts (`Boost 1..N`) pre-charge local `96 V` inputs, close `Q_ISOL`, and enter standby/parallel sharing (passive SiC secondary diodes block reverse back-feed). Coordinator verifies `380.0 V ≤ V_400 ≤ 415.0 V` and `Split_Rail_Balance_OK = 1`.

#### Stage 6 — `RUNNING` (Staggered AC Inverter Enable & Source Dispatch)
- Coordinator transitions to `RUNNING` (`0x04`) and sets `Allow AC Inverter Outputs = 1`.
- **Time-Master Inverter First:** Closes 2-pole `HV+`/`HV-` DC pre-charge & main DC disconnect (`ΔV ≤ 1.5 V`), soft-ramps `60 Hz` half-bridge AC voltage (`0 → 120/240 VAC` over `~100 ms`), verifies zero GFCI leakage, closes AC line contactor (`L1` or `L1+L2`; Neutral remains continuously bonded to `HV_MID / PE`), and broadcasts zero-cross sync pulses.
- **Staggered Branch/Parallel Inverters:** Lock `Sync_Locked = 1`, close 2-pole DC disconnects, phase-match, and close AC line contactors staggered by `200–500 ms` each to avoid simultaneous household inrush spikes.
- **Solar & Aux Dispatch:** Solar Gateways enable rooftop Solar MPPTs onto `96 VDC`; Generator and Grid Interface remain in `READY` standby until dispatched.

#### Normal Software Stop vs. Hardware E-Stop
- **Normal Software Stop (`READY`):** Orderly downstream-to-upstream shutdown (`AC Inverters OPEN` → `Boosts OFF` → `Release CONT_CMD3` so `NC` discharge relays drop `±200 V` and `96 V` `< 48 VDC` in `< 9 s` → `Batteries Q_ISOL OPEN`).
- **Hardware E-Stop / DC GFDI Trip:** Latched Safety Relay drops `AUX_COIL_24V → 0 V` instantaneously (`< 1 µs` `HRTIM1_FLT3` PWM kill + spring-open all contactors + spring-close all `NC` discharge relays + NEC 690.12 roof Solar shutdown), while `AUX_LOGIC_24V` keeps Coordinator & STM32 cards online to log `EMERGENCY_FAULT (Bit 15)` and hold `LOCKOUT`.

---

## Open Questions

- Exact voltage-droop deadbands for autonomous Battery Node transitions between charge and discharge if Coordinator loss persists over multiple hours.
- Time delay before automatically shedding non-critical AC Inverter Nodes during low-SOC `DEGRADED` operation.
