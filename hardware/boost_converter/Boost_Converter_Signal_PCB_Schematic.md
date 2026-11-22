# 10 kW 2-Phase FB-LLC Boost Module — 2-Layer Signal & Gate-Drive PCB Schematic Spec

Status: Rev 0.1 EasyEDA-Pro-Ready Subcircuit Spec (2026-11-22) — Companion to [`Boost_Converter_Architecture.md`](./Boost_Converter_Architecture.md) and [`Boost_Converter_Mechanical_and_Power.md`](./Boost_Converter_Mechanical_and_Power.md).

**Core Principle:** This host PCB is a simple **`1 oz`, 2-layer, hand-solderable (`0805` / SOIC) board carrying `< 1 A` of logic, gate-drive, and isolated sensing signals**. Zero main power current ever touches the PCB.

---

## 1. Board Architecture & Isolation Keep-Outs

Partition the 2-layer PCB (`~180 mm × 100 mm`) into **four clean zones** with explicit copper keep-out moats (both Top & Bottom layers clear of copper, plus `2 mm` routed air slots under high-voltage isolators where practical):

1. **Control Domain (`GND_CTRL`, `24 V / 5 V / 3.3 V`):** `J1`/`J2` `2×20` female sockets (`SFH11-PBPC-D20-ST-BK`) for the common `STM32G474QET6` control card, `AUX_LOGIC_24V` `2 A` eFuse/fuse, local `5 V` & `12 V` regulators, `TLV271` buffers, `SN74LVC1G74` hardware fault latch, and `AUX_COIL_24V` contactor coil drivers.
2. **Primary Gate-Drive Islands (`8×` Isolated Islands):** `4× TI UCC21520DW` outputs + `8×` `2 W` isolated `24 V → 15 V` DC-DC bricks feeding `8×` 2-pin `5.08 mm` screw terminals (`G`, `KS`) along the edge closest to `Q1–Q8` (short `< 10 cm` twisted pairs to SOT-227 `G`/`KS` studs). Keep `≥ 4 mm` copper spacing between adjacent floating high-side gate islands!
3. **HV Voltage Sense Islands (`3×` Isolated Front-Ends):** `3× TI AMC1311BDWV` (`5 kVrms`, `8 mm` creepage) receiving flying twisted sense leads from `96 V` (`LV+`/`LV-`), `+200 V` (`HV+`/`HV_MID`), and `-200 V` (`HV_MID`/`HV-`) via `5.08 mm` `2-pin` HV terminal blocks. Maintain `≥ 8 mm` clearance under every `AMC1311` and `≥ 4 mm` between HV divider nodes.
4. **Current Sense & Fault Inputs:** `2×` `1:200` thru-hole HF CT headers (`I_res1`, `I_res2`), `2×` `LEM HO` busbar Hall sensor `4-pin` headers (`I_96`, `I_400`), `2×` NTC headers (`T_hs_pri`, `T_mag_sec`), thermal snap-disc loop header, and `FAN_PWM`/`FAN_TACH` header.

---

## 2. Subcircuit 1 — Isolated SOT-227 Gate Drive (`+12.3 V / -2.7 V` Bipolar Bias)

Repeat **4×** (`U_GD1..4` = `TI UCC21520DW`, SOIC-16 wide-body, `5.7 kVrms`, `4 A` source / `6 A` sink, hardware shoot-through interlock):

### 2.1 Primary Control Side (Logic Domain `GND_CTRL`)
- **`VCCI`:** `+5.0 V_HOST` (from local `24 V → 5 V` `1 A` buck on host board) decoupled with `100 nF` + `1 µF` `0805`.
- **PWM Inputs (`INA`, `INB`):** Connected to `J2:1..2` (`CHA1/2`), `J2:5..6` (`CHB1/2`), `J2:7..8` (`CHC1/2`), `J2:11..12` (`CHD1/2`) via `47 Ω` series damping resistors + **`10 kΩ` pull-downs to `GND_CTRL`** right at each `UCC21520` pin (guarantees hardware OFF during MCU reset or card removal).
- **`DT` Pin (Hardware Dead-Time Interlock):** Connect a **`30 kΩ` (`1%`)** resistor from `DT` to `GND_CTRL` → enforces **`~300 ns` hardware shoot-through dead-time** independently of STM32 `HRTIM1` dead-time!
- **`DIS` Pin (Dual-Path Hardware Kill):** Driven by an **OR gate (`SN74LVC1G32` or diode-OR)** combining:
  1. Inverted `GATE_EN` (`PD15`, `J2:25`, pulled LOW by `10 kΩ` so `DIS = HIGH` by default when MCU is off/reset!), AND
  2. Active-HIGH **`HW_FAULT_LATCH_Q`** from the `< 300 ns` hardware fault latch (Subcircuit 3).
  - *Result:* Even if the STM32 is frozen, a hardware overcurrent/OV trip pulls `DIS = HIGH` directly in `< 200 ns`!

