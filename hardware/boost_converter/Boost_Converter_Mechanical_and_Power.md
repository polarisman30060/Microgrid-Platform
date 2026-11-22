# 10 kW 2-Phase FB-LLC Boost Module — Mechanical, Heatsink & Laminated Busbar Blueprint

Status: Rev 0.1 Mechanical & Thermal Baseline (2026-11-22) — Companion to [`Boost_Converter_Architecture.md`](./Boost_Converter_Architecture.md) and [`Boost_Converter_Signal_PCB_Schematic.md`](./Boost_Converter_Signal_PCB_Schematic.md). Designed for **zero high-current PCB traces** (`100%` SOT-227 silicon + M4/M6 copper busbars) and DIY workshop fabrication.

---

## 1. Physical Envelope & 4-Zone Top-Down Layout (`~360 mm × 320 mm`)

All `12×` SOT-227 power modules mount flat onto **one wide grounded extruded aluminum heatsink** (`250 mm × 300 mm` baseplate), flanked by the Magnetics/`C_r` deck and the elevated 2-layer `< 1 A` signal PCB carrying the plugged-in `STM32G474QET6` control card.

```text
  <============================== ~360 mm (Width) ==============================>
  +-----------------------------------------------------------------------------+
  |  [FAN 1: 120mm 24V PWM]  == AIRFLOW DOWN LENGTH ==  [FAN 2: 120mm 24V PWM]  |  ^
  +-----------------------------------------------------------------------------+  |
  |  ZONE 1: PRIMARY 96V SOT-227s |  >= 20mm AIR/G10  |  ZONE 3: SEC +/-200V    |  |
  |  (On Grounded Aluminum HS)    |  ISOLATION MOAT   |  (On Grounded Al HS)    |  |
  |                               |                   |                         |  |
  |  [Q1]===LAMINATED===[Q3]      |  [Cr1: 4x1.5uF]   |  [D1/D2 SOT-227]        |  |
  |  (HS)   96V+ / 96V- (HS)      |  [CT1] -> [Lr1]   |  [D3/D4 SOT-227]        |  |
  |  [Q2]===COPPER BAR==[Q4]      |  +-------------+  |   |                     |  |
  |  (LS)   + 2.2uF MKP (LS)      |  | T1: TDK U93 |  |   +-> [50A Fuse HV+]    | ~320 mm
  |         + 10mF Caps           |  | 6 : 13 : 13 |  |   +-> [HV_MID Bar=PE]   | (Length)
  |  ------ PHASE 1 (5 kW) ------ |  +-------------+  |   +-> [50A Fuse HV-]    |  |
  |  ------ PHASE 2 (5 kW) ------ |  +-------------+  |                         |  |
  |  [Q5]===LAMINATED===[Q7]      |  | T2: TDK U93 |  |  [D5/D6 SOT-227]        |  |
  |  (HS)   96V+ / 96V- (HS)      |  | 6 : 13 : 13 |  |  [D7/D8 SOT-227]        |  |
  |  [Q6]===COPPER BAR==[Q8]      |  +-------------+  |   |                     |  |
  |  (LS)   + 2.2uF MKP (LS)      |  [CT2] -> [Lr2]   |   +-> Output Studs to   |  |
  |                               |  [Cr2: 4x1.5uF]   |       60mF/Rail Bank    |  |
  +-------------------------------+-------------------+-------------------------+  |
  |  ZONE 4: 2-LAYER 1oz SIGNAL & GATE-DRIVE PCB (Mounted on 20mm Standoffs)    |  |
  |  [J1/J2 -> STM32 Control Card + Waveshare 1.3" LCD] | [4x UCC21520 + Bias]  |  |
  |  [3x AMC1311] | [Fault Latch] | [Q_PRE / Q_ISOL Drivers] | [AUX 24V In]     |  v
  +-----------------------------------------------------------------------------+
```

*Mechanical Assembly Note:* A fiberglass (`G10/FR4`, `3 mm` thick) vertical isolation baffle stands between Zone 1 (`96 V` primary) and Zone 3 (`±200 V` secondary) across the Magnetics Deck so no loose wire or tool slip can bridge primary and secondary domains (`≥ 20 mm` physical creepage/clearance — more than double the `8 mm` reinforced target).

---

## 2. SOT-227 Mounting Pattern & Heatsink Thermal Budget

