# System Safety, Auxiliary Lifeline, Overcurrent & E-Stop Architecture

Status: Approved Architectural Baseline (Rev 1.0, 2026-11-22) — Resolves all P0 Electrical & Safety items from [`Project_Punchlist.md`](./Project_Punchlist.md). Component part numbers, fuse curves, and safety-relay wiring must still be bench-verified against applicable NEC/UL standards prior to energizing high-power hardware.

---

## 1. Split Diode-OR'd `24 VDC` Auxiliary Control Lifeline

Replacing the earlier `30 V 50 kHz` AC bus eliminates unsynchronized multi-battery AC phase-shorting, prevents single-tap collapse of the microgrid, removes `50 kHz` broadband EMI on long cabinet/rooftop harnesses, and cleanly bounds the `24 VDC` supply feeding the STM32 Control Card.

### 1.1 Dual-Rail Topology (`AUX_LOGIC_24V` vs. `AUX_COIL_24V`)

```text
  [Battery 0 Pack (48V)] --> [Isol. 24.0V DC-DC + Droop] --> [Ideal Diode OR] --+
  [Battery 1 Pack (48V)] --> [Isol. 24.0V DC-DC + Droop] --> [Ideal Diode OR] --+
  [Battery N Pack (48V)] --> [Isol. 24.0V DC-DC + Droop] --> [Ideal Diode OR] --+
                                                                                |
                         +======================================================+
                         |  SHARED 24.0 VDC AUXILIARY BACKBONE (22.8 - 25.2 V, clamped < 32 V)
                         |
                         +---> [Per-Tap Fuse / eFuse (1-3 A)] ---> AUX_LOGIC_24V (UN-INTERRUPTIBLE)
                         |       * ESP32-S3 Coordinator & HMI
                         |       * All STM32G474QET6 Control Cards (+ local optional 24V:24V iso brick)
                         |       * Isolated CAN transceivers & telemetry sensors
                         |       * NEVER broken by E-Stop or local module faults
                         |
                         +---> [LATCHED HARDWARE SAFETY RELAY] --> AUX_COIL_24V (SAFETY-INTERLOCKED)
                                 (Series NC Loop: Cabinet E-Stop + Exterior E-Stop +
                                  96V DC GFDI Aux Trip + Manual Re-Arm Pushbutton)
                                 |
                                 +---> [Per-Tap Fuse / eFuse] ---> All 24 VDC Contactor Coils (Q_PRE, Q_ISOL, AC/DC)
                                 +---> 400V (±200V) Fail-Safe NC Discharge Relay Coils (energized OPEN)
                                 +---> Generator Fuel / Ignition Solenoid Interlock
                                 +---> Opto-Isolated ESTOP_OK -> HRTIM1_FLT3 (J2:20) on every power module
                                 +---> NEC 690.12 Rooftop Solar Rapid Shutdown 24V Keep-Alive Rail
```

### 1.2 Electrical Ratings & Fault-Isolation Rules

| Parameter | Specification | Engineering Rationale |
| :--- | :--- | :--- |
| **Nominal Voltage** | `24.0 VDC` (`22.8 – 25.2 VDC` normal, `< 30 V` transient clamp) | Directly matches `24 VDC` industrial contactors and safely bounds the STM32 Control Card buck input (`P1` hold). |
| **Source Per Battery Node** | Galvanically isolated `48 V → 24.0 VDC` (`≥ 2.5 kVrms`, `150–250 W`) with `~0.3–0.5 V` output droop + Ideal-Diode OR (`LM74700` / Schottky) + output fuse | Seamless `0 ms` parallel sharing & failover across up to 5 Battery Nodes with zero clock/phase synchronization or circulating current. |
| **Per-Tap Short Protection** | Fast-acting fuse or active current-limited eFuse (`1 A` Coordinator/Display; `2–3 A` per power module; `0.5 A` per Solar Gateway/tap) | A dead short on any single module tap trips locally in microseconds without collapsing `AUX_LOGIC_24V` for the rest of the microgrid. |
| **Optional Per-Card Isolation** | Host board may add a compact `24 V → 24 V` isolated DC-DC (`10–15 W`) ahead of `J1:1..4` | Keeps every module's `GND_CTRL` isolated from `AUX_GND` for zero inter-cabinet ground loops and safe isolated oscilloscope probing (power-stage sensors/drivers/CAN remain isolated either way). |
| **Black-Start Rule** | At least one healthy Battery Node energizes `AUX_LOGIC_24V` directly from its `48 V` pack without requiring `96 VDC` or `400 VDC` bus presence. | Preserves full Black-Start capability; local faults never disable `AUX_LOGIC_24V`. |

