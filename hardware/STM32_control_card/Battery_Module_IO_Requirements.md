# Battery Charger/Discharger Module Control-Card I/O Requirements

Status: Draft functional inventory (Rev 0.2 — updated for the **2.5 kW Isolated Bidirectional Dual Active Bridge (DAB)** topology per [`../Battery_Isolation_Topology_Study.md`](../Battery_Isolation_Topology_Study.md)). Verified 100% compatible with the draft common control card ([`STM32_Control_Card_Pinout.md`](./STM32_Control_Card_Pinout.md)).

## Purpose and Scope

Identify the interface between the common `STM32G474QET6` control card and the `2.5 kW` isolated bidirectional Battery Charger/Discharger power board described in [`../Battery_and_BMS_Hardware.md`](../Battery_and_BMS_Hardware.md) and [`./STM32_Control_Card_Architecture.md`](./STM32_Control_Card_Architecture.md).

The module galvanically isolates and bridges a nominal `48 V` battery pack (`40.0–58.4 V`, managed by a COTS DALY Smart BMS) to the negative-grounded `96 VDC` Energy Bus using a fixed-frequency (`75 kHz`) single-phase Dual Active Bridge (`1:2` turns ratio) with `SPS`/`EPS`/`DPS` phase-shift modulation.

## Functional Signal Inventory

Directions are relative to the control card. All power-stage voltage, current, and BMS links are galvanically isolated on the host power board before reaching `J1`/`J2`.

| Function | Direction | Quantity | Control Card Mapping | Purpose & Notes |
| :--- | :--- | :---: | :--- | :--- |
| **DAB Dual Full-Bridge PWM** | Output | **8** (`4` comp. pairs) | `J2:1..2` (`CHA1/2`), `J2:5..6` (`CHB1/2`), `J2:7..8` (`CHC1/2`), `J2:11..12` (`CHD1/2`) | `48 V` pack bridge (`Timer A/B`) + `96 V` bus bridge (`Timer C/D`). Fixed `75 kHz` with seamless bidirectional phase shift `±φ` and `EPS`/`DPS` light-load control. |
| **Shared Gate-Driver Enable** | Output | **1** | `J2:25` (`PD15` / `GATE_EN`) | Enables/disables all 4 dual isolated gate drivers (`UCC21520`); defaults OFF (`10 kΩ` pull-down) on reset, power loss, or card removal. |
| **AC Transfer Inductor Current (`I_tank`)** | Input | **1** | `J1:9` (`AIN_01`, `PA0`) | Isolated high-bandwidth CT/Hall sense on `L_k` for cycle-by-cycle peak current limiting and transformer flux balance. |
| **Battery & `96 V` Bus DC Currents** | Input | **2** | `J1:10..11` (`AIN_02..03`, `PA1..PA2`) | Isolated `Allegro ACS37002` Hall sensors on both the `48 V` pack port and `96 VDC` bus port. |
| **Battery Pack Voltage (`V_bat`)** | Input | **1** | `J1:12` (`AIN_04`, `PA3`) | **Isolated (`TI AMC1311`)** pack voltage measurement (galvanically isolated from both `GND_CTRL` and `LV_BUS-`). |
| **`96 V` Bus Voltage (`V_96`)** | Input | **1** | `J1:13` (`AIN_05`, `PA4`) | **Isolated (`TI AMC1311`)** `96 VDC` bus voltage measurement for pre-charge `ΔV ≤ 1.5 V` check, droop control, and OV/UV protection. |
| **Heatsink & Magnetics Temperature** | Input | **1–2** | `J1:14..15` (`AIN_06..07`, `PA5..PA6`) | Analog heatsink (and optional transformer) temperature sensors. |
| **Hardware Fault Trips** | Input | **2** | `J2:19` (`FLT1`), `J2:20` (`FLT3`) | `FLT1`: Wired-OR of `I_tank`/`I_bat`/`I_96` OC comparators, OV comparators, and gate-driver faults (shuts down all 8 PWM outputs in `< 1 µs`). `FLT3`: Transformer/inductor thermal snap disc + E-Stop. |
| **Pre-Charge & Main Contactor Commands** | Output | **2** | `J1:32..33` (`PD2`, `PD7`) | Drive host-side `24 VDC` low-side drivers for `Q_PRE` and `Q_ISOL` on the `96 VDC` bus interface. |
| **Contactor Mirror Feedbacks** | Input | **2** | `J1:35..36` (`PE3`, `PE4`) | Verify `Q_PRE` and `Q_ISOL` physical contact states. |
| **Cooling Fan PWM & Tachometer** | Out / In | **2** | `J1:30..31` (`PD12`, `PD13`) | `TIM4` PWM speed control and tachometer feedback. |
| **DALY BMS Communication Link** | In / Out | **2–3** | `J1:27..29` (`PD5`, `PD6`, `PD4`) | `USART2` (`TX`/`RX` + optional RS485 `DE`) through a host-side galvanic isolator to the local COTS DALY Smart BMS. |

**Resource Summary:** **8 `HRTIM1` PWM outputs**, **6–7 analog inputs**, **2 hardware trips**, **1 shared `GATE_EN`**, **2 contactor outputs + 2 feedbacks**, **fan PWM + tach**, and **2–3 isolated BMS UART/RS485 signals**.

## Hardware Boundaries

- The `48 V` battery pack, the `96 VDC` bus, the `GND_CTRL` logic domain, and the system CAN bus are **four separate galvanic domains**. Never connect pack `B-`/`P-` or `LV_BUS-` to `GND_CTRL` or to each other.
- Keep the DALY BMS isolated from `GND_CTRL` and strictly separate from the System Coordinator CAN bus; the `STM32` acts as the local telemetry gateway.
- Provide `10 kΩ` pull-downs on all 8 PWM lines (`J2:1..12`), `GATE_EN`, and contactor lines for hardware safe-off on reset or card removal, and set `PWR_CR3.UCPD1_DBDIS = 1` in firmware before enabling `PA9`/`PA10`.

## Open Hardware Allocation Questions

- Confirm whether a series DC-blocking film capacitor (`10–20 µF`) is paired with `I_tank` sensing or whether active cycle-by-cycle flux balancing alone is used.
- Confirm whether `I_bat` and `I_96` `ACS37002` fault outputs wire-OR into `HRTIM1_FLT1` alongside the fast `I_tank` comparator.
