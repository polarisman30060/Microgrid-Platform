# Battery Module Galvanic Isolation & Topology Trade-Off Study

Status: Draft Engineering Study & Baseline Selection (Rev 0.1). Recommends migrating the `2.5 kW` Battery Charger/Discharger stage from a non-isolated 4-switch buck-boost to an **Isolated Bidirectional Single-Phase Dual Active Bridge (DAB)**.

---

## 1. Why Investigate Battery Isolation Now?

With the microgrid moving to:
- A **galvanically isolated `400 VDC` (`±200 VDC`) Inverter Bus** (midpoint bonded to `PE` / AC Neutral), and
- A **negative-grounded `96 VDC` Energy Bus** (`LV_BUS-` bonded to `PE` at one single-point DC GFDI lug so the `96 V` bus never floats),

retaining a **non-isolated** 4-switch buck-boost converter between each `48 V` battery pack (`40.0 V – 58.4 V`) and the grounded `96 VDC` bus creates three concrete engineering hazards:

1. **DALY BMS Low-Side Disconnect Bypass Hazard:** Commercial `16S` DALY Smart BMS units place their charge/discharge protection MOSFETs in the **negative leg (`B-` to `P-`)**. When `LV_BUS-` is referenced to Earth (`PE`), a non-isolated converter ties `P-` directly to `LV_BUS-` (`PE`). Any accidental insulation pinch or conductive fault between a battery rack's `B-` terminal (or metal cell casing / rack tray) and Earth (`PE`) **shorts around the DALY BMS MOSFETs**, silently defeating cell-level over-discharge, over-charge, and short-circuit disconnect!
2. **Multi-Pack Ground Loops & Chemistry Mixing:** With up to 5 parallel Battery Nodes (potentially mixing LiFePO4, NMC, Lead-Acid, or Sodium-Ion racks), a shared non-isolated negative bus couples switching noise, BMS reference potentials, and fault currents between packs.
3. **Uncontrolled Fault Path:** In a non-isolated 4-switch buck-boost, a high-side switch failure or `96 V` bus fault can pull uncontrolled fault current from the `48 V` pack through MOSFET body diodes until fuses or `Q_ISOL` open under load.

Galvanically isolating each Battery Charger/Discharger converter removes all three hazards, keeps every `48 V` battery pack and its DALY BMS completely floating inside its own safe `< 60 VDC` SELV/ELV domain, and aligns with the isolated `48 V → 24 VDC` auxiliary lifeline (`AUX_LOGIC_24V` / `AUX_COIL_24V`) already derived from each pack.

---

## 2. Candidate Topologies (`40.0–58.4 V` Pack ↔ `91.2–100.8 V` Bus, `2.5 kW`)

Total voltage conversion ratio range across both ports:
- Minimum ratio: `91.2 V / 58.4 V = 1.56`
- Nominal ratio: `96.0 V / 48.0 V = 2.00` (exact `1:2` turns ratio!)
- Maximum ratio: `100.8 V / 40.0 V = 2.52` (`1.62:1` total gain span)