### 2.2 Isolated Output Side (Repeat `8×` — One Floating Island Per SOT-227 MOSFET)
Each `UCC21520` channel (`OUTA` / `OUTB`) uses its own **`24 V → 15.0 V`, `2 W` isolated DC-DC brick** (e.g., `Mornsun B2415S-2WR3` / `Recom R2S-2415` / `Murata NXE2S2415MC`, `≥ 3 kVDC` isolation, low coupling capacitance `< 20 pF`) split into **`+12.3 V / -2.7 V`** relative to the SOT-227 **Kelvin Source (`KS`)**:

```text
  Isolated 15.0V Brick (+V_ISO) ---+---> [4.7uF + 100nF 25V] ---> UCC21520 VDDA (or VDDB)
                                   |
                                [1 kOhm 0.5W]
                                   |
                                   +---> KELVIN SOURCE (KS) ---> Twisted Pair to SOT-227 KS Stud
                                   |
                        [2.7V 1W Zener, e.g. 1N4726A]   ||   [4.7uF 16V X7R]
                        (Cathode to KS, Anode to -V_ISO)
                                   |
  Isolated 15.0V Brick (-V_ISO) ---+---> UCC21520 VSSA (or VSSB)  (= -2.7 V relative to KS!)

  UCC21520 OUTA --->+-- [R_gon = 4.7 Ohm, 1206 0.5W] ---------------------+---> GATE (G) Twisted Pair
                    +-- [D_off = B340A Schottky (Anode->G)]-[R_goff=2.2R]-+     to SOT-227 G Stud
                                                                          |
                                      [SMBJ18CA 18V Bi-TVS] || [10 kOhm] -+---> KS
```

- **Why `+12.3 V / -2.7 V`?** Driving the SOT-227 gate to `-2.7 V` during turn-off provides massive `dV/dt` Miller immunity (`V_GS(th) ≈ 3.5–4.5 V` → `> 6 V` noise margin!) using a single standard `15 V` brick per switch!

---

## 3. Subcircuit 2 — `1:200` Tank CT Conditioning & `< 300 ns` Dual-Path Hardware Fault Latch

### 3.1 Resonant Tank Current Sense (`I_res_Ph1`, `I_res_Ph2` — Repeat `2×`)
- **Sensor:** Large-aperture pass-through ferrite toroid **`TDK B64290L0048X087`** (`35.5 mm` OD, `N87`, DigiKey `3910123`) hand-wound with **`200` turns of `32 AWG`** wire (`1:200` ratio; slips directly over the `15 mm²` `26 AWG` primary bundle and handles `> 200 A` peak effortlessly—avoid tiny `20–35 A` SMD/TH CTs like `PA1005` or `CS1200L`!).
- **Burden Resistor (`R_burd`):** **`3.3 Ω`** built from **`3× 10.0 Ω` (`1%`, `1206`, `0.25 W` non-inductive)** in parallel (`0.75 W` rating; dissipates only `0.34 W` at `64.3 A RMS` nominal!).
  - Scaling: `64.3 A RMS` → `1.06 V RMS` (`1.50 V` pk); **`130 A` peak hardware OC trip** → **`±2.145 V` peak**!
- **Branch A — Precision Full-Wave Average/RMS Rectifier (To STM32 ADC `J1:14..15`):**
  - Classic 2-op-amp precision full-wave rectifier (`TI OPA2376` / `TLV2372` on `±5 V` or single-supply `5 V` with `1.65 V` bias) + 2nd-order RC low-pass (`10 kΩ` + `15 nF`, `f_c ≈ 1 kHz`), scaled so `106 A RMS` (`150 A` pk) = `3.00 V` into `AIN_06` (`PA5`, `J1:14`) and `AIN_07` (`PA6`, `J1:15`).
