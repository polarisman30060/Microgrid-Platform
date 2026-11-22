# DC Bus Capacitance, Soft-Start Pre-Charge & Autonomous Droop Specification

Status: Approved Architectural Baseline (Rev 1.0, 2026-11-22) — Defines bulk capacitor bank sizing, zero-dump-load `96 VDC` transient absorption, Battery DAB soft-start pre-charge, and autonomous `DEGRADED_AUTONOMOUS` voltage-droop curves across both primary DC buses.

---

## 1. Design Philosophy: High-Capacitance Stiff DC Buses

Both primary DC buses (`96 VDC` Energy Bus and `400 VDC` / `±200 VDC` Split Inverter Bus) are intentionally sized as **ultra-stiff bulk capacitor banks** so that:
1. **Very Slow Outer Voltage Loops (`3–15 Hz`)** remain unconditionally stable with high phase margin through hard `10 kW` load steps and sudden load dumps.
2. **Stage Impedance Decoupling:** Parallel converters (Solar MPPTs, Battery DABs, Boost FB-LLCs, Inverters) see low-impedance voltage buses, eliminating negative-incremental-impedance oscillations.
3. **Zero Dump-Load Resistors Needed:** Because PV panels are bounded current sources with fast local `< 2–5 ms` overvoltage curtailment (no spinning mechanical inertia), sizing `C_96` generously absorbs worst-case BMS cutoff load dumps cleanly below the `108.0 V` trip threshold without dump-load choppers.
4. **Soft-Start Cold Bring-Up (No Hot-Swap):** Because no high-power module is hot-swapped, cold bus pre-charge is performed softly and losslessly by the **Battery DAB** (`96 VDC` bus) and **Boost FB-LLC `PSM` soft-start** (`400 VDC` split bus) rather than burning kilojoules in chassis pre-charge resistors.

---

## 2. Bulk Capacitor Bank Baseline Specifications

### 2.1 `400 VDC` (`±200 VDC` Split-Rail) Inverter Bus Bank — Tier B Ultra-Stiff Centralized Bank

- **Topology:** Symmetrical split bank (`C_top` across `HV+` to `HV_MID`; `C_bot` across `HV_MID` to `HV-`) with `HV_MID` single-point bonded to Earth (`PE`) and AC Neutral (`N`).
- **Baseline Capacitance:** **`C_top = 60 mF` (`60,000 µF`)** + **`C_bot = 60 mF` (`60,000 µF`)** (`250 VDC` minimum rated; `350 VDC` preferred for long-life margin) → **`C_eff,400 = 30.0 mF`** effective series capacitance across `400 VDC`.
- **Hardware Realization:** `6× 10,000 µF / 250 V` (or `10× 6,000 µF / 350 V`) industrial screw-terminal electrolytic cans per half-rail (`12` cans total, `~90–120 A RMS` `120 Hz` ripple rating per rail) on laminated `HV+ / HV_MID / HV-` busbars, closely bypassed with polypropylene film capacitors (`≥ 20 µF / 400 V` per rail) at converter/inverter taps.
- **Transient & Ripple Performance:**
  - **Stored Energy @ `400 V` (`±200 V`):** `2,400 J` (`1,200 J` per half-rail).
  - **Hard `10 kW` Step with Ultra-Slow `3–5 Hz` Outer Loop (`t_ramp = 50 ms`, `ΔE = 250 J`):** `ΔV_400 = ±20.8 V` (`±10.4 V` per rail — stays cleanly inside `360–435 V` trip bounds; for `t_ramp = 20 ms` / `100 J`, `ΔV_400 = ±8.3 V`, staying effortlessly inside the `380–415 V` normal band!).
  - **`120 Hz` Full-Load (`10 kW`) AC Ripple:** `1.1 V pp` (`0.28%` peak-to-peak); `17.7 A RMS` balanced (`~34 A RMS` worst-case asymmetric `120 VAC`).
