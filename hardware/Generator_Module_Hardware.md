# Integrated Starter-Generator Node Hardware Baseline

## Module Scope
- **Operational Role:** Bidirectional current drain/source interfacing a petrol-driven **24V, 200A heavy-duty semi-truck alternator core** with the `96VDC Energy Bus`.
- **Control Architecture:** Local **STM32G474QE** 4-layer daughter card managing engine cranking, field excitation PWM, and boost rectification, paired with a `1.3-inch` OLED display.
- **Communication & Auxiliary Power:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface) via a CAN isolator whose bus-side transceiver rail is powered by the **30V 50kHz** common auxiliary power source (application protocol **TBD**). Local control bias and `24VDC` contactors are also powered from the **30V 50kHz** source.

## Power Stage & Dual-Mode Hardware
- **Topology:** High-current bidirectional synchronous buck-boost converter bridging the `24V` alternator rail and the `96VDC Energy Bus`, plus a 3-phase active rectifier bridge.
- **Semiconductors & Gate Drive:**
  - Baseplate-isolated **SOT-227 (miniBLOC)** power modules bolted to a fan-cooled aluminum heatsink and interconnected via M4-clamped solid copper busbars.
  - Standardized **Infineon** isolated gate drive modules (shared hardware design with the 5-phase boost converter gate drive stage).
- **Dual-Mode State Machine:**
  1. **Cranking / Motor Mode (`96VDC ➔ 24V`):** Bucks `96VDC` bus energy down to `24V–28V` to drive the alternator core as a high-torque starter motor.
  2. **Generation Mode (`24V ➔ 96VDC`):** Reverses power flow once the engine reaches operating RPM, modulating rotor field excitation PWM and boosting output onto the `96VDC Energy Bus`.
- **Isolated Telemetry & Bus Interface:** **Allegro ACS37002** current sensors, **TI AMC1311** voltage isolation amplifiers, and standardized `24VDC` Pre-Charge and Main Isolation contactors.