| Criterion | 1. Single-Phase Dual Active Bridge (DAB) | 2. Bidirectional CLLC Resonant | 3. Isolated Current-Fed Push-Pull / Dual Half-Bridge | 4. Non-Isolated 4-Switch Buck-Boost (Legacy) |
| :--- | :--- | :--- | :--- | :--- |
| **Galvanic Isolation** | **Yes** (`≥ 2.5–4 kVrms` HF xfmr) | **Yes** (`≥ 2.5–4 kVrms` HF xfmr) | **Yes** (HF xfmr) | **No** (shares `LV_BUS-` / `PE`) |
| **Bidirectional Transition** | **Seamless** — simply reverse phase shift `+φ ↔ −φ` (zero mode-switch glitch!) | Complex — requires changing SR timing & PFM direction | Asymmetric charge (voltage-fed) vs. discharge (current-fed) | Seamless duty sweep |
| **Fit for `1.62:1` Voltage Span** | **Excellent** — fixed `f_sw` (`60–80 kHz`), EPS/DPS modulation preserves ZVS across wide voltage ratio | **Poor** — `1.62:1` span demands wide `f_sw` swing (`> 2:1`), high circulating current at extremes, complex bidir. SR | Moderate — primary voltage spikes on current-fed turn-off require active clamp or lossy snubbers | Excellent |
| **Magnetics Complexity** | **Simple** — 1 HF transformer (`1:2`) + 1 small AC transfer inductor `L_k` (no giant DC-biased toroid!) | Complex — symmetric `L_r1`, `C_r1`, `L_r2`, `C_r2`, tight tolerance sensitivity | 1 HF transformer + 1 large DC input inductor (`65 A` DC bias) | 1 large DC-biased Kool Mµ toroid (`37 µH / 69 A`) |
| **Power Semiconductors** | 8 MOSFETs (`4× 100 V` + `4× 150 V`) + 4 dual drivers | 8 MOSFETs + 4 dual drivers + dual high-current film cap banks | 4–6 MOSFETs + active clamp FET/cap | 4 MOSFETs (`150 V`) + 2 dual drivers |
| **STM32 Control Card Fit** | **8 `HRTIM1` outputs** (`CHA1/2..CHD1/2` — **already on `J2:1..12`!**) | 8 `HRTIM1` outputs (tight real-time SR sensing needed) | 4–6 `HRTIM1` outputs | 4 `HRTIM1` outputs |
| **Short-Circuit & Soft-Start** | **Native** — series `L_k` + HF xfmr block DC; ramp `φ` from `0°` | Good (frequency foldback), harder startup in reverse | Needs startup pre-charge / clamp control | Body-diode fault path exists |
| **Estimated Efficiency (`2.5 kW`)** | **`95.5–97.0%`** | `96.5–97.5%` (only near `48 V` nom; drops at `40 V`/`58 V`) | `93–95%` | `96–97%` |

---

## 3. Decision: Single-Phase Dual Active Bridge (DAB) Selected

**Selected Baseline:** **2.5 kW Isolated Bidirectional Single-Phase Dual Active Bridge (DAB)** (`1:2` turns ratio, fixed `75 kHz` switching frequency, Extended/Dual Phase-Shift modulation).

### Why DAB Wins Decisively for This Microgrid
1. **Natural `1:2` Ratio Match:** Nominal `48 V ↔ 96 V` sits right at unity voltage conversion ratio (`M = n·V_bus / V_bat = 1.0`), minimizing reactive circulating current and achieving ZVS across the majority of the `40–58.4 V` LiFePO4 / Lead-Acid operating band.
2. **Instantaneous Charge ↔ Discharge Reversal:** In `DEGRADED_AUTONOMOUS` droop mode, a Battery Node must smoothly swing from absorbing Solar excess (`CHARGING`) to supporting a step load (`DISCHARGING`) in milliseconds. In a DAB, transferred power is simply `P = (n · V_bat · V_bus / (2 · f_sw · L_k)) · d(1 − |d|)`, where `d = φ / π` smoothly crosses zero from `-d` to `+d` with zero hardware relay or mode-switch discontinuity.
3. **Zero STM32 Control Card Changes:** Uses `HRTIM1` `CHA1/2`, `CHB1/2` (48 V pack full-bridge) and `CHC1/2`, `CHD1/2` (96 V bus full-bridge) — the exact same 8 `J2:1..12` PWM pins used by the Boost FB-LLC module!
4. **Inherent Isolation & Fault Blocking:** Completely isolates the `48 V` battery pack and DALY BMS from grounded `LV_BUS-` (`PE`), eliminates the `B-` bypass hazard, and provides natural DC short-circuit blocking when gates disable.

---

## 4. First-Pass 2.5 kW DAB Sizing Skeleton