---

## 2. System DC Overcurrent Fusing & Contactor Coordination

**Core Protection Principle:** Mechanical contactors (`Q_ISOL`, `Q_PRE`) are rated for functional switching and normal/overload disconnect—they shall **never** be relied upon as the sole clearing device for a bolted multi-kiloamp DC short circuit. Every battery pack, power source, and bulk capacitor bank feeder requires passive high-AIC DC fusing coordinated below contactor weld thresholds.

| Circuit / Module Port | Max Continuous Current | Fault Energy Source | Mandatory Passive Protection | Contactor / Disconnect Requirement |
| :--- | :--- | :--- | :--- | :--- |
| **Battery Pack `48 V` Port (`2.5 kW`)** | `62.5 A` @ `40 V` (`75 A` peak) | LiFePO4 / cell bank (`> 2–5 kA` short) | **Class-T Fuse (`100–110 A`, `125–160 VDC`, `20 kA` AIC)** on `B+` inside pack | DALY BMS FET disconnect + manual service disconnect |
| **Battery DAB `96 V` Port (`2.5 kW`)** | `26–32 A` | `96 V` bus cap bank + other sources | **DC Fuse (`45–50 A`, `150–250 VDC`, high-AIC)** on ungrounded `LV_BUS+` | `Q_PRE` + `Q_ISOL` hermetically sealed DC contactor (`≥ 150 VDC`, `≥ 100 A`, aux mirror contact) |
| **Boost FB-LLC `96 V` Input (`10 kW`)** | `108–114 A` (`150 A` peak) | `96 V` bus cap bank + Batteries + Solar | **Class-T / Semiconductor Fuse (`175–200 A`, `150–250 VDC`, `≥ 20 kA` AIC)** on `LV_BUS+` | `Q_PRE` + `Q_ISOL` DC contactor (e.g., Kilovac `EV200` / GIGAVAC `GX14` / Hongfa `HFE82V`, `24 VDC` coil, mirror contact) |
| **Solar MPPT (`PV+` & `96 V` Out)** | `10–15 A` PV / `5–8 A` `96 V` | `96 V` bus back-feed (on fault) | **`gPV` Fuse (`1.56× I_sc`)** on `PV+` + **`10–15 A` (`150 VDC+`)** fuse on `LV_BUS+` | Solid-state or `24 VDC` output relay controlled by `AUX_COIL_24V` rapid-shutdown keep-alive |
| **Generator `96 V` Port (`~5 kW`)** | `~55 A` (`80 A` cranking) | `96 V` bus cap bank | **Class-T / DC Fuse (`100–125 A`, `150–250 VDC`)** on `LV_BUS+` | `Q_PRE` + `Q_ISOL` DC contactor (`≥ 150 VDC`, `≥ 100 A`, mirror contact) |
| **Utility-Tie `96 V` Port (`10.8 kW`)** | `~115 A` | `96 V` bus cap bank | **Class-T / DC Fuse (`150–175 A`, `150–250 VDC`)** on `LV_BUS+` + `60 A` 2-pole AC breaker | 2-pole AC input contactor + `Q_PRE` + `Q_ISOL` DC contactor |
| **Boost FB-LLC `±200 V` Output (`10 kW`)** | `26.3 A` (`37.5 A` peak) per rail | `±200 V` split cap bank + parallel Boosts | **2-Pole Semiconductor / DC Fuse (`50 A`, `500 VDC`)** on **BOTH `HV+` and `HV-`** | **Never fuse `HV_MID` (`N/PE`)!** Passive SiC diodes block reverse current; primary `C_r` + xfmr block DC feed-through |
| **Inverter Module `±200 V` DC Input** | `12–24 A` (`48 A` surge) per rail | `±200 V` split cap bank | **2-Pole DC Fuse (`50–63 A`, `500 VDC`)** on **BOTH `HV+` and `HV-`** | 2-pole DC disconnect (`≥ 500 VDC` DC-1, `HV+` & `HV-`) + AC line contactor (`L1` / `L1`+`L2`, Neutral continuously bonded) |

