# Battery Node Hardware Baseline

## System Partitioning

Per the `Project Context` and `Battery Node` architecture, a single Battery Node presents one unified interface to the System Coordinator, while internally dividing into three physical hardware subsystems:

1. **Battery Pack** (Energy Storage Medium)
2. **Battery Management System (BMS)** (COTS DALY Unit for Cell Monitoring, Balancing & Pack Protection)
3. **Charger / Discharger Module** (2.5kW Bidirectional DC/DC Power Stage & Bus Interface)

Because the Charger/Discharger Module **galvanically isolates** the `48V` pack and its DALY BMS from the negative-grounded `96VDC Energy Bus`, different battery chemistries (LiFePO4, NMC, Lead-Acid, Sodium-Ion) can be swapped or mixed across parallel Battery Nodes without altering the external system interface, and an accidental `B-` to Earth (`PE`) fault on a battery rack cannot bypass the DALY BMS low-side protection MOSFETs. See [`Battery_Isolation_Topology_Study.md`](./Battery_Isolation_Topology_Study.md).

---

## Subsystem 1: Battery Pack

- **Primary Reference Pack:** `16S10P` `21700` LiFePO4 cell array (`48V` nominal, `40.0V` cutoff to `58.4V` full charge, `~40Ah–50Ah` capacity per series block).
- **Alternative Chemistry Support:** `48V` nominal deep-cycle Lead-Acid blocks (Flooded, AGM, Gel) or `14S`/`16S` Lithium-Ion / Sodium-Ion strings.
- **Common Auxiliary Power Source (Split `24 VDC` Lifeline):** Each battery pack feeds an internal, galvanically isolated `48 V → 24.0 VDC` auxiliary supply (`≥ 2.5 kVrms`, output droop + Ideal-Diode OR) feeding the un-interruptible **`AUX_LOGIC_24V`** (`Black Start` lifeline) and E-Stop-interlocked **`AUX_COIL_24V`** backbone. See [`../docs/System_Safety_and_Protection_Architecture.md`](../docs/System_Safety_and_Protection_Architecture.md).

---

## Subsystem 2: Battery Management System (BMS)

The Battery Node uses a commercial off-the-shelf (COTS) **DALY BMS** for cell-level monitoring, passive balancing, and primary pack protection. The BMS communicates strictly with the local Charger/Discharger controller (`STM32G474QE`) and never connects directly to the System Coordinator CANbus.

- **Hardware Standard:** COTS **DALY Smart BMS** (`16S` / `48V` configuration, sized to match pack continuous discharge ratings of `≥80A–100A`).
- **Internal Node Communication (BMS ➔ Converter):**
  - **Protocol:** Native **DALY BMS communication protocol** (galvanically isolated UART / RS485 or native DALY CAN link to the local `STM32G474QE` daughter card).
  - **Data Exchanged:** Pack state-of-charge (`SOC`), individual cell min/max voltages, pack temperature, charge/discharge MOSFET status, and BMS protection fault flags.