- **Branch B — Fast Bidirectional Window Overcurrent Comparator (To Hardware Fault Latch):**
  - AC burden voltage passes through a **`100 Ω` + `1 nF` (`100 ns` blanking)** filter into a fast dual comparator (`TI TLV3502` or `LM319` / `MAX962`, propagation delay `< 10–30 ns`) comparing against **`+2.15 V`** and **`-2.15 V`** thresholds (`130 A` pk trip on either half-cycle!).

### 3.2 Unified Hardware SR Fault Latch (`< 300 ns` Total Trip Delay)
Combine ALL critical Layer-0 open-drain / active-low fault triggers into one wired-OR bus (`10 kΩ` pull-up to `+3.3 V_HOST`):
1. `Ph1_TANK_OC_N` (from Phase 1 `±2.15 V` window comparator, `> 130 A` pk)
2. `Ph2_TANK_OC_N` (from Phase 2 `±2.15 V` window comparator, `> 130 A` pk)
3. `HV_POS_OV_N` (`V_HV+ > 218.0 V` comparator off `AMC1311` output: `> 2.624 V` threshold)
4. `HV_NEG_OV_N` (`|V_HV-| > 218.0 V` comparator off `AMC1311` output: `> 2.624 V` threshold)
5. `BUS96_OC_N` (`LEM HO 150-S` `OCD` pin or comparator `> 175 A`)

- **Latch IC (`TI SN74LVC1G74DCUR` D-Flip-Flop, powered from `+3.3 V_HOST`):**
  - Wire wired-OR `FAULT_TRIG_N` to asynchronous **`PRE_N` (Preset, active-LOW)** → any `< 50 ns` fault pulse immediately sets **`Q = HIGH`** asynchronously!
  - Wire **`Q` (`HW_FAULT_LATCH_Q`)** to:
    1. All four `UCC21520DW` **`DIS`** pins via OR gate (**direct hardware gate kill in `< 200 ns`**!), AND
    2. **`HRTIM1_FLT1` (`PA12`, `J2:19`)** via `100 Ω` resistor (tells STM32 `HRTIM1` to freeze PWM immediately and trigger fault ISR!).
  - **Reset Path:** Tie `D = LOW`, `CLR_N = HIGH`, and drive `CLK` from **`GPIO_PF4` (`J2:32`)** (with `10 kΩ` pull-down to `GND_CTRL`)! Because `PRE_N` overrides `CLK`, firmware can ONLY clear the latch after the underlying fault disappears AND software intentionally pulses `GPIO_PF4` during an authorized `LOCKOUT` recovery.
  - **Power-Up Default:** Add a `100 nF` cap + `100 kΩ` RC on `PRE_N` (or keep `GATE_EN = LOW` at boot) so power-up initializes cleanly in a safe disabled state until firmware verifies all sensors and issues a deliberate clear pulse.

---

## 4. Subcircuit 3 — `TI AMC1311BDWV` Isolated Voltage Dividers (`V_96`, `V_HV+`, `V_HV-`)

All three voltage channels use the identical **`TI AMC1311BDWV`** (`5 kVrms` reinforced, `0–2.00 V` linear input, `1.0×` gain, `1.44 V` common-mode differential output) powered on the HV side by a tiny `5 V → 5 V` isolated supply (e.g., `TI SN6501` or `Mornsun B0505S-1WR3` per HV domain — note `V_HV+` and `V_HV-` share `HV_MID` so **ONE isolated `5 V` supply referenced to `HV_MID` powers BOTH `V_HV+` and `V_HV-` `AMC1311` front-ends!**).

| Channel | Full-Scale (`2.00 V` in) | Top Divider String (`1%`, `1206`) | Bottom `R_bot` (`0.1%`, `0805`) + Filter | Nominal Voltage @ ADC (`G = 1.50`) | Control Card Pin |
| :--- | :---: | :--- | :--- | :---: | :--- |
| **`V_BUS96`** (`LV+` to `LV-`) | `119.6 V` | `2× 29.4 kΩ` (`58.8 kΩ`, `400 V` rating) | `1.00 kΩ` + `10 nF` NP0 (`f_c = 16 kHz`) | `96.0 V` → `2.408 V` (`0.24 W` tot) | `J1:9` (`AIN_01`, `PA0`) |
| **`V_HV+`** (`HV+` to `HV_MID`) | `249.2 V` | `4× 30.9 kΩ` (`123.6 kΩ`, `800 V` rating) | `1.00 kΩ` + `10 nF` NP0 (`f_c = 16 kHz`) | `+200.0 V` → `2.408 V` (`218 V` OV = `2.624 V`) | `J1:10` (`AIN_02`, `PA1`) |
| **`V_HV-`** (`HV_MID` to `HV-`)* | `249.2 V` | `4× 30.9 kΩ` (`123.6 kΩ`, `800 V` rating) | `1.00 kΩ` + `10 nF` NP0 (`f_c = 16 kHz`) | `-200.0 V` → `+2.408 V` (`-218 V` OV = `2.624 V`) | `J1:11` (`AIN_03`, `PA2`) |

