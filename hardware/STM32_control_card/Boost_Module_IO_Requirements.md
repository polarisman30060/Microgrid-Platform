# Boost Module Control-Card I/O Requirements

Status: Draft functional inventory (Rev 0.2 — updated for the **10 kW 2-Phase Interleaved Full-Bridge LLC** topology with earth-referenced `400 VDC` / `±200 VDC` split output). Verified 100% compatible with the draft common control card ([`STM32_Control_Card_Pinout.md`](./STM32_Control_Card_Pinout.md)) with zero card PCB changes required.

## Purpose and Scope

Identify the interface between the common `STM32G474QET6` control card and the `10 kW` (`2 × 5 kW`) 2-phase interleaved Full-Bridge LLC boost power board described in [`Boost_Converter_Hardware.md`](../boost_converter/Boost_Converter_Hardware.md), [`Boost_Converter_Architecture.md`](../boost_converter/Boost_Converter_Architecture.md), [`Boost_Node.md`](../../architecture/nodes/Boost_Node.md), and [`STM32_Control_Card_Architecture.md`](./STM32_Control_Card_Architecture.md).

The boost module galvanically isolates and converts the negative-grounded `96 VDC` Energy Bus to the earth-referenced `400 VDC` (`±200 VDC` split-rail) Inverter Bus. Its baseline uses two `90°`-interleaved `100%` SOT-227 primary full-bridges (`f_r = 45.0 kHz`, M4 copper busbars, `TDK U93/76/30` transformers), center-tapped full-bridge SiC Schottky diode rectifiers tied to `HV_MID` (`N/PE`), `PFM` regulation, `PSM` soft-start and active phase-current balance trim, light-load phase shedding, and `HRTIM1` hardware burst mode.

## Functional Signal Inventory

Directions are relative to the control card. The host power board houses all power switches, isolated gate drivers, magnetics, isolated sensors, hardware fault comparators, and `24 VDC` contactor drivers.

| Function | Direction | Quantity | Control Card Mapping | Purpose & Notes |
| :--- | :--- | :---: | :--- | :--- |
| **Primary Full-Bridge PWM (Ph1 & Ph2)** | Output | **8** (`4` comp. pairs) | `J2:1..2` (`CHA1/2`), `J2:5..6` (`CHB1/2`), `J2:7..8` (`CHC1/2`), `J2:11..12` (`CHD1/2`) | Ph1 uses `Timer A` (`0°`) + `Timer B` (`180°−φ₁`); Ph2 uses `Timer C` (`90°`) + `Timer D` (`270°−φ₂`). Supports PFM, PSM soft-start, per-phase PSM balancing, shedding, and hardware burst mode. (`CHE1/2`, `CHF1/2` remain spare.) |
| **Shared Gate-Driver Enable** | Output | **1** | `J2:25` (`PD15` / `GATE_EN`) | Hardware enable for all 4 dual isolated gate drivers; must default OFF (`10 kΩ` pull-down) during reset, power loss, or card removal. |
| **Fast Hardware Fault Trip** | Input | **1** | `J2:19` (`PA12` / `HRTIM1_FLT1`) | Asynchronous (`< 1 µs`) wired-OR hardware trip: Ph1/Ph2 resonant tank OC comparators, `96 V` bus OC (`ACS37002`), `V_HV+`/`V_HV-` fast OV comparators, and gate-driver UVLO/faults. Latches all 8 PWM outputs OFF in hardware. |
| **Aux / Thermal Hardware Trip** | Input | **1** | `J2:20` (`PB10` / `HRTIM1_FLT3`) | Fail-safe normally-closed series loop of transformer/resonant-inductor thermal snap discs (`T1`, `T2`, `L_r1`, `L_r2`) and external E-Stop interlock. |
| **`96 V` Input Bus Voltage** | Input | **1** | `J1:9` (`AIN_01`, `PA0`) | Isolated (`TI AMC1311`) `96 VDC` measurement for pre-charge `ΔV ≤ 1.5 V` check, UV/OV protection, and feedforward. |
| **`+200 V` (`HV+`) & `-200 V` (`HV-`) Split Rail Voltages** | Input | **2** | `J1:10..11` (`AIN_02..03`, `PA1..PA2`) | Two independent isolated (`TI AMC1311`) measurements relative to `HV_MID` (`PE`). Computes both total `V_400 = V_HV+ + |V_HV-|` and split-rail balance `V_HV+ − |V_HV-|`. |
| **`96 V` Input & `400 V` Output Bus Currents** | Input | **2** | `J1:12..13` (`AIN_04..05`, `PA3..PA4`) | Isolated `Allegro ACS37002` sensors for power telemetry, parallel module `20 ms` load balancing, and overload protection. |
| **Ph1 & Ph2 Resonant Tank Currents** | Input | **2** | `J1:14..15` (`AIN_06..07`, `PA5..PA6`) | High-bandwidth isolated CT/Hall RMS-or-peak conditioned sense for Phase 1 / Phase 2 active PSM current-balance trim and `TELEM_BOOST_SLOW` reporting. |
| **Temperatures (Primary Heatsink & Magnetics/Rectifier)** | Input | **2** | `J1:16..17` (`AIN_08..09`, `PA7`, `PC0`) | Analog NTC/temp conditioning for thermal derating, fan curve, and `TELEM_BOOST_FAST`/`SLOW`. |
| **Optional `HV_MID` Neutral Current** | Input | **0–1** | `J1:18` (`AIN_10`, `PC1`) | Provisional isolated Hall sensor on `HV_MID` busbar for asymmetric `120 VAC` load diagnostics. |
| **Contactor & Discharge Commands** | Output | **3** | `J1:32..34` (`PD2`, `PD7`, `PD14`) | `CONT_CMD1`: `Q_PRE` (`96 V` pre-charge); `CONT_CMD2`: `Q_ISOL` (`96 V` main isolation); `CONT_CMD3`: active `400 V` cap-bank discharge / optional HV output disconnect. |
| **Contactor Mirror Feedbacks** | Input | **3** | `J1:35..37` (`PE3..PE5`) | Auxiliary mirror contacts verifying `Q_PRE`, `Q_ISOL`, and discharge/HV disconnect states to catch welded contacts. |
| **Cooling Fan PWM & Tachometer** | Out / In | **2** | `J1:30..31` (`PD12`, `PD13`) | `TIM4_CH1` PWM speed command + `TIM4_CH2` tachometer feedback for heatsink and magnetics forced-air cooling. |