### 2.1 SOT-227 (miniBLOC) Footprint & Hardware
- **Body Size:** `38.2 mm × 25.4 mm`; mounting ears **`2× M4` on `30.0 mm` centers**; top M4 screw terminals on **`14.9 mm × 12.7 mm`** centers.
- **Recommended Grid Pitch:** **`45.0 mm` × `35.0 mm`** center-to-center between adjacent packages (leaves `~7–10 mm` gap for socket wrench access, air circulation, and twisted-pair gate routing).
- **Baseplate Isolation:** Both `IXFN300N20X3` (or `IXFN230N20T`) and `GHXS100B065S-D3` feature internally ceramic-isolated (`≥ 2.5–3 kV`) copper baseplates—apply a thin, even layer of non-conductive thermal grease (e.g., Dow Corning `340` / Arctic `MX-4`) and torque mounting M4 screws to **`1.3–1.5 N·m`**. Torque top M4 busbar terminal screws to **`1.1–1.3 N·m`** (always use spring lock washers / Belleville washers so copper thermal cycling never loosens joints!).

### 2.2 Thermal Budget (`10.0 kW` Continuous / `15.0 kW` Peak Surge)

| Heat Source | Qty | Loss @ `10 kW` Cont. | Loss @ `15 kW` Peak | Heat Path | Junction-to-Heatsink `ΔT` |
| :--- | :---: | :---: | :---: | :--- | :--- |
| **Primary SOT-227 MOSFETs (`IXFN300N20X3`)** | 8 | `~105 W` (`13.1 W` ea) | `~190 W` (`23.8 W` ea) | Directly into Al heatsink | `~3.0 °C` (`R_thJC ≈ 0.18 °C/W`) |
| **Secondary Dual SiC SOT-227 (`GHXS100B065S`)** | 4 | `~72 W` (`18.0 W` ea) | `~113 W` (`28.3 W` ea) | Directly into Al heatsink | `~9.0 °C` (`R_thJC ≈ 0.45 °C/W`) |
| **Total Heatsink Thermal Load** | **12** | **`~187 W`** | **`~323 W`** | **Forced-air extruded HS** | **Target `T_hs ≤ 65 °C` @ `40 °C` amb** |
| **Transformers (`T1`/`T2`), `L_r1`/`L_r2`, `C_r`, Bars** | Lot | `~85 W` | `~130 W` | Direct forced-air duct | `95 °C` NC snap discs on `T1`/`T2`/`L_r` |
| **Total Module Heat Rejection** | — | **`~272 W` (`~97.3%` eff.)** | **`~453 W`** | **2× `120 mm` `24 V` PWM fans** | — |

- **Heatsink Sizing:** For `ΔT_hs = 25 °C` (`T_hs = 65 °C` at `40 °C` ambient) with `187 W` continuous semiconductor dissipation:
  $$R_{\text{th,hs}} \le \frac{25\,^\circ\text{C}}{187\text{ W}} = \mathbf{0.134\,^\circ\text{C/W}}$$
  - Select an extruded or bonded-fin aluminum heatsink **`~250 mm wide × 300 mm long × 40–50 mm fin height`** (typical forced-air `R_th ≈ 0.08–0.11 °C/W` at `3–4 m/s` air velocity), mounted with a simple sheet-metal shroud so all fan airflow passes strictly through the fins and over the `U93` magnetics deck.
- **Fan Sizing (`2× 120 mm` `24 VDC` PWM, e.g., `Delta AFB1224VH` / `Sunon MEC0382V1`):**
  - Combined `~120–160 CFM` (`2× 60–80 CFM`) driven by `FAN_PWM` (`TIM4_CH1`, `J1:30`) with tachometer monitored on `FAN_TACH` (`J1:31`).
  - Bulk air temperature rise across `300 W` at `120 CFM`: $\Delta T_{\text{air}} = \frac{1.76 \times 300}{120} = \mathbf{4.4\,^\circ\text{C}}$ (only `8.8 °C` even if one fan fails!).

---

## 3. Laminated Copper Busbar Stackup & Fabrication Guide