*\*Polarity Trick for `V_HV-`:* Connect the `4× 30.9 kΩ` top string to `HV_MID` (`0 V`) and the `1.00 kΩ` bottom resistor to `HV-` (`-200 V`), powering that `AMC1311`'s isolated `5 V` supply referenced to `HV-` (so `VINP` sits `+1.605 V` above `VINN = HV-`!). That delivers a clean positive `0–3.00 V` ADC voltage with **zero negative-voltage op-amp inverter needed**! *(Note: this means `V_HV-` uses its own tiny `1 W` `5 V → 5 V` isolated brick referenced to `HV-`, while `V_HV+` uses one referenced to `HV_MID`, and `V_96` uses one referenced to `LV_BUS-` — 3× `B0505S-1WR3`, ~$2 each!)*

- **Differential-to-Single-Ended Buffer (Repeat `3×`, `TI TLV271` or `OPA2376` on `+3.3 V_HOST`):**
  - `R1 = R2 = 10.0 kΩ` (`0.1%`, `0805`) from `AMC1311 VOUTP`/`VOUTN`.
  - `R3 = R4 = 15.0 kΩ` (`0.1%`, `0805`) → exact **`G = 1.500`** (`0–2.00 V` → `0–3.00 V`).
  - `100 pF` NP0 across feedback `R3` (`f_c = 106 kHz`) + `100 Ω` + `10 nF` kickback RC right at `J1:9..11`.

---

## 5. Subcircuit 4 — `AUX_LOGIC_24V`, `AUX_COIL_24V` (`ESTOP_OK`), Thermal & Contactor Drivers

1. **`AUX_LOGIC_24V` Input (`J_AUX_LOGIC`, 2-pin `5.08 mm`):**
   - Enters through **`2 A` fast fuse / `TI TPS25982` eFuse** + `SMBJ28A` TVS (`V_br = 31 V`, clamps `< 45 V`) → feeds `J1:1..4` (`+24V_IN` to STM32 Control Card) AND host local `24 V → 5.0 V` `1 A` buck (for `UCC21520` `VCCI`, `LEM HO` sensors, `AMC1311` `5 V` bricks) + `5 V → 3.3 V` LDO (`+3.3 V_HOST` for comparators, fault latch, and `TLV271` buffers).
2. **`AUX_COIL_24V` & Opto-Isolated `ESTOP_OK` (`J_AUX_COIL`, 2-pin `5.08 mm`):**
   - Passes through a `10 kΩ / 0.25 W` series resistor into a **`Vishay SFH615A` / `PC817`** optocoupler LED (`~2.2 mA` when `AUX_COIL_24V = 24 V`).
   - Optocoupler transistor pulls **`ESTOP_OK`** cleanly; wired in series with the normally-closed (`NC`) `95 °C` magnetics thermal snap-disc loop (`T1`, `T2`, `L_r1`, `L_r2`) into **`HRTIM1_FLT3` (`PB10`, `J2:20`)**!
   - *Cold-Start Rule:* Per [`../../architecture/System_State_MAchine.md`](../../architecture/System_State_MAchine.md), firmware distinguishes `ESTOP_OK = LOW` in `STARTUP`/`READY` (*Waiting for Re-Arm*) from a drop during `BUS_PRECHARGE`/`RUNNING` (immediate `< 1 µs` `HRTIM1_FLT3` kill + `LOCKOUT`).