- **External System Communication (Converter ➔ Coordinator):**
  - **Physical Layer:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` peripheral via `TI ISO1042DWV` + card-mounted isolated DC/DC powered from `AUX_LOGIC_24V`).
  - **Application Protocol:** Uses the working Classical CAN 2.0B registry in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md) (`GlobalNodeID 0x01–0x05`, `TELEM_BATTERY_FAST` `0x200+`, `TELEM_BATTERY_SLOW` `0x210+`). The `STM32G474QE` acts as a strict protocol gateway, encapsulating local DALY BMS telemetry and converter power metrics onto the System Coordinator CANbus.
- **Protection Role:** Acts as the primary cell-level hardware disconnect (over-voltage, under-voltage, over-temperature, and short-circuit cutoff) independent of the Charger/Discharger control loop.

---

## Subsystem 3: Charger / Discharger Module (2.5 kW Isolated Bidirectional DAB)

This module houses the **STM32G474QET6** local node controller on a 4-layer daughter card paired with a 2-layer power stage that **galvanically isolates** and bridges the `48V` pack (`40.0V–58.4V`) to the negative-grounded `96VDC Energy Bus`. See [`Battery_Isolation_Topology_Study.md`](./Battery_Isolation_Topology_Study.md) for the topology trade-off analysis and first-pass sizing skeleton.

- **Power Rating:** **2.5 kW continuous** (`3.0 kW` short-term peak) bidirectional power transfer (`62.5 A` max battery-side current at `40.0 V` deep discharge; `26 A` nominal on the `96 VDC` bus).
- **Topology:** **Galvanically isolated single-phase Dual Active Bridge (DAB)** switching at a fixed **75 kHz**, using Single-Phase-Shift (`SPS`) at medium/heavy load and Extended/Dual Phase-Shift (`EPS`/`DPS`) at light load and extreme battery voltages to preserve ZVS and seamlessly reverse power flow (`CHARGING ↔ DISCHARGING`) by changing phase shift sign (`±φ`).
- **Power Switches (`Q1–Q8`):**
  - **48 V Pack-Side Full Bridge (`Q1–Q4`):** Four (4) `100 V` low-`R_DS(on)` MOSFETs (e.g., **Infineon `IPW036N10N5`**, `≤ 4 mΩ`, **TO-247**) mounted to a heatsink via ceramic insulators.
  - **96 V Bus-Side Full Bridge (`Q5–Q8`):** Four (4) `150 V` N-channel MOSFETs (e.g., **Infineon `IPW110N15N5`**, `9.7 mΩ`, **TO-247**).
- **Gate Drivers:** Four (4) **TI `UCC21520`** dual-channel isolated gate drivers (`5.7 kVrms`, `4 A` source / `6 A` sink) driven by `HRTIM1_CHA1/CHA2` (`PA8`/`PA9`) + `HRTIM1_CHB1/CHB2` (`PA10`/`PA11`) on the `48 V` bridge and `HRTIM1_CHC1/CHC2` (`PB12`/`PB13`) + `HRTIM1_CHD1/CHD2` (`PB14`/`PB15`) on the `96 V` bridge, with shared `GATE_EN` (`PD15`) safe-off interlock and isolated `12–15 V` gate bias rails.
- **Isolation Transformer & Transfer Inductor (`T1`, `L_k`):**
  - **HF Transformer (`T1`):** Ferrite core (`3C95`/`N87`), **`1 : 2` turns ratio** (centered at `48 V ↔ 96 V` unity gain), `≥ 2.5–4 kVrms` galvanic isolation between pack and `96 VDC` bus.
  - **AC Transfer Inductance (`L_k`):** **`~2.2–2.8 µH`** referred to the `48 V` primary side (transformer leakage + compact external gapped ferrite AC inductor; eliminates the large DC-biased Kool Mµ toroid) plus an optional series polypropylene DC-blocking capacitor (`10–20 µF`) to prevent transformer flux walking.
- **Snubbers & Bulk Capacitance:**
  - `0.1 µF / 250 V` polypropylene film snubbers (`KEMET R76`) close-coupled across both switching bridges.
  - Parallel banks of low-ESR electrolytic capacitors on both the `48 V` battery port (`100 V` rated) and `96 VDC` bus port (`160 V` rated).
- **Bus Interface & Contactors:**
  - Class-T / semiconductor DC fusing on both pack and `96 VDC` ports.
  - Standardized `24 VDC` coil Pre-Charge Bypass Contactor (`CONT_CMD1` / `PD2`, `J1:32`) and hermetically sealed Main Isolation Contactor (`CONT_CMD2` / `PD7`, `J1:33`) on the `96 VDC` bus side, powered via E-Stop-interlocked **`AUX_COIL_24V`** (plus `CONT_CMD3` / `PD14` gating the `96 V` `NC` bus discharge relay on Startup Battery 0; pin assignments per draft [`STM32_control_card/STM32_Control_Card_Pinout.md`](./STM32_control_card/STM32_Control_Card_Pinout.md)).
- **Telemetry Acquisition (All Galvanically Isolated from `GND_CTRL`):**
  - **AC Tank Current (`I_tank`):** High-bandwidth Hall/CT sensor on `L_k` for peak overcurrent hardware trip (`HRTIM1_FLT1`) and flux-balance monitoring.
  - **Pack & `96 V` Bus Currents:** `Allegro ACS37002` Hall-effect sensors on both DC ports.
  - **Battery Pack Voltage (`V_bat`) & `96 V` Bus Voltage (`V_96`):** Independent **TI `AMC1311`** isolated differential amplifiers (required because the battery pack and `96 V` bus are now galvanically isolated from each other and from `GND_CTRL`).
  - **Thermal Protection:** Heatsink NTC sensor + fail-safe transformer/inductor thermal snap disc (`HRTIM1_FLT3`).
