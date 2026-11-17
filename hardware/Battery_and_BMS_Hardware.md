# Battery Node Hardware Baseline

## System Partitioning

Per the `Project Context` and `Battery Node` architecture, a single Battery Node presents one unified interface to the System Coordinator, while internally dividing into three physical hardware subsystems:

1. **Battery Pack** (Energy Storage Medium)
2. **Battery Management System (BMS)** (COTS DALY Unit for Cell Monitoring, Balancing & Pack Protection)
3. **Charger / Discharger Module** (2.5kW Bidirectional DC/DC Power Stage & Bus Interface)

Because the Charger/Discharger Module isolates the pack from the `96VDC Energy Bus`, different battery chemistries (LiFePO4, NMC, Lead-Acid, Sodium-Ion) can be swapped or mixed across parallel Battery Nodes without altering the external system interface.

---

## Subsystem 1: Battery Pack

- **Primary Reference Pack:** `16S10P` `21700` LiFePO4 cell array (`48V` nominal, `40.0V` cutoff to `58.4V` full charge, `~40Ah–50Ah` capacity per series block).
- **Alternative Chemistry Support:** `48V` nominal deep-cycle Lead-Acid blocks (Flooded, AGM, Gel) or `14S`/`16S` Lithium-Ion / Sodium-Ion strings.
- **Common Auxiliary Power Source (`30V 50kHz`):** Each battery pack feeds an internal, independent high-frequency push-pull inverter stage that steps down cell voltage into the un-interruptible **30V 50kHz** common auxiliary power source (`Black Start` lifeline).

---

## Subsystem 2: Battery Management System (BMS)

The Battery Node uses a commercial off-the-shelf (COTS) **DALY BMS** for cell-level monitoring, passive balancing, and primary pack protection. The BMS communicates strictly with the local Charger/Discharger controller (`STM32G474QE`) and never connects directly to the System Coordinator CANbus.

- **Hardware Standard:** COTS **DALY Smart BMS** (`16S` / `48V` configuration, sized to match pack continuous discharge ratings of `≥80A–100A`).
- **Internal Node Communication (BMS ➔ Converter):**
  - **Protocol:** Native **DALY BMS communication protocol** (galvanically isolated UART / RS485 or native DALY CAN link to the local `STM32G474QE` daughter card).
  - **Data Exchanged:** Pack state-of-charge (`SOC`), individual cell min/max voltages, pack temperature, charge/discharge MOSFET status, and BMS protection fault flags.
- **External System Communication (Converter ➔ Coordinator):**
  - **Physical Layer:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` peripheral via a CAN isolator whose bus-side transceiver rail is powered by the **30V 50kHz** common auxiliary power source).
  - **Application Protocol:** **To Be Defined (TBD)** in a future specification phase. The `STM32G474QE` acts as a strict protocol gateway, encapsulating local DALY BMS telemetry and converter power metrics onto the System Coordinator CANbus once defined.
- **Protection Role:** Acts as the primary cell-level hardware disconnect (over-voltage, under-voltage, over-temperature, and short-circuit cutoff) independent of the Charger/Discharger control loop.

---

## Subsystem 3: Charger / Discharger Module (2.5kW Bidirectional Converter)

This module houses the **STM32G474QET6** local node controller on a 4-layer daughter card paired with a 2-layer power stage that bridges the `48V` pack to the `96VDC Energy Bus`.

- **Power Rating:** **2.5kW continuous** bidirectional power transfer (`62.5A` max battery-side current at `40.0V` deep discharge).
- **Topology:** Non-isolated 4-switch synchronous buck-boost converter switching at **50 kHz**.
- **Power Switches (`Q1–Q4`):** Four (4) single through-hole **Infineon IPW110N15N5** N-channel MOSFETs (`150V`, `110A`, `9.7mΩ`, **TO-247**) mounted to a heatsink via ceramic sil-pads.
- **Gate Drivers:** Two (2) **TI UCC21520** dual-channel isolated gate drivers (`4A` source / `6A` sink) driven by `HRTIM1_CHA1/CHA2` (`PA8`/`PA9`) and `HRTIM1_CHB1/CHB2` (`PA10`/`PA11`) with `4.7Ω–10Ω` gate resistors and isolated `15V` bias supplies.
- **Main Storage Inductor (`L1`):**
  - **Core:** **Magnetics Inc. 0077090A7** Kool Mµ (Sendust) Toroid (`57.2mm` OD).
  - **Inductance & Winding:** **37.3 µH** (`20%` current ripple allowance, `~69A` peak saturation), wound with **34 turns** of **four (4) parallel strands of 18 AWG** heavy-enameled magnet wire.
- **Snubbers & Bulk Capacitance:**
  - `0.1µF / 250V` polypropylene film snubbers (`KEMET R76`) close-coupled across switching legs.
  - Parallel banks of `220µF / 160V` low-ESR electrolytic capacitors (`United Chemi-Con`) on both battery and `96VDC` bus terminals.
- **Bus Interface & Contactors:**
  - Standardized `24VDC` coil Pre-Charge Bypass Contactor (`PC6`) with series ceramic power resistor (`10Ω–50Ω`, `50W`) and Main Isolation Contactor (`PC7`), powered via the **30V 50kHz** common auxiliary power source.
- **Telemetry Acquisition:**
  - **Inductor Current:** `Allegro ACS37002` / `ACS372` Hall-effect sensor (`PB0` / `ADC3_IN12`).
  - **Battery Voltage:** Precision divider (`47kΩ / 1.5kΩ`) (`PA0` / `ADC1_IN1`).
  - **96V Bus Voltage:** `TI AMC1311` isolated differential amplifier + `TLV271` op-amp (`PA1` / `ADC2_IN2`).