| Parameter | Preliminary Value | Sizing Basis |
| :--- | :--- | :--- |
| **Rated Bidirectional Power (`P_rated`)** | `2.5 kW` continuous (`3.0 kW` peak) | `52 A` @ `48 V` (`62.5 A` @ `40 V`); `26 A` @ `96 V` |
| **Switching Frequency (`f_sw`)** | `75.0 kHz` (fixed) | Compact ferrite magnetics, low core loss, easy `HRTIM1` resolution |
| **Transformer Turns Ratio (`N_pri : N_sec`)** | `1 : 2` (e.g., `6 T : 12 T`) | Centered at `48 V ↔ 96 V` (`M = 1.0`) |
| **Series Transfer Inductance (`L_k`, 48 V side)** | `~2.2 – 2.8 µH` (leakage + external `L_k`) | Delivers `2.5 kW` at `φ ≈ 22°–28°` (`48 V → 96 V`), `3.0 kW` peak at `φ ≈ 35°`, well below `90°` max power limit (`P_max ≈ 5.5 kW`) |
| **48 V Pack-Side Bridge (`Q1–Q4`)** | `4× 100 V` low-`R_DS(on)` MOSFETs (e.g., `Infineon IPW036N10N5` / `TO-247`, `≤ 4 mΩ`) | `V_bat,max = 60 V` (`100 V` rating gives `40 V` margin); `~42 A RMS` per switch at `40 V` |
| **96 V Bus-Side Bridge (`Q5–Q8`)** | `4× 150 V` MOSFETs (e.g., `Infineon IPW110N15N5` / `TO-247`, `9.7 mΩ`) | `V_bus,max = 108 V` (`150 V` rating gives `42 V` margin); `~21 A RMS` per switch |
| **Isolated Gate Drivers (`4×` dual)** | `4× TI UCC21520` (`5.7 kVrms`, `4 A / 6 A`) | 2 on pack side (`CHA1/2`, `CHB1/2`), 2 on 96 V bus side (`CHC1/2`, `CHD1/2`), shared `GATE_EN` |
| **Modulation Strategy** | Single-Phase-Shift (`SPS`) at medium/high load; Extended/Dual Phase-Shift (`EPS`/`DPS`) at light load & extreme `V_bat` (`40 V` / `58.4 V`) | Eliminates light-load hard switching and minimizes reactive circulating current |
| **DC Blocking Capacitor (Optional)** | High-current `10–20 µF` film cap in series with transformer primary | Guarantees zero flux walking / transformer saturation from tiny gate-drive timing asymmetry |
| **Current & Voltage Sensing** | `I_tank` (high-bandwidth Hall/CT on `L_k` for peak OC & flux-balance), `I_bat`, `I_96`, `V_bat` (`AMC1311`), `V_96` (`AMC1311`), `T_hs` | **6–7 ADC channels** (`J1:9..15`), all galvanically isolated from `GND_CTRL` |
| **Hardware Faults** | `HRTIM1_FLT1` (`J2:19`): `I_tank` OC, `I_bat`/`I_96` OC, bridge OV/driver fault; `HRTIM1_FLT3` (`J2:20`): magnetics snap-disc / E-Stop | Asynchronous `< 1 µs` gate shutdown of all 8 MOSFETs |
| **Pack-Side & Bus-Side Disconnects** | DALY BMS + Class-T pack fuse on `48 V` side; standardized `Q_PRE` + `Q_ISOL` (`24 VDC`) + DC fuse on `96 V` side | Full dual-sided isolation and pre-charge (`ΔV ≤ 1.5 V`) |

---

## 5. Open Engineering Verification Items Before Battery DAB Prototype

1. Confirm exact `L_k` value (`2.2–2.8 µH`) and `EPS`/`DPS` lookup table in PLECS/LTspice across the full `40.0–58.4 V` × `91.2–100.8 V` corner matrix to verify soft-switching boundaries.
2. Decide whether to scale future Battery DAB modules to `5.0 kW` (using parallel FETs or `SOT-227` bridges) while keeping the identical `8-PWM` `STM32` control-card pinout.
3. Verify `DC` blocking capacitor sizing vs. active flux-balancing control to prevent transformer saturation during fast charge-to-discharge transients.
