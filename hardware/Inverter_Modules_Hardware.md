# Transformerless AC Inverter Modules Hardware Baseline

## Shared Module Architecture
- **Operational Role:** Converts the galvanically isolated, earth-referenced **`400VDC` (`±200VDC` split-rail) Inverter Bus** (`HV+ = +200 V`, `HV_MID = 0 V = N = PE`, `HV- = -200 V`) into `60 Hz` household AC power using current-mode parallel control, scalable up to a `100 A` residential panel.
- **Why Half-Bridge Referenced to `HV_MID` (`N/PE`):** Because the upstream 2-phase FB-LLC Boost converter isolates the `400VDC` split-capacitor bus and its center tap (`HV_MID`) is single-point bonded to Earth (`PE`) as AC Neutral, each AC hot leg (`L1`, `L2`) is synthesized by a single half-bridge leg switching between `+200 VDC` and `-200 VDC` relative to `HV_MID`. This provides `30 V` nominal modulation headroom above `170 V` (`120 VAC` peak), cuts switch count compared to full/3-leg bridges, eliminates high-frequency common-mode switching voltage on Neutral, and supports standard residential AC GFCI/RCD personnel protection.
- **Per-Module Rating:** `20 A RMS` continuous / `40 A` peak surge per output module.
- **Control Architecture:** Local **STM32G474QET6** 4-layer daughter card with the common control card's Waveshare `1.3-inch` SPI LCD + keypad HAT ([STM32_Control_Card_Architecture.md](./STM32_control_card/STM32_Control_Card_Architecture.md)), software circuit-breaker reset support via Coordinator webpage, local inverse-time `I²t` overload protection, and residual-current (GFCI) leakage detection.
- **Communication & Auxiliary Power:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface, working registry in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)) with local logic power from `AUX_LOGIC_24V` and `24 VDC` contactor coils from E-Stop-interlocked `AUX_COIL_24V`.
- **DC-Side Disconnect:** 2-pole DC disconnect breaking both ungrounded rails (`HV+` and `HV-`) while `HV_MID` stays solidly referenced to the Neutral-Ground bond.
- **Hardware Phase Synchronization:** Isolated digital sync link (`HRTIM1_SCOUT` / `HRTIM1_SCIN` or dedicated timer pulse — exact pulse format and Time-Master failover under definition per [`STM32_control_card/Inverter_Module_IO_Requirements.md`](./STM32_control_card/Inverter_Module_IO_Requirements.md)).

---

## Variant A: 120 VAC Single-Phase Output Module
- **Topology:** **Single-leg Half-Bridge** (`Line 1` switching between `+200 VDC` and `-200 VDC` through an LC output filter referenced to `HV_MID` = `Neutral` / `PE`).
- **Power Switches (`2×`):** Through-hole **onsemi `FCH023N65S3L4`** SuperFET III N-channel MOSFETs (`650 V`, **TO-247-4L** Kelvin-source package), driven by `HRTIM1_CHA1/CHA2`.
- **Output Disconnect:** Single-pole `24 VDC` coil AC line contactor breaking `Line 1` into a physical air-gap state on fault trips (Neutral remains continuously bonded per residential electrical safety practice), paired with the 2-pole `HV+`/`HV-` DC disconnect.

---

## Variant B: 120/240 VAC Split-Phase Output Module
- **Topology:** **Two-leg Half-Bridge** (`Line 1` via `HRTIM1_CHA1/CHA2` and `Line 2` via `HRTIM1_CHB1/CHB2`, driven `180°` out of phase between `+200 VDC` and `-200 VDC` around `HV_MID` = `Neutral` / `PE`). Unbalanced neutral return current (`up to 40 A` peak) returns directly into the split-capacitor midpoint `HV_MID` and is magnetically balanced across the upstream FB-LLC center-tapped transformer secondaries (`HRTIM1_CHC1/CHC2` remains available on `J2` if an optional local active midpoint-balancer leg is ever populated).
- **Power Switches (`4×`):** Through-hole **onsemi `FCH023N65S3L4`** SuperFET III N-channel MOSFETs (`650 V`, **TO-247-4L** Kelvin-source package).
- **Output Disconnect:** Industrial **2-Pole `24 VDC` AC line contactor** driven by a low-side MOSFET to break `Line 1` and `Line 2` simultaneously into an air-gap state upon fault trips (Neutral remains continuously bonded to `HV_MID / PE`), paired with the 2-pole `HV+`/`HV-` DC disconnect.