- **Fail-Safe Discharge (`< 10 s` to `< 48 VDC`):**
  - Per half-rail (`C_top` & `C_bot`): Spring-closed **Normally-Closed (`NC`) `24 VDC` relay** (coil on `AUX_COIL_24V`) + **`100 Ω / 200 W`** pulse-rated aluminum wirewound resistor (`2.15 A` peak, `1,200 J` dump, `1.5 τ = 9.0 s` to `< 48 VDC`, **`0 W` running loss**).
  - Backup high-ohm bleeder: **`33 kΩ / 5 W`** metal-oxide resistor per rail (`1.2 W` continuous) + direct-wired **HV Present LED** (`> 30 VDC`).

### 2.2 `96 VDC` Primary Energy Bus Bank — Zero-Dump-Load Ultra-Stiff Bank

- **Baseline Capacitance:** **`C_96 = 330 mF` (`330,000 µF`, `160 VDC` rated)** (`220 mF` minimum for `≤ 10 kW` PV arrays; `330 mF` baseline covers up to `15 kW` PV arrays with zero dump-load resistor!).
- **Hardware Realization:** `15× 22,000 µF / 160 V` industrial screw-terminal electrolytic cans (e.g., Cornell Dubilier `DCMC` / KEMET `ALS70` series, `~250 A RMS` combined ripple capability) on M4/M6 laminated `LV_BUS+ / LV_BUS-` copper busbars, bypassed by `≥ 40 µF / 250 V` polypropylene film capacitors distributed at converter ports.
- **Transient & Zero-Dump-Load Performance:**
  - **Stored Energy @ `96.0 V`:** `1,520 J`.
  - **Hard `10 kW` Load Step with Slow Outer Loop (`t_ramp = 20 ms`, `ΔE = 100 J`):** `ΔV_96 = ±3.16 V` (stays comfortably inside `91.2–100.8 V` `±5%` normal band! Even at `t_ramp = 30 ms` / `150 J`, `ΔV_96 = ±4.7 V`, still inside `±5%`!).
  - **Worst-Case Solar Load-Dump (Sudden BMS Charge-FET Cutoff at `V_96 = 100.8 V`):**
    - Every per-panel Solar MPPT executes autonomous local hardware/fast-ISR overvoltage curtailment starting at `101.0 V` (`0%` duty within `< 2–5 ms`, zero CAN dependency).
    - Even under a conservative `15 kW` array × `5 ms` curtailment tail (`75 J`): `ΔV = +2.25 V` → peaks at **`103.05 V`** (`5.0 V` below `108.0 V` OV trip!).
    - Even under an extreme `15 kW` × `10 ms` delayed tail (`150 J`): `ΔV = +4.5 V` → peaks at **`105.3 V`** (`2.7 V` below `108.0 V` trip!). **No `96 V` dump-load resistor is required.**
    - *(Future mechanical-inertia exception: if a wind or hydro turbine is ever added to the `96 VDC` bus, that turbine controller must include its own local diversion/dump load.)*
- **Fail-Safe Discharge (`< 10 s` to `< 48 VDC`):**
  - Spring-closed **Normally-Closed (`NC`) `24 VDC` relay** (coil on `AUX_COIL_24V`) + **`33 Ω / 200 W`** wirewound resistor (`3.0 A` peak, `1,520 J` dump, `0.73 τ = 7.9 s` from `100 V` to `< 48 VDC`, `0 W` running loss), backed up by `10 kΩ / 5 W` passive bleeder (`< 1 W`) + Bus-Energized LED.

---

## 3. Soft-Start Pre-Charge Architecture (No Hot-Swap)

Because modules are never hot-swapped under load, cold system startup avoids high-energy pre-charge resistors completely:

1. **Cold `96 VDC` Bus Bring-Up (Battery DAB Soft-Start):**
   - With `96 VDC` at `0 V`, the Coordinator verifies all `Required` modules are `READY` and commands the designated startup Battery Node(s) to close `Q_ISOL` onto the de-energized bus (`ΔV = 0 V`, zero arc).
   - Battery DAB ramps phase shift `φ` from `0°` under constant-current limiting (`e.g., 10–15 A` total), smoothly charging the `330 mF` `96 VDC` bus from `0 V` to `96.0 V` in **`~2.1–3.2 seconds`** with zero resistor heat.
