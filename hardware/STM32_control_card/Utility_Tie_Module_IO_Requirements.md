# Utility-Tie Module Control-Card I/O Requirements

Status: Draft functional inventory (Rev 0.2 — updated for the **10.8 kW Two-Stage Galvanically Isolated Utility-Tie** topology: interleaved active PFC + isolated `400 VDC → 96 VDC` DC-DC stage). Verified 100% compatible with the draft common control card ([`STM32_Control_Card_Pinout.md`](./STM32_Control_Card_Pinout.md)).

## Purpose and Scope

Identify the interface between the common `STM32G474QET6` control card and the `10.8 kW` isolated Utility-Tie power board described in [`../Utility_Tie_Hardware.md`](../Utility_Tie_Hardware.md) and [`./STM32_Control_Card_Architecture.md`](./STM32_Control_Card_Architecture.md).

Because Utility Neutral (`PE`) and `LV_BUS-` (`PE` via DC GFDI) are both Earth-referenced, the module uses two stages:
1. **Stage 1 (PFC):** Two-phase `180°` interleaved active PFC (`120/240 VAC` → internal `400 VDC` link).
2. **Stage 2 (Isolated DC-DC):** Galvanically isolated Full-Bridge LLC or PSFB (`400 VDC` → `96 VDC` Energy Bus, `≥ 4 kVrms` isolation).

## Functional Signal Inventory

Directions are relative to the control card. All AC, internal `400 VDC`, and `96 VDC` measurements are galvanically isolated on the host board.

| Function | Direction | Quantity | Control Card Mapping | Purpose & Notes |
| :--- | :--- | :---: | :--- | :--- |
| **Stage 1 Interleaved PFC PWM** | Output | **2** | `J2:1` (`CHA1`), `J2:5` (`CHB1`) | Two `180°` out-of-phase HRTIM PWM commands for the interleaved active PFC stage. |
| **Stage 2 Isolated Bridge PWM** | Output | **4** (`2` comp. pairs) | `J2:7..8` (`CHC1/2`), `J2:11..12` (`CHD1/2`) | Full-bridge primary PWM for the isolated `400 V → 96 V` LLC/PSFB stage (`CHE1/2`, `CHF1/2` remain spare if 2-phase isolated stage or SR is later added). |
| **Gate-Driver Enable** | Output | **1** shared | `J2:25` (`PD15` / `GATE_EN`) | Shared hardware enable/disable for PFC and isolated-stage gate drivers (`10 kΩ` pull-down default OFF). |
| **AC Input Voltage** | Input | **1** | `J1:9` (`AIN_01`) | Isolated differential measurement (`TI AMC1311`) of instantaneous AC line voltage for PLL, zero-crossing, and grid-window/anti-islanding detection. |
| **AC Input & PFC Phase Currents** | Input | **3** | `J1:10..12` (`AIN_02..04`) | Isolated `ACS37002` sensors for total AC current and both interleaved PFC phase currents. |
| **Internal `400 VDC` Link Voltage** | Input | **1** | `J1:13` (`AIN_05`) | Isolated (`TI AMC1311`) intermediate PFC bus voltage for PFC regulation and OV protection. |
| **`96 V` Bus Voltage & Output Current** | Input | **2** | `J1:14..15` (`AIN_06..07`) | Isolated `AMC1311` (`V_96`) and `ACS37002` (`I_96`) for pre-charge `ΔV ≤ 1.5 V` and regulated DC import control. |
| **Stage 2 Primary Tank Current** | Input | **1** | `J1:16` (`AIN_08`) | Isolated CT/Hall sense for Stage 2 overcurrent protection and diagnostics. |
| **Heatsink / Magnetics Temp** | Input | **1–2** | `J1:17..18` (`AIN_09..10`) | Analog heatsink/transformer temperature sensing. |
| **Hardware Trips** | Input | **2** | `J2:19` (`FLT1`), `J2:20` (`FLT3`) | `FLT1`: PFC/Stage-2 OC, `400 V` OV, and gate-driver faults (disables all 6 PWM outputs in `< 1 µs`). `FLT3`: Magnetics thermal snap discs / E-Stop. |
| **Contactor Commands (AC, `Q_PRE`, `Q_ISOL`)** | Output | **3** | `J1:32..34` (`PD2`, `PD7`, `PD14`) | AC utility input contactor, `96 V` pre-charge contactor, and `96 V` main isolation contactor. |
| **Contactor Mirror Feedbacks** | Input | **3** | `J1:35..37` (`PE3..PE5`) | Auxiliary mirror contacts verifying all three contactors. |
| **Cooling Fan PWM & Tachometer** | Out / In | **2** | `J1:30..31` (`PD12`, `PD13`) | `TIM4` PWM speed control and tachometer feedback. |

**Resource Summary:** **6 `HRTIM1` PWM outputs**, **9–10 analog inputs**, **2 hardware trips**, **1 shared `GATE_EN`**, **3 contactor outputs + 3 feedbacks**, and **fan PWM + tach** — all comfortably supported by `J1` and `J2`.

## Hardware Boundaries

- Maintain reinforced galvanic isolation (`≥ 4 kVrms`) between the Utility AC / internal `400 VDC` primary domain, the negative-grounded `96 VDC` output domain, and `GND_CTRL`.
- Route all overcurrent, `400 V` OV, and driver faults to `HRTIM1_FLT1` with host-side `10 kΩ` default-off pull-downs on every PWM and contactor output.

## Open Hardware Allocation Questions

- Confirm whether Stage 2 uses a single `10.8 kW` full-bridge LLC/PSFB (`4` PWM) or a 2-phase `2 × 5.4 kW` interleaved LLC (`8` PWM — still fits `J2` if PFC uses `TIM1`/`TIM8` or `CHE1`/`CHF1`).
- Confirm whether AC zero-crossing uses a dedicated hardware comparator input alongside isolated ADC sampling.
