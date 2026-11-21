# 10.8kW Two-Phase Interleaved PFC Utility-Tie Hardware Baseline

## Module Scope
- **Operational Role:** Bidirectional or unidirectional grid interface converting `120VAC / 240VAC` utility power onto the `96VDC Energy Bus` (`10.8kW` continuous rating) with active power factor correction (`PFC`).
- **Control Architecture:** Local **STM32G474QE** 4-layer daughter card running `180°` out-of-phase interleaved `HRTIM1` PWM loops, paired with the common control card's Waveshare `1.3-inch` SPI LCD + keypad HAT ([STM32_Control_Card_Architecture.md](./STM32_control_card/STM32_Control_Card_Architecture.md)).
- **Communication & Auxiliary Power:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface, working registry in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)) with local control bias and `24VDC` contactors derived from the **30V 50kHz** common auxiliary power source.

## Power Stage & Sensing Requirements
- **Topology:** Two-phase interleaved active PFC stage to reduce input current ripple, shrink EMI filter volume, and distribute thermal load across parallel switching legs.
- **Semiconductors & Gate Drive:** High-voltage Superjunction MOSFETs (**SuperFET III** / **X2-Class**) and SiC Schottky rectifiers driven by **Infineon** isolated gate drivers mounted on the power board.
- **Isolated Grid & Bus Sensing:**
  - **AC Input Voltage & Zero-Crossing:** Galvanically isolated differential sensing via **TI AMC1311** across a high-impedance resistive divider to track instantaneous line phase angle and amplitude for unity power factor current shaping.
  - **Phase & Input Currents:** Isolated **Allegro ACS37002** Hall-effect sensors for cycle-by-cycle current loop regulation and overcurrent protection.
- **Grid & Bus Disconnects:** Dual `24VDC` coil safety contactors providing physical air-gap isolation on both the AC utility input and the `96VDC Energy Bus` output.