**Resource Summary:** **8 `HRTIM1` PWM outputs** (of 12 on `J2`), **9–10 analog inputs** (of 16 on `J1`), **2 `HRTIM1` fault inputs** (of 4 on `J2`), **1 `GATE_EN`**, **3 contactor commands + 3 feedbacks**, and **fan PWM/tach**. CAN, Waveshare LCD/keypad HAT, SWD, and card power are common card functions.

## Control and Protection Boundaries

- **Galvanic Separation:** Primary `96 V` (`LV_BUS-` to `PE`), secondary `±200 V` (`HV_MID` to `PE`), and control `GND_CTRL` are strictly isolated across `≥ 4 kVrms` reinforced barriers. Never connect `96 V` or `±200 V` rails directly to control-card pins.
- **Inherent Fault Interruption:** Because series resonant capacitors (`C_r1`, `C_r2`) and HF transformers block DC, asserting `HRTIM1_FLT1` (`< 1 µs`) stops all energy flow from `96 V` to `400 V` immediately—before `Q_ISOL` opens.
- **Hardware Default-Off & UCPD Caveat:** Host board must provide `10 kΩ` pull-downs on `J2:1..12` (`CHA1..CHD2`), `GATE_EN` (`PD15`), and `CONT_CMD1..3` so all switches and contactors stay OFF on reset, power loss, or card removal. Firmware must set `PWR_CR3.UCPD1_DBDIS = 1` during early init before enabling `PA9`/`PA10` (`CHA2`/`CHB1`).
- **Capacitive-Mode Prevention:** Firmware enforces a hard minimum switching frequency floor (`f_min = 28 kHz`, above the inductive-capacitive boundary) and monitors `FLT1` tank overcurrent so the LLC never operates in hard-switched capacitive mode.

## Open Decisions

- Confirm CT vs. high-bandwidth Hall (`ACS37030`) conditioning topology (peak-hold vs. RMS-to-DC vs. synchronized HRTIM ADC sampling) for `I_res_Ph1` / `I_res_Ph2`.
- Confirm whether `CONT_CMD3` drives an active `400 V` bleeder discharge FET or a 2-pole HV output contactor (passive SiC diodes already block reverse current from parallel modules).
- Validate end-to-end propagation delay from resonant tank OC comparator → `HRTIM1_FLT1` → `UCC21520` → MOSFET gate off on bench hardware (`< 500 ns` target).