### 3.1 Primary `96 V+ / 96 V-` Laminated DC-Link Spine (The ONLY Low-Inductance Busbar!)
- **Copper Stock:** Two bars of **`1/8" × 1.00"` (`3.2 mm × 25.4 mm`, `C110` electrolytic copper, `80.6 mm²` cross-section)** — rated `> 200 A` continuous (`~1.4 A/mm²` at `114 A`, `< 5 °C` self-heating!).
- **Stackup (Top to Bottom):**
  1. **Top Layer:** `96 V+` (`LV_BUS+`) copper bar (`3.2 mm` thick) — lands on Drain (`D`) M4 studs of High-Side FETs (`Q1`, `Q3`, `Q5`, `Q7`) and `+` terminals of `2× 10,000 µF / 160 V` local screw-terminal electrolytics.
  2. **Insulator Layer:** **`0.5 mm` (`20 mil`) Nomex 410 or `G10/FR4` sheet**, cut **`6 mm` wider on all edges (`37 mm` wide)** than the copper bars to guarantee generous edge creepage!
  3. **Bottom Layer:** `96 V-` (`LV_BUS-`) copper bar (`3.2 mm` thick) — lands on Power Source (`S`) M4 studs of Low-Side FETs (`Q2`, `Q4`, `Q6`, `Q8`) and `-` terminals of the local electrolytics.
- **Clearance Holes & Insulating Shoulder Washers:** Wherever an M4 bolt drops through the top bar to reach the bottom bar (or vice-versa), drill an oversized **`10 mm` clearance hole** in the bypassed bar + insert a **`G10`/nylon insulating shoulder bushing** so the bolt shank can never short the two bars!
- **Per-Leg Snubber Film Caps (`4×`):** Bolt one **`2.2 µF / 400 V` screw-lug MKP film capacitor** directly across the `96 V+` and `96 V-` M4 studs at each of the four half-bridge legs (`Q1/Q2`, `Q3/Q4`, `Q5/Q6`, `Q7/Q8`). Combined laminated loop inductance `< 15–20 nH` keeps `41 A` turn-off overshoot `< 20 V` on `200 V` MOSFETs!

### 3.2 Forgiving AC Resonant Bars & Screw-Lug `C_r` Carrier
- **Switch-Node Straps (`SW_A`, `SW_B`, `SW_C`, `SW_D`):** Short `1/8" × 3/4"` (`3.2 × 19 mm`) copper straps connect `Q1_S → Q2_D` (`SW_A`), `Q3_S → Q4_D` (`SW_B`), etc.
- **Screw-Lug `C_r` Bank (`4× 1.5 µF` MKP per phase = `6.0 µF`):** Two parallel `1/8" × 3/4"` copper collector strips bolt directly across the `4×` screw-lug film capacitors (`SW_A` strip on one side, `CR_NODE` strip on the other) supported on `M4` ceramic/nylon standoffs.
- **HF CT Pass-Through:** Route the single heavy `26 AWG` bundled primary lead (or narrow copper bar tab) from `CR_NODE` straight through the center window of the `1:200` toroidal HF CT (`I_res_Ph1` / `I_res_Ph2`) into `L_r1` / `L_r2` → `T1` / `T2` → `SW_B` / `SW_D`. Remember: **all inductance in this AC loop is tuned into `L_r = 2.08 µH` via the LCR measure-and-shim procedure!**

### 3.3 Secondary `±200 VDC` Output Busbars (`HV+`, `HV_MID`, `HV-`)
- Use three `1/8" × 1/2"` (`3.2 × 12.7 mm`, `40 mm²` — handles `38 A` peak effortlessly) copper bars mounted on `10 mm` ceramic/G10 standoffs (`≥ 10 mm` air spacing between `HV+`, `HV_MID`, and `HV-`):
  - `HV+` bar collects `D1/D2` (Ph1) & `D5/D6` (Ph2) cathodes → **`50 A / 500 VDC` semiconductor fuse** → `HV+` output stud.
  - `HV_MID` bar collects `T1_CT` & `T2_CT` center-tap lugs directly → **unfused** `HV_MID` stud (routes to centralized `60 mF/rail` cap bank single-point `PE / Neutral` bond).
  - `HV-` bar collects `D3/D4` (Ph1) & `D7/D8` (Ph2) anodes → **`50 A / 500 VDC` semiconductor fuse** → `HV-` output stud.

---

## 4. Transformer (`TDK U93/76/30`) & `L_r` Clamping Rules
- **NEVER run a steel bolt through the inside of a ferrite core window** (it acts as a shorted 1-turn secondary and will glow red-hot in seconds!).
- Clamp each `TDK U93/76/30` U-U pair externally using two **`6 mm` thick `G10/FR4` fiberglass end-plates** and four external stainless threaded rods positioned **outside** the ferrite core perimeter, sandwiching the calibrated **`1.52 mm` FR4/Nomex gap shims** cleanly across both U-leg mating faces.
- Bolt the `95 °C` normally-closed (`NC`) thermal snap disc onto a small aluminum clip pressed firmly against the outer Kapton wrap of the winding (never touching bare copper!).
