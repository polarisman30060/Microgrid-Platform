# 10kVA 5-Phase Interleaved Boost Module Hardware Baseline

## Module Scope
- **Operational Role:** Steps up the `96VDC Energy Bus` (`±5%` regulation) to the `380VDC Inverter Bus` at **10kVA continuous** (`15kW` short-term peak) per module.
- **Control Architecture:** Local **STM32G474QE** 4-layer daughter card running `HRTIM1` with `72°` hardware phase shifting, active per-phase current balancing, and dynamic phase shedding across 5 interleaved stages, paired with the common control card's Waveshare `1.3-inch` SPI LCD + keypad HAT ([STM32_Control_Card_Architecture.md](./STM32_control_card/STM32_Control_Card_Architecture.md)).
- **Communication:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface, `500 kbps` Classic CAN via `TI ISO1042DWV` on the common control card) supporting master/slave current-matching load balancing and Coordinator telemetry per [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md) (`TELEM_BOOST_FAST` `0x300+`, draft `TELEM_BOOST_SLOW` `0x310+`).

## Power Stage & Semiconductors
- **Switching Frequency:** **40 kHz per phase** (`200 kHz` effective ripple frequency across the 5 interleaved stages).
- **Power Switches (5x):** Baseplate-isolated **IXYS IXFN150N65X2** (or `IXFN100N65X2`) X2-Class Superjunction MOSFETs in **SOT-227 (miniBLOC)** packages, bolted to a fan-cooled aluminum heatsink and linked via M4-clamped solid copper busbars.
- **Boost Rectifiers (5x):** **SemiQ GHXS100B065S-D3** dual isolated `650V` SiC Schottky diode arrays in **SOT-227 (miniBLOC)** packages.
- **Gate Drivers (5x):** **Infineon 1ED3122MC12H** isolated single-channel gate drivers with Active Miller Clamping, mounted natively on the power board.
- **Phase-Shedding Hardware Support:** Each of the 5 phases features independent `HRTIM1` PWM gating (`Infineon 1ED3122MC12H`) and dedicated per-phase current sensing (`Allegro ACS37002`), allowing the local controller to dynamically disable idle phases and re-index phase-shift angles (`72°`, `90°`, `120°`, `180°`) at light loads.

## Magnetics & Bulk Capacitance
- **Phase Inductors (5x):** Hand-wound **Magnetics Inc. 0077735A7** Kool Mµ (Sendust) `74mm` OD toroid cores, each wound with **63 turns** of a **23-strand 24 AWG** heavy-enameled magnet wire ribbon bundle.
- **380VDC Bulk Capacitance:** Separate external bulk capacitor board combining parallel high-voltage aluminum electrolytic snap-in cans with `0.1µF / 630V` polypropylene film snubbers to stabilize the `380VDC` bus and slow voltage control loop demands.

## Isolation, Protection & Telemetry
- **Bus Interface & Auxiliary Power:** Standardized `24VDC` coil Pre-Charge Bypass Contactor with series ceramic power resistor (`10Ω–50Ω`, `50W`) and Main Isolation Contactor, plus control logic bias derived from the **30V 50kHz** common auxiliary power source.
- **Isolated Telemetry:** Phase and bus currents captured via **Allegro ACS37002** Hall-effect sensors (configured with elevated overcurrent trip thresholds); `96VDC` and `380VDC` bus voltages sensed via **TI AMC1311** isolated differential amplifiers.
