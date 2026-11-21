# Transformerless AC Inverter Modules Hardware Baseline

## Shared Module Architecture
- **Operational Role:** Converts the `380VDC Inverter Bus` into `60Hz` household AC power using current-mode parallel control, scalable up to a `100A` residential panel.
- **Per-Module Rating:** `20A RMS` continuous / `40A` peak surge per output module.
- **Control Architecture:** Local **STM32G474QE** 4-layer daughter card with the common control card's Waveshare `1.3-inch` SPI LCD + keypad HAT ([STM32_Control_Card_Architecture.md](./STM32_control_card/STM32_Control_Card_Architecture.md)), software circuit-breaker reset support via Coordinator webpage, and local inverse-time `I²t` overload protection.
- **Communication & Auxiliary Power:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface, working registry in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)) with local logic and `24VDC` contactor coil rails derived from the **30V 50kHz** common auxiliary power source.
- **Hardware Phase Synchronization:** Sub-10ns **TI ISO7710** digital isolators intercepting a `10µs` zero-crossing pulse broadcast by the Hardware Time Master module to reset slave HRTIM timers in lockstep.

---

## Variant A: 120VAC Single-Phase Output Module
- **Topology:** Full H-bridge inverter (`Line 1` and `Neutral`).
- **Power Switches (4x):** Through-hole **onsemi FCH023N65S3L4** SuperFET III N-channel MOSFETs (`650V`, **TO-247-4L** Kelvin-source package).
- **Output Disconnect:** `24VDC` coil contactor controlled via a low-side MOSFET to provide a physical air-gap disconnect on fault trips.

---

## Variant B: 240VAC Split-Phase Output Module
- **Topology:** Three-leg bridge topology (`Line 1`, `Line 2`, and an active `Neutral` leg capable of sinking up to `40A` peak unbalanced neutral return current).
- **Power Switches (6x):** Through-hole **onsemi FCH023N65S3L4** SuperFET III N-channel MOSFETs (`650V`, **TO-247-4L** Kelvin-source package).
- **Output Disconnect:** Industrial **Schneider Electric 3-Pole 24VDC contactor** driven by a low-side MOSFET to break `Line 1`, `Line 2`, and `Neutral` simultaneously into an air-gap state upon fault trips.