---

## 3. Single-Point `LV_BUS-` DC GFDI & `400 VDC` (`±200 V`) Stored-Energy Discharge

### 3.1 Single-Point `96 VDC` Negative-Ground Bond via DC GFDI
- `LV_BUS-` is bonded to the main Earth (`PE`) busbar at **exactly ONE physical point** through a **DC GFDI circuit breaker (`1 A – 5 A` DC rated, with integral normally-closed auxiliary trip contact)**.
- **Normal Operation:** Holds `LV_BUS-` solidly at `0 V` relative to `PE` (no floating `96 V` rail, aligns safely with negative-grounded generator alternator frame and PV array reference).
- **Ground-Fault Event:** Any insulation fault from `LV_BUS+` or `PV+` to chassis/Earth drives fault current through the GFDI bond, immediately tripping the `1–5 A` DC GFDI breaker.
- **Interlock Action:** Opening the GFDI breaker's auxiliary contact immediately breaks the **Hardwired Latched E-Stop Safety Relay loop**, dropping `AUX_COIL_24V` to `0 V`, halting all power stages (`HRTIM1_FLT3`), opening all contactors, and latching system `LOCKOUT` (`Bit 6 = 0x0040` / `Bit 15 = 0x8000`) until the ground fault is located and repaired.

### 3.2 Fail-Safe Stored-Energy Discharge (`400 VDC` / `±200 VDC` Split Bank & `96 VDC` Bank)
Large bulk capacitor banks sized for slow voltage-regulation loops store substantial energy (`E = ½ C V²`) and cannot safely rely on high-loss continuous bleeders alone:

1. **Primary Fail-Safe Discharge — Dual-Gated Spring-Closed Normally-Closed (`NC`) Relays (`≤ 10 s` to `< 48 VDC`):**
   - Each half-rail (`C_top`: `+200 V → HV_MID` and `C_bot`: `HV_MID → -200 V`, `60 mF` each) has a dedicated `24 VDC` coil **Normally-Closed (`NC`) DC-rated discharge relay** in series with a chassis-mounted **`100 Ω / 200 W`** pulse-rated wirewound resistor (`2.15 A` peak, `1,200 J` dump, `1.5 τ = 9.0 s` to `< 48 VDC`).
   - **Dual-Gated Coil Interlock (Critical Design Rule):** The relay coil's `+24 V` supply comes from **`AUX_COIL_24V`** (hardwired E-Stop safety relay), and its low-side return is switched by **`CONT_CMD3`** on Primary Boost 0 (and similarly on Startup Battery 0 for the `96 V` `330 mF` bank's `33 Ω / 200 W` `NC` relay). This guarantees THREE simultaneous safety properties:
     1. **Safe in `READY`:** Buses remain actively discharged (`0 V`) even after the operator presses Safety Re-Arm, right up until `BUS_PRECHARGE` begins.
     2. **Clean Software Stop (`0 W` running loss):** Energized OPEN during `BUS_PRECHARGE`/`RUNNING`; releasing `CONT_CMD3` during normal software `STOP` cleanly discharges both buses `< 48 VDC` in `< 9 s` without triggering E-Stop.
     3. **Zero-Software Hardware E-Stop:** Cutting `AUX_COIL_24V` drops every `NC` discharge relay CLOSED by spring force regardless of MCU or firmware state.