3. **Contactor & `NC` Discharge Relay Low-Side Drivers (`3×`):**
   - `CONT_CMD1` (`PD2`, `J1:32` → `Q_PRE`), `CONT_CMD2` (`PD7`, `J1:33` → `Q_ISOL`), `CONT_CMD3` (`PD14`, `J1:34` → Centralized `±200 V` `NC` Discharge Relays).
   - Each drives a **`NVD5117PL` / `IRLR2905`** DPAK N-MOSFET (`100 Ω` gate resistor + `10 kΩ` pull-down to `GND_CTRL`) switching the low side of the `24 VDC` coil whose high side connects strictly to **`AUX_COIL_24V`**, protected by a **`MUR120`** flyback diode across each coil.
   - Auxiliary mirror contacts (`Q_PRE_AUX`, `Q_ISOL_AUX`, `DISCH_AUX`) connect to `CONT_FB1..3` (`PE3..PE5`, `J1:35..37`) via `10 kΩ` pull-ups to `+3.3 V_HOST` + `1 kΩ` / `100 nF` RC debounce filters.
4. **Busbar Current (`LEM HO 150-S` / `HO 50-S`) & Heatsink/Magnetics NTCs:**
   - Standard stocked `5 V` `LEM HO 150-S` / `HO 50-S` sensors output `2.5 V ± 2.0 V` (`0.5–4.5 V`) — use a simple precision `10 kΩ / 15 kΩ` (`0.1%`) resistor divider (`× 0.60`, scaling to `1.50 V ± 1.20 V` = `0.30–2.70 V` safe inside `0–3.3 V`) + `10 nF` RC filter into `J1:12..13` (`AIN_04..05`). *(This avoids special-order lead times on the `3.3 V` `/SP33` derivatives; if `/SP33` is in stock, omit the divider and use `100 Ω + 10 nF` directly.)*
   - `10 kΩ` (`B = 3950`) NTC thermistors bolted to Primary Heatsink (`T_hs_pri`) and Secondary/Magnetics (`T_mag_sec`) form simple voltage dividers with `10 kΩ` (`0.1%`) pull-ups to `+3.3 V_HOST` + `100 nF` filter caps into `J1:16..17` (`AIN_08..09`).

---

## 6. Complete Host PCB ↔ STM32 Control Card (`J1` & `J2`) Netlist Summary

| Control Card Pin | Net Name | Host PCB Circuit Connection |
| :--- | :--- | :--- |
| `J1:1..4` | `+24V_IN` / `GND_CTRL` | Fused `AUX_LOGIC_24V` (`2 A` eFuse) & host logic ground |
| `J1:9..11` | `AIN_01..03` | `V_BUS96`, `V_HV+`, `V_HV-` (`3× AMC1311 + TLV271 G=1.50`) |
| `J1:12..13` | `AIN_04..05` | `I_IN_96V` (`LEM HO 150-S` + `0.60×` div), `I_OUT_400V` (`LEM HO 50-S` + `0.60×` div) |
| `J1:14..15` | `AIN_06..07` | `I_RES_PH1`, `I_RES_PH2` (`2× 1:200 CT` + precision rectifier/LPF) |
| `J1:16..17` | `AIN_08..09` | `T_HS_PRI`, `T_MAG_SEC` (`2× 10 kΩ` NTC dividers) |
| `J1:30..31` | `FAN_PWM` / `FAN_TACH` | Open-drain `5 V` PWM buffer & tach pull-up to `2× 120 mm` `24 V` fans |
| `J1:32..37` | `CONT_CMD1..3` / `FB1..3` | `Q_PRE`, `Q_ISOL`, `±200 V NC Discharge` low-side FETs & mirror feedbacks |
| `J2:1..2, 5..6` | `PWM_A1..B2` | Phase 1 `UCC21520DW` (`U_GD1`, `U_GD2`) w/ `10 kΩ` pull-downs |
| `J2:7..8, 11..12` | `PWM_C1..D2` | Phase 2 `UCC21520DW` (`U_GD3`, `U_GD4`) w/ `10 kΩ` pull-downs |
| `J2:19` | `FLT1_TRIP` | `HW_FAULT_LATCH_Q` (`SN74LVC1G74`, `< 300 ns` tank OC / HV OV / bus OC) |
| `J2:20` | `FLT3_AUX` | Series `T1/T2/Lr1/Lr2` `95 °C` NC snap discs + opto `ESTOP_OK` (`AUX_COIL_24V`) |
| `J2:25` | `GATE_EN` | Shared gate-driver enable (`10 kΩ` PD, OR'd with `HW_FAULT_LATCH_Q` into `DIS`) |
| `J2:32` | `GPIO_PF4` | Hardware fault latch `CLK` reset pulse (`10 kΩ` PD) |
