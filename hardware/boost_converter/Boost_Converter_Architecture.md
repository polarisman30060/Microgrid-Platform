# 10 kW 2-Phase Interleaved Full-Bridge LLC Boost Module — Engineering Design Skeleton

Status: Rev 0.3 Hand-Buildable SOT-227 + Busbar Baseline (2026-11-22) — Analytically verified for `±15–20%` DIY magnetics/parasitic fudge factor via [`Simulations/fbllc_verify.py`](./Simulations/fbllc_verify.py) and [`Simulations/Boost_FBLLC_5kW_Phase.cir`](./Simulations/Boost_FBLLC_5kW_Phase.cir). See companion fabrication packages: [`Boost_Converter_Mechanical_and_Power.md`](./Boost_Converter_Mechanical_and_Power.md), [`Boost_Converter_Signal_PCB_Schematic.md`](./Boost_Converter_Signal_PCB_Schematic.md), [`Boost_Converter_Magnetics_Build_Guide.md`](./Boost_Converter_Magnetics_Build_Guide.md), [`Boost_Converter_Hardware.md`](./Boost_Converter_Hardware.md), and the DigiKey-sourced [`Boost_Converter_BOM.md`](./Boost_Converter_BOM.md).

---

## 1. Design Philosophy: Zero High Power on PCBs & Forgiving DIY Magnetics

This module is engineered specifically to be **hand-built and hand-wound** without needing heavy-copper multi-layer power PCBs or fragile fine-strand factory Litz wire:

1. **100% SOT-227 Silicon + M4 Bolted Copper Busbars:** All `12×` power semiconductor modules (`8×` primary MOSFETs + `4×` dual secondary SiC Schottky rectifiers) use baseplate-isolated **SOT-227 (miniBLOC)** packages bolted flat to **one grounded aluminum heatsink** (no sil-pads needed). All main currents (`96 V` input → `Q1–Q8` → screw-lug `C_r` → `L_r` → `T1/T2` → `D1–D8` → `±200 V` output) travel exclusively on **M4/M6 copper busbars and crimped ring lugs**.
2. **Simple `1 oz` 2-Layer Signal & Gate-Drive PCB (`< 1 A`):** The PCB carries **zero high-power traces**—only `J1`/`J2` STM32 control-card sockets, `UCC21520DW` gate drivers (wired via short **twisted pairs** to each SOT-227's dedicated **Gate (`G`) and Kelvin Source (`KS`)** screw terminals), `AMC1311` voltage isolators, CT/Hall sensor conditioning, `< 300 ns` hardware fault latch, and `24 V` contactor drivers.
3. **Forgiving `45.0 kHz` Resonant Tank (`±15–20%` Fudge Factor):** Lowering `f_r` from `65 kHz` to **`45.0 kHz`** (`38–65 kHz` normal band; still well above `20 kHz` audible range) raises `L_r` to **`2.08 µH`** so busbar strays and hand-wound transformer leakage (`~0.8–1.3 µH`) simply become part of `L_r`, dialed in precisely with a small external gapped inductor and a `$30` LCR meter.
4. **Easy DIY `26 AWG` Wire + Giant `TDK U93/76/30` Ferrite Cores:** At `45 kHz`, copper skin depth is **`0.311 mm`**, larger than standard **`26 AWG`** wire radius (`0.202 mm`). You can twist tough `26 AWG` magnet-wire bundles with a cordless drill and wind them easily inside the giant `1,520 mm²` window (`11%` fill!) of a **`TDK U93/76/30` (`N87`)** U-core pair with huge `≥ 8 mm` Kapton/Nomex insulation margins.

---

## 2. Verified Electrical Specification

| Parameter | Nominal | Normal Window | Peak / Trip Bounds | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Input Voltage (`V_BUS96`)** | `96.0 VDC` | `91.2 V – 100.8 V` (`±5%`) | `< 84.0 V` UV / `> 108.0 V` OV | `330 mF` bank; `LV_BUS-` bonded to `PE` via `1–5 A` DC GFDI |
| **Output Voltage (`V_HV_TOTAL`)** | `400.0 VDC` | `380.0 V – 415.0 V` | `< 360.0 V` UV / `> 435.0 V` OV | Centralized `60 mF`/rail split bank (`±200 V`); `HV_MID = N = PE` |
| **Split-Rail Balance (`|V_HV+ − |V_HV-||`)** | `0.0 V` | `≤ 10.0 V` warn | `> 20.0 V` fault trip | Center-tapped SiC full-bridge self-balances with zero DC core bias |
| **Power Rating** | `10.0 kW` (`2 × 5.0 kW`) | `0 – 10.0 kW` cont. | `15.0 kW` (`2 × 7.5 kW`) peak surge | Single-phase shedding below `~3.5 kW` (`35%`), re-entry at `4.5 kW` |
| **Input / Output DC Current** | `107 A` in / `25.0 A` out | `113 A` in / `26.3 A` out | `170 A` in / `37.5 A` out (`15 kW`) | `200 A` Class-T on `LV_BUS+`; 2-pole `50 A / 500 VDC` on `HV+`/`HV-` |
| **Resonant Frequency (`f_r`)** | **`45.0 kHz`** | **`38 – 65 kHz`** PFM band | **`< 28 kHz`** HW/SW capacitive floor | `90°` primary phase shift → `180 kHz` combined output ripple cancellation |
| **Target Efficiency** | **`~97.0%`** | `~250–320 W` total loss @ `10 kW` | `~420 W` @ `15 kW` peak | ZVS primary turn-on + ZCS secondary turn-off |
| **Galvanic Isolation** | **`≥ 4.0 kVrms`** | Reinforced (`≥ 8 mm` crp/clr) | — | Primary `96 V` ↔ Secondary `±200 V` ↔ `GND_CTRL` ↔ `CAN` |

---

## 3. Power-Stage Topology & Zero-PCB-Power Busbar Architecture

```text
  96 VDC ENERGY BUS (330 mF Bank; LV_BUS- -> 1-5A DC GFDI -> PE)
     |
  [200A Class-T Fuse] -> [Q_PRE (22R 50W)] || [Q_ISOL (EV200/GX14)] -> [I_IN_96V Busbar Hall]
     |                                                                          |
     +===== LAMINATED 96V+ / 96V- COPPER BUSBAR (1/8"x1" + 0.5mm Nomex) =======+
     |      (2x 10,000uF/160V Screw-Term Caps + 2.2uF/400V Leg Snubbers)        |
     |                                                                          |
     v  PHASE 1 (5 kW, 0 deg)                                                   v  PHASE 2 (5 kW, 90 deg)
  +-----------------------------+                                            +-----------------------------+
  | 4x SOT-227 IXFN300N20X3     |                                            | 4x SOT-227 IXFN300N20X3     |
  | Leg A: CHA1/2  Leg B: CHB1/2|                                            | Leg C: CHC1/2  Leg D: CHD1/2|
  | (G/KS twisted pairs to PCB) |                                            | (G/KS twisted pairs to PCB) |
  +--------------+--------------+                                            +--------------+--------------+
                 | [Thru-Hole HF CT I_RES_PH1 on Copper Bar]                                | [Thru-Hole HF CT I_RES_PH2]
                 v                                                                          v
  [Cr1: 6.0uF Screw-Lug MKP]--[Lr1: 2.08uH total]                            [Cr2: 6.0uF Screw-Lug MKP]--[Lr2: 2.08uH total]
                 |                                                                          |
          +------+-----+  T1: TDK U93/76/30 (N87)                                    +------+-----+  T2: TDK U93/76/30 (N87)
          | Pri: 6T    |  1.52 mm 2-leg shim -> Lm = 12.5 uH                         | Pri: 6T    |  1.52 mm 2-leg shim -> Lm = 12.5 uH
          | (26 AWG)   |  ====== >= 4 kVrms REINFORCED BARRIER ======                | (26 AWG)   |  ====== >= 4 kVrms REINFORCED BARRIER ======
          | Sec1: 13T  |---> [2x Dual SOT-227 SiC] -> [50A Fuse] -> HV+ (+200 VDC) <-| Sec1: 13T  |---> [2x Dual SOT-227 SiC] -> [50A Fuse]
          | CT         |------------------------------------------> HV_MID (0V=N=PE)<| CT         |------------------------------------+
          | Sec2: 13T  |---> [2x Dual SOT-227 SiC] -> [50A Fuse] -> HV- (-200 VDC) <-| Sec2: 13T  |---> [2x Dual SOT-227 SiC] -> [50A Fuse]
          +------------+                                                             +------------+
                                                        |
                       =================================+=================================
                       CENTRALIZED 400 VDC SPLIT BULK CAP BANK (60 mF C_top + 60 mF C_bot)
                       * Single-Point HV_MID -> PE (AC Neutral Bond)
                       * Dual-Gated NC 24V Discharge Relays (AUX_COIL_24V + CONT_CMD3)
                         + 100 Ohm / 200 W Resistors (< 9 s to < 48 VDC, 0 W running loss)
                       ===================================================================
```

### Critical Busbar Rules (Only ONE Loop Must Be Tight!)
1. **Tight Loop — Laminated `96 V+ / 96 V-` DC-Link Busbars:** Stack two `1/8" × 1"` (`3.2 × 25 mm`, `80 mm²`) copper bars flat face-to-face separated by a **`0.5 mm` (`20 mil`) Nomex or G10 FR4 insulator sheet** (`> 10 kV` dielectric, clearance holes around opposite-polarity M4 bolts). This cuts commutation loop inductance below **`< 15–20 nH`**. Bolt one **`2.2 µF / 400 V` screw-lug MKP snubber capacitor** directly across the `96 V+ / 96 V-` M4 studs at each half-bridge leg (`4×` total) so `41 A` turn-off spikes stay `< 20 V` on `200 V` MOSFETs.
2. **Forgiving Loop — AC Resonant Path (`SW_A → C_r → L_r → T1 → SW_B`):** Any stray inductance in the copper bars, screw-lug `C_r` bank, or hand-wound transformer leads **simply adds to `L_r`!** You do not need to obsess over minimizing inductance here—just tune it out with the external `L_r` core using an LCR meter!

---

## 4. Verified Tank Corner Matrix & `±15–20%` DIY Fudge-Factor Proof

Per `5 kW` phase (`N_p : N_s1 : N_s2 = 6 : 13 : 13`, `C_r = 6.00 µF`, `L_r = 2.08 µH`, `L_m = 12.50 µH`, `L_n = 6.01`, `f_r = 45.05 kHz`, `Z_r = 0.589 Ω`):

| Operating Corner (Per `5 kW` Phase) | Required Gain `M` | Peak Available `M_pk` | Switching Freq `f_sw` | Capacitive Floor `f_pk` | ZVS Margin | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Nominal `96.0 V → 400 V @ 5.0 kW`** | `0.969` | `1.231` | **`49.6 kHz`** | `22.7 kHz` | `+26.9 kHz` | **PASS** |
| **Min Normal `91.2 V → 400 V @ 5.0 kW`** | `1.020` | `1.231` | **`42.5 kHz`** | `22.7 kHz` | `+19.8 kHz` | **PASS** |
| **Min Normal `91.2 V → 415 V @ 5.0 kW`** | `1.058` | `1.291` | **`38.4 kHz`** | `21.7 kHz` | `+16.7 kHz` | **PASS** |
| **Max Normal `100.8 V → 400 V @ 5.0 kW`** | `0.923` | `1.231` | **`57.4 kHz`** | `22.7 kHz` | `+34.7 kHz` | **PASS** |
| **Max Normal `100.8 V → 380 V @ 5.0 kW`** | `0.877` | `1.164` | **`64.7 kHz`** | `24.6 kHz` | `+40.0 kHz` | **PASS** |
| **Peak Surge `91.2 V → 400 V @ 7.5 kW` (`15 kW` tot)** | `1.020` | `1.056` | **`42.3 kHz`** | `33.3 kHz` | **`+9.0 kHz`** | **PASS** |
| **UV Fault Edge `84.0 V → 400 V @ 5.0 kW`** | `1.107` | `1.231` | **`33.6 kHz`** | `22.7 kHz` | `+10.9 kHz` | **PASS** |

### Hand-Winding & Parasitic Stress Test (`91.2 V → 400 V`)
- **Loose hand-wind (`L_r +15%`, `L_m −15%`, `C_r +5%`):** `f_r = 41.0 kHz`, still passes `10 kW` & `13 kW` surge (`f_sw = 39.2 kHz`, `+14 kHz` ZVS margin).
- **Tight hand-wind (`L_r −15%`, `L_m +15%`, `C_r −5%`):** `f_r = 50.1 kHz`, still passes `10 kW` & `13 kW` surge (`f_sw = 45.9 kHz`, `+11 kHz` ZVS margin).
- **Extra busbar stray (`L_r +20%`):** `f_r = 41.1 kHz`, still passes `10 kW` & `13 kW` surge (`f_sw = 39.1 kHz`, `+12 kHz` ZVS margin).

**Primary Stresses (`5 kW` / `96 V`):** `I_in,dc = 53.8 A`, `I_m,pk = 41.0 A`, `I_r,rms = 64.3 A`, `I_sw,rms = 45.5 A` per SOT-227 FET, `V_Cr,pk = 53.5 V`.

---

## 5. DIY Hand-Wound Magnetics Guide (`TDK U93/76/30` + `26 AWG` Bundles)

### 5.1 Main Isolation Transformer (`T1`, `T2` — Per `5 kW` Phase)
- **Core:** **1× TDK/Epcos `U93/76/30` (`N87` ferrite)** (two `U93/76/30` halves = one closed U-U pair; `A_e = 840 mm²`, `V_e = 297 cm³`, giant window **`A_w = 1,520 mm²`**!).
- **Flux & Core Loss:** `B_pk = 105.7 mT` at `45 kHz / 96 V` (`~6 W` core loss), `166.7 mT` worst-case at `30 kHz / 100.8 V` (far below `350 mT` saturation).
- **Air Gap (`L_m = 12.50 µH`):** Total gap `3.04 mm` → place a **`1.52 mm` uniform FR4/Nomex spacer across BOTH mating faces** of the U-U core pair and clamp with fiberglass/non-magnetic clamping plates (never pass a conductive metal bolt through the core window!).
- **Primary Winding (`6 Turns`, `~64 A RMS`):**
  - Skin depth at `45 kHz` = `0.311 mm` → `26 AWG` radius = `0.202 mm` (negligible skin effect!).
  - Build a **`15 mm²`** primary bundle using **`4` parallel twisted ropes of `30 × 26 AWG`** heavy-enameled magnet wire (`120` strands total; `~4.3 A/mm²`) — twist each 30-strand rope gently with a cordless drill, tape into a flat ribbon with Kapton, wind **6 neat turns** onto a custom G10/FR4 sleeve on Leg 1, strip ends in a solder pot, and crimp straight into **M4/M6 copper ring lugs**.
- **Reinforced Insulation Barrier (`≥ 4 kVrms`):** Wrap **4 overlapping layers of `2 mil` Kapton tape + `6 mm` Nomex side margins** (`≥ 10 mm` creepage/clearance — trivial inside the `1,520 mm²` U93 window, where total copper fills only **`11%`**!).
- **Secondary Winding (`13 T + 13 T` Center-Tapped, `~14 A RMS` per half):**
  - Build two matched **`3.2 mm²`** ropes of **`25 × 26 AWG`** (sleeved in PTFE/Nomex tubing or using Triple-Insulated Wire) and wind **bifilar (`Sec1` and `Sec2` side-by-side together for 13 turns)** directly over the insulated primary on Leg 1 so leakage between `CT–A` and `CT–B` is `< 0.5 µH` (tight `±200 V` cross-regulation). Connect end of `Sec1` to start of `Sec2` as **`CT` (`HV_MID`)**.
- **Thermal Snap Disc:** Clamp a `95°C` normally-closed (`NC`) bimetallic snap disc against the winding outer wrap (series into `HRTIM1_FLT3`).

### 5.2 External Tunable Resonant Inductor (`L_r1`, `L_r2`) & "Measure-and-Shim" Calibration
1. **Build `T1` and `T2` first**, bolt them onto the busbars with `C_r` bypassed temporarily, **short `Sec1` & `Sec2` together**, and measure the total loop inductance (`L_leakage + L_busbar`) at the `Q1–Q4` bridge terminals with an LCR meter (`@ 10–100 kHz`). Expect **`~0.8 – 1.3 µH`**.
2. Subtract your measured loop inductance from **`2.08 µH`** to get the exact external `L_r,ext` needed (typically **`~0.8 – 1.2 µH`**).
3. Wind **`3 turns`** of the same `15 mm²` `26 AWG` primary bundle onto a **`TDK PQ40/40` or `E55/28/21` (`N87`)** gapped ferrite core, adjusting the paper/Nomex spacer thickness while watching the LCR meter until **both Phase 1 and Phase 2 total loop inductances hit `2.08 µH ± 1.5%`** (`L_r1 == L_r2`)!
4. Finally, verify open-circuit `L_m1` and `L_m2` (`12.50 µH ± 2%`) by tweaking the `~1.52 mm` U93 shim thickness. This 10-minute LCR calibration completely erases DIY winding and busbar layout variations!

---

## 6. 2-Layer Signal & Gate-Drive PCB Subcircuits (`< 1 A` Total — Easy EasyEDA Pro)

All high power stays off the PCB. The 2-layer `1 oz` host signal board connects to `J1`/`J2` of the common STM32 Control Card and contains five clean low-current blocks:

1. **Isolated Gate-Drive Block (`4× TI UCC21520DW` + `8×` `2 W` Isolated `+12 V / -2 V` Bias Bricks):**
   - Each `UCC21520DW` drives one half-bridge leg (`Q1/Q2`, `Q3/Q4`, `Q5/Q6`, `Q7/Q8`) with built-in hardware shoot-through interlock and `10 kΩ` pull-downs on all 8 PWM inputs (`J2:1..12`).
   - Use **`R_g,on = 4.7 Ω`**, **`R_g,off = 2.2 Ω`** (parallel antiparallel Schottky `BAT54`/`1N5819`), `18 V` bidirectional TVS across `G–KS`, and a `10 kΩ` `G–KS` bleed resistor.
   - Route each gate output to its SOT-227 via a **short (`< 10 cm`) `20 AWG` tightly twisted pair** landed on the **`G` and `KS` (Kelvin Source)** M4 studs!
   - Bipolar **`+12 V / -2 V`** (or `+15 V / -3 V`, e.g., `Recom R2S-2415` / `Mornsun QA01C` or `15 V` brick + `2.7 V` Zener negative-bias offset) completely prevents parasitic Miller false turn-on on SOT-227 MOSFETs.
2. **Dual-Path `< 300 ns` Hardware Fault Latch:**
   - Wired-OR fast comparators (`TLV3501` / `LM393` with `200 ns` RC noise blanking):
     - Phase 1 & Phase 2 primary tank CTs (`> 130 A` peak bidirectional window comparator)
     - `96 V` bus `ACS37002` `FAULT` pin (`> 175 A`)
     - `V_HV+ > 218 V` or `|V_HV-| > 218 V` hardware OV comparators (off `AMC1311` outputs)
     - Gate-driver `UVLO` faults
   - Drives a hardware **`SN74LVC1G74` D-flip-flop latch** whose output simultaneously asserts **`HRTIM1_FLT1` (`PA12`, `J2:19`)** AND pulls all four **`UCC21520` `DISABLE` pins HIGH directly in hardware** (`< 200 ns` shutdown even if the STM32 is frozen or unplugged!). Resettable ONLY by MCU pulse on **`GPIO_PF4` (`J2:32`)**.
   - Series string of `T1`/`T2`/`L_r1`/`L_r2` `NC` thermal snap discs + opto-isolated `ESTOP_OK` (`AUX_COIL_24V`) drives **`HRTIM1_FLT3` (`PB10`, `J2:20`)** and the hardware latch.
3. **Isolated Voltage Conditioning (`3× TI AMC1311BDWV`):**
   - Flying twisted pairs from `LV_BUS+`/`LV_BUS-`, `HV+`/`HV_MID`, and `HV-`/`HV_MID` enter three high-impedance resistive dividers (`≥ 4` series `1206` MELF/thick-film resistors per HV leg for `> 1 kV` voltage rating + `≥ 8 mm` PCB creepage slot under each `AMC1311`) scaled to `0–2.0 V`, buffered by `TLV271` diff-to-SE op-amps into `J1:9..11` (`AIN_01..03`).
4. **Current Sensing (`2×` Thru-Hole HF CTs + `2×` Busbar Hall Sensors):**
   - **`I_res_Ph1`, `I_res_Ph2`:** Pass each phase's AC copper bar or Litz lead through a **`1:100` or `1:200` toroidal HF CT** (e.g., `Vacuumschmelze` / `Coilcraft CS1100L` / `Pulse PA1005.100`, `100 A+` window) → burden resistor on PCB → precision full-wave peak/RMS rectifier to `J1:14..15` (`AIN_06..07`) + fast window comparator to the hardware fault latch.
   - **`I_96`, `I_400`:** Panel/busbar-mount Hall sensors (e.g., `LEM HO 150-S` / `HO 50-S` or `ACS37002` busbar carrier) → `J1:12..13` (`AIN_04..05`).
5. **Auxiliary Power & Contactor Drivers:**
   - `AUX_LOGIC_24V` enters through a `2 A` fast fuse/eFuse → feeds `J1:1..4` (STM32 Control Card) + local `12 V` / `5 V` gate-bias & op-amp regulators.
   - `AUX_COIL_24V` (from E-Stop Safety Relay) feeds opto-isolated `ESTOP_OK` and the high-side of `Q_PRE`, `Q_ISOL`, and the `±200 V` `NC` discharge relay coils, switched on the low side by `NVD5117PL` DPAK MOSFETs driven by `CONT_CMD1..3` (`J1:32..34`) with `MUR120` flyback diodes and `CONT_FB1..3` (`J1:35..37`) mirror inputs.

---

## 7. Preliminary BOM Summary (Full DigiKey-Sourced BOM: [`Boost_Converter_BOM.md`](./Boost_Converter_BOM.md))

| Subsystem | Qty | Verified MPN (DigiKey Preferred) | Key Spec | Notes |
| :--- | :---: | :--- | :--- | :--- |
| **Primary MOSFETs (`Q1–Q8`)** | 8 | `IXYS IXFN300N20X3` (`IXFN300N20X3-ND`) / alt `IXFN230N20T` | `200 V`, `300 A`, `~3.5 mΩ`, SOT-227 w/ Kelvin Source | Bolted directly to common grounded heatsink |
| **Secondary SiC Diodes (`D1–D8`)** | 4 | `SemiQ GHXS100B065S-D3` | `650 V`, dual `100 A` SiC Schottky, SOT-227 | `2×` dual modules/phase = center-tapped full bridge |
| **Isolated Gate Drivers** | 4 | `TI UCC21520DWR` (or `UCC21520ADWR`) | Dual `5.7 kVrms`, `4 A / 6 A`, HW shoot-through interlock | Plus `8×` `Murata NXJ2S2415MC-R7` `2 W` stocked isolated bricks |
| **Main Transformers (`T1`, `T2`)** | 4 halves | `TDK B67345B0001X087` (`U93/76/30` `N87`, U-half!) | `A_e = 840 mm²`, `A_w = 1,520 mm²`, `6:13:13`, `1.52 mm` shim | Hand-wound with `26 AWG` twisted bundles (`2` closed U-U pairs) |
| **External `L_r` Cores (`L_r1`, `L_r2`)** | 4 halves | `TDK B66335G0000X187` (`E55/28/21` `N87`, `A_e = 353 mm²`) | `3 T` of `26 AWG` bundle (`159 mT` @ `140 A` pk), gapped to `2.08 µH` | LCR-matched `±1.5%` between Phase 1 & Phase 2 |
| **Resonant Screw-Lug Film Caps (`C_r`)** | 8 | `Vishay MKP386M515200YT4` (`BC2794-ND`) | `1.5 µF ±5%`, `2000 VDC / 700 VAC`, screw-mount MKP | `4×` per phase = `6.0 µF` bolted directly to copper bars |
| **Local `96 V` Input Caps + Snubbers** | 2 + 4 | `Nichicon LNT2C103MSE` + `Vishay MKP386M522100JT3` | `10,000 µF / 160 V` screw-term + `2.2 µF / 1000 V` screw MKP | Keeps primary DC-link loop `< 20 nH` |
| **Copper Busbars & Insulation** | Lot | `1/8" × 1"` (`C110`) Copper Bar + `0.5 mm` Nomex/G10 sheet | `80 mm²` cross-section (`> 200 A` rated), M4/M6 hardware | Zero high power on PCB |
| **Input/Output Fuses & Contactors** | Lot | `JJN-200` + `TE EV200AAANA` + `LEV100A5ANG` + `2× FWP-50A14FA` | Hermetically sealed DC contactors; `50 A / 700 V` aR fuses | **Never fuse `HV_MID`** |
| **Isolated Sensors** | 3 + 2 + 2 | `3× AMC1311BDWVR`, `2× LEM HO 150-S / HO 50-S`, `2× TDK B64290L0048X087` | `5 kVrms` voltage + `5 V` stocked busbar Hall + `35.5 mm` `1:200` CT | `200 T` pass-through CT handles `> 200 A` effortlessly |
| **Control Card Sockets** | 2 | `Sullins SFH11-PBPC-D20-ST-BK` (`S9200-ND`) | Mates with common `STM32G474QET6` control card `J1`/`J2` | Verified 100% compatible with zero card changes |

---

## 8. Digital Control (`STM32G474QET6` `HRTIM1`) & Bench Bring-Up

- **`HRTIM1` Timing (`184 ps` res):** Master Timer sets `PER = T_sw` (`f_sw` clamped strictly inside **`[28 kHz, 120 kHz]`**). `Timer A` (`0°`), `Timer B` (`180° − φ₁`), `Timer C` (`90°`), `Timer D` (`270° − φ₂`), `300 ns` dead-time (generous for SOT-227), `UCPD1_DBDIS = 1` at boot.
- **Outer Loop (`3–5 Hz` PI):** Damped by the centralized `60 mF/rail` split bank (`1.1 V pp` `120 Hz` ripple), completely immune to `120 Hz` oscillation.
- **Active Balance, Shedding & Soft-Start:** `PSM` soft-start (`110 kHz`, `φ: 160° → 0°`); hybrid `PFM + PSM` trim (`0°–15°`) equalizes Phase 1 & Phase 2 currents while keeping exact `90°` interleaving; Phase 2 sheds `< 3.5 kW`; `HRTIM1` hardware Burst Mode below `~400 W`.
- **Bring-Up Sequence:** (1) Simulate in [`Simulations/Boost_FBLLC_5kW_Phase.cir`](./Simulations/Boost_FBLLC_5kW_Phase.cir); (2) Bench-test `HRTIM1` `90°` interleaving + `< 300 ns` fault latch with `0 V` bus; (3) LCR-match `L_r1 == L_r2` (`2.08 µH ± 1.5%`) and `L_m1 == L_m2` (`12.5 µH ± 2%`); (4) `24 VDC` low-voltage ZVS & `±V_out` balance check; (5) Full-voltage `5 kW` → `10 kW` → `15 kW` thermal soak & `FLT1`/E-Stop trip verification.