2. **Secondary Backup Passive Bleeder + Visual Indicator:**
   - Matched high-ohm passive bleeder resistors (`33 kΩ / 5 W` metal-oxide) permanently across both `C_top` and `C_bot` (`1.2 W` continuous per rail) provide static voltage equalization across series cans and a secondary fail-safe discharge path if an NC relay ever stuck open.
   - Direct-wired **HV Present LED indicators** on both `+200 V` and `-200 V` rails remain visibly lit down to `< 30 VDC` for operator lockout/tagout verification.
3. **`96 VDC` (`330 mF`) Bulk Bank Discharge:**
   - Dedicated dual-gated `NC` discharge relay (`AUX_COIL_24V` + Battery 0 `CONT_CMD3`) + **`33 Ω / 200 W`** wirewound resistor (`3.0 A` peak, `1,520 J` dump, `0.73 τ = 7.9 s` from `100 V` to `< 48 VDC`, `0 W` running loss), backed up by a `10 kΩ / 5 W` passive bleeder (`< 1 W`) + Bus-Energized LED.

---

## 4. Hardwired Emergency Stop (E-Stop) & NEC 690.12 Solar Rapid Shutdown

### 4.1 Stop Category 0 Hardware Safety Chain (Zero Software Dependency)
`AUX_COIL_24V` is fed strictly through a **latched safety relay** whose series normally-closed (NC) enable loop includes:
1. Main cabinet red mushroom E-Stop pushbutton (NC, twist-to-release).
2. Exterior / fire-responder E-Stop & Rapid Shutdown initiator (NC).
3. `96 VDC` single-point DC GFDI auxiliary trip contact (NC).
4. Optional enclosure door safety interlock (with key-operated service bypass).
5. Momentary **Manual Re-Arm Pushbutton** — resetting a mushroom button alone **never** re-energizes `AUX_COIL_24V`; an intentional operator Re-Arm press is required after verifying safe conditions.

### 4.2 Deterministic E-Stop Response & Cold-Start `ESTOP_OK` Distinction
- **Cold-Start (`STARTUP` / `READY`) Pre-Re-Arm Behavior:** On initial Black Start, `AUX_LOGIC_24V` powers the Coordinator and STM32 cards while `AUX_COIL_24V` is still `0 V` (until the operator presses **Manual Safety Re-Arm**). Firmware must treat `ESTOP_OK = LOW` (`HRTIM1_FLT3`) in `STARTUP`/`READY` as **"Safety Interlock Disarmed (Waiting for Re-Arm)"**—inhibiting contactor closures and PWM without falsely latching `LOCKOUT` on boot.
- **Active-State (`BUS_PRECHARGE` / `RUNNING`) E-Stop Timeline:**
  1. **`t < 1 µs`:** Loss of `AUX_COIL_24V` drops opto-isolated `ESTOP_OK` on every module's `HRTIM1_FLT3` (`J2:20`) → immediate asynchronous hardware PWM kill on Boost FB-LLC, Battery DAB, Inverters, Generator, and Utility-Tie.
  2. **`t ≈ 10–25 ms`:** Every `Q_ISOL`, `Q_PRE`, AC output contactor, 2-pole HV DC disconnect, and Generator fuel/ignition solenoid drops OPEN by spring force.
  3. **`t ≈ 25 ms – 10 s`:** Fail-safe `NC` discharge relays close across `C_top` (`+200 V`), `C_bot` (`-200 V`), and `C_96` → bus voltages collapse below `< 48 VDC`.
  4. **Rooftop Solar Rapid Shutdown (NEC 690.12):** Loss of the `AUX_COIL_24V` rapid-shutdown keep-alive wire opens every roof-mounted per-panel Solar MPPT's `96 V` output stage immediately, bleeding the rooftop DC homerun conductors below **`< 30 VDC` within seconds** (well inside NEC 690.12's `30 s / ≤ 30 V` outside-array-boundary rule), while individual panels remain `≤ 80 V` (`~36–48 V` `V_oc`) inside the array boundary.
  5. **Continuous Diagnostics & Logging:** `AUX_LOGIC_24V` remains powered throughout, so every STM32 broadcasts `EMERGENCY_FAULT` (`Bit 15 = 0x8000`, `FLT_COM_ESTOP_OPEN`), latches `LOCKOUT`, and the ESP32-S3 Coordinator logs the event to MicroSD and displays **"EMERGENCY STOP ACTIVE"** on the touchscreen and web UI.

