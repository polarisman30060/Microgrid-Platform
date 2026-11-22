# Integrated Starter-Generator Node Hardware Baseline

## Module Scope
- **Operational Role:** Bidirectional current drain/source interfacing a petrol-driven **24V, 200A heavy-duty semi-truck alternator core** with the `96VDC Energy Bus`.
- **Control Architecture:** Local **STM32G474QE** 4-layer daughter card managing engine cranking, field excitation PWM, and boost rectification, paired with the common control card's Waveshare `1.3-inch` SPI LCD + keypad HAT ([STM32_Control_Card_Architecture.md](./STM32_control_card/STM32_Control_Card_Architecture.md)).
- **Communication & Auxiliary Power:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface, working registry in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)) with local control bias from `AUX_LOGIC_24V` and `24VDC` contactors/ignition solenoid from E-Stop-interlocked `AUX_COIL_24V`.

## Power Stage & Dual-Mode Hardware
- **Topology:** High-current bidirectional synchronous buck-boost converter bridging the `24V` alternator rail and the `96VDC Energy Bus`, plus a 3-phase active rectifier bridge.
- **Semiconductors & Gate Drive:**
  - Baseplate-isolated **SOT-227 (miniBLOC)** power modules bolted to a fan-cooled aluminum heatsink and interconnected via M4-clamped solid copper busbars.
  - Standardized isolated gate drive modules (`TI UCC21520` / `Infineon 1ED3122`, shared across the high-power microgrid converter stages).
- **Grounding Alignment:** Because `LV_BUS-` on the `96VDC Energy Bus` is referenced to Earth (`PE`) via system DC GFDI, a negative-grounded `24V` truck alternator aligns cleanly with `LV_BUS-`; however, high-current return paths must travel exclusively on heavy copper busbars rather than chassis steel, with isolated Hall sensing (`ACS37002`) preventing ground-loop measurement errors.
- **Dual-Mode State Machine:**
  1. **Cranking / Motor Mode (`96VDC ➔ 24V`):** Bucks `96VDC` bus energy down to `24V–28V` to drive the alternator core as a high-torque starter motor.
  2. **Generation Mode (`24V ➔ 96VDC`):** Reverses power flow once the engine reaches operating RPM, modulating rotor field excitation PWM and boosting output onto the `96VDC Energy Bus`.
- **Isolated Telemetry & Bus Interface:** **Allegro ACS37002** current sensors, **TI AMC1311** voltage isolation amplifiers, and standardized `24VDC` Pre-Charge and Main Isolation contactors.