2. **Joining an Already-Live `96 VDC` Bus (e.g., Additional Battery, Boost, Generator, Utility-Tie):**
   - **Additional Battery DABs:** Close nothing until the DAB softly regulates its own internal `96 V` port capacitor to match `V_BUS96` (`ΔV ≤ 1.5 V`), then closes `Q_ISOL` seamlessly (`Q_PRE` optional/redundant for DAB!).
   - **Boost / Generator / Utility-Tie Modules:** Only pre-charge their own **small local module input capacitance** (`~2–5 mF`, storing `< 25 J`), easily handled by a compact `22–50 Ω / 50 W` `Q_PRE` resistor in `< 0.5 s` before closing `Q_ISOL`. (During cold startup, Boost modules may alternatively close `Q_ISOL` at `0 V` and ride up with the Battery DAB soft-start.)
3. **Cold `400 VDC` (`±200 V`) Bus Bring-Up (Boost FB-LLC `PSM` Soft-Start):**
   - Primary Boost FB-LLC ramps `±200 VDC` (`60 mF` per rail) smoothly from `0 V` to `±200.0 V` using high-frequency `PSM` soft-start (`~140–160 kHz`, ramping phase shift `φ: 160° → 0°` under resonant current limiting; e.g., `5 A` DC output charges `30 mF` eff. to `400 V` in `2.4 seconds`).
   - Secondary Boost modules joining a live `400 VDC` bus are naturally blocked from reverse back-feed by their passive SiC Schottky output rectifiers.

---

## 4. `96 VDC` & `400 VDC` Autonomous Voltage-Droop Curves (`DEGRADED_AUTONOMOUS` Mode)

When `COORD_HEARTBEAT` (`0x100`) is absent for `> 500 ms`, active modules seamlessly hold bus stability using deterministic local V-I droop curves with zero communication:

### 4.1 `96 VDC` Energy Bus Droop Map (`91.2 V – 100.8 V` Normal Band)

| `96 VDC` Bus Voltage Band | Autonomous Subsystem Behavior |
| :--- | :--- |
| **`> 108.0 V`** | **Hardware OV Trip (`FLT_COM_BUS96_OV`)** — All sources/converters gate off & open `Q_ISOL`. |
| **`101.0 V – 104.0 V`** | **Solar MPPT Autonomous CV Curtailment:** Linearly derates PV injection from `100%` at `101.0 V` to `0%` by `102.5 V` (`< 2–5 ms` local fast loop); prevents OV on full batteries or BMS cutoff. |
| **`97.0 V – 100.8 V`** | **Battery Fleet Autonomous `CHARGING` Zone:** Battery DABs absorb surplus solar power proportionally (`0%` charge at `97.0 V` → `100%` max charge limit at `100.0 V`), respecting local DALY BMS cell-max derating. |
| **`95.0 V – 97.0 V`** | **Battery Fleet Deadband (`IDLE`):** Prevents battery-to-battery circulating charge/discharge hunting when generation matches load (`96.0 V` center). |
| **`91.2 V – 95.0 V`** | **Battery Fleet Autonomous `DISCHARGING` Zone:** Battery DABs supply deficits proportionally (`0%` discharge at `95.0 V` → `100%` max discharge limit at `92.0 V`), naturally sharing load across parallel batteries by matching droop slope (`~3 V` full-scale droop). |
| **`84.0 V – 91.2 V`** | **Low-Bus Warning / Critical Load-Shed Band:** Batteries at full allowed discharge; non-critical loads shed if configured. |
| **`< 84.0 V`** | **Hardware UV Trip (`FLT_COM_BUS96_UV`)** — Converters isolate cleanly; `AUX_LOGIC_24V` stays alive. |

*(Note: Generator and Grid Interface Nodes do not participate in `DEGRADED_AUTONOMOUS` droop; per [`System_State_MAchine.md`](../architecture/System_State_MAchine.md), they execute controlled ramp-down/cooldown and isolate safely on Coordinator loss.)*

### 4.2 `400 VDC` (`±200 VDC`) Inverter Bus Autonomous Behavior
- **Primary Boost Node:** Holds `400.0 VDC` (`±200.0 V`) with mild `~5 V` full-load droop (`402.5 V` no-load → `397.5 V` at `10 kW`, well inside `380–415 V`).
- **Secondary Boost Nodes:** Enter `Standby` upon Coordinator timeout (`> 500 ms`) so parallel Boost modules never fight over voltage regulation without `20 ms` current-sharing tokens.