---

## 5. Key Architectural Decisions & Catch Log (2026-11-22)

Recorded here so future engineering sessions preserve the exact rationale behind every architectural pivot and edge-case catch:

| Decision / Catch | Problem Prevented | Authoritative Reference |
| :--- | :--- | :--- |
| **10 kW 2-Ph FB-LLC Boost (`96 V → ±200 V` / `400 V`)** | Eliminates non-isolated `96 V → HV` short-circuit diode path and isolates `400 V` split bus so `HV_MID` can safely bond to `PE` / AC Neutral (`N`). Bumping `380 V → 400 V` adds `30 V` modulation headroom above `170 V` (`120 VAC` peak). | [`../hardware/boost_converter/Boost_Converter_Architecture.md`](../hardware/boost_converter/Boost_Converter_Architecture.md) |
| **Center-Tapped Full-Bridge SiC Rectifier (`6:13:13`, `45 kHz`)** | Magnetically clamps `+200 V` and `-200 V` equal with zero DC transformer core bias, uses `100%` SOT-227 + M4 copper busbars (zero high-power PCB traces), gives `±15–20%` DIY winding/parasitic fudge factor on `TDK U93/76/30` (`26 AWG` bundles), and passes `15 kW` surge at `91.2 V`. | [`../hardware/boost_converter/Simulations/fbllc_verify.py`](../hardware/boost_converter/Simulations/fbllc_verify.py) |
| **Inverter Half-Bridge (`1-leg 120V` / `2-leg 240V`)** | Prevents dead-shorting a switched Neutral leg when `HV_MID` is bonded to `PE`; cuts switch count in half; uses 2-pole `HV+`/`HV-` DC disconnect and line-only AC disconnect (`L1`/`L2`). | [`../hardware/Inverter_Modules_Hardware.md`](../hardware/Inverter_Modules_Hardware.md) |
| **Two-Stage Isolated Utility-Tie (`PFC + LLC/PSFB`)** | Prevents Utility Neutral (`PE`) from shorting into negative-grounded `LV_BUS-` (`PE`) on negative AC half-cycles. | [`../hardware/Utility_Tie_Hardware.md`](../hardware/Utility_Tie_Hardware.md) |
| **2.5 kW Isolated Bidirectional DAB Battery** | Prevents accidental `B-` to `PE` faults on a battery rack from bypassing COTS DALY BMS low-side (`B-`/`P-`) protection FETs under grounded `LV_BUS-`; uses `8` `HRTIM1` pins already on `J2`. | [`../hardware/Battery_Isolation_Topology_Study.md`](../hardware/Battery_Isolation_Topology_Study.md) |
| **Split Diode-OR'd `24 VDC` Auxiliary Bus** | Replaces `30 V 50 kHz` AC lifeline—eliminates unsynchronized multi-battery AC shorting, single-tap collapse, and `50 kHz` roof harness EMI; bounds Control Card `24 V` input (`22.8–25.2 V`). | Section 1 above |
| **Ultra-Stiff Bulk Banks (`60 mF`/rail `±200 V`; `330 mF` `96 V`)** | Enables ultra-slow `3–5 Hz` outer loops through `10 kW` transients, absorbs `15 kW` Solar load-dump with zero dump-load resistor, and eliminates giant pre-charge resistors via lossless Battery DAB cold soft-start. | [`Bus_Capacitance_Droop_and_Transient_Spec.md`](./Bus_Capacitance_Droop_and_Transient_Spec.md) |
| **Dual-Gated `NC` Discharge Relays + Cold-Start `ESTOP_OK`** | Keeps buses at `0 V` in `READY`, discharges `< 48 V` in `< 9 s` on software `STOP` or hardware E-Stop (`0 W` running loss), and prevents false `LOCKOUT` on cold boot before operator presses Safety Re-Arm. | [`../architecture/System_State_MAchine.md`](../architecture/System_State_MAchine.md) |
