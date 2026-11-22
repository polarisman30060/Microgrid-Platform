# 10.8 kW Galvanically Isolated Utility-Tie Charger Hardware Baseline

## Module Scope
- **Operational Role:** Galvanically isolated, power-factor-corrected (`PFC`) grid interface converting `120 VAC / 240 VAC` utility power onto the negative-grounded `96VDC Energy Bus` (`10.8 kW` continuous rating at `240 VAC`, derated at `120 VAC`).
- **Why Two-Stage Isolation Is Mandatory:** Because Utility Neutral is bonded to Earth (`PE`) at the residential service entrance AND `LV_BUS-` on the `96VDC Energy Bus` is referenced to `PE` via DC GFDI, a non-isolated PFC tied directly to `96VDC` would create a direct AC-to-Earth short circuit on negative half-cycles (and cannot step `240 VAC` down to `96 VDC`). A two-stage isolated topology eliminates all ground loops across every operating mode.
- **Control Architecture:** Local **STM32G474QET6** 4-layer daughter card running `HRTIM1` across both stages (`180°` interleaved PFC + isolated full-bridge DC-DC), paired with the common control card's Waveshare `1.3-inch` SPI LCD + keypad HAT ([STM32_Control_Card_Architecture.md](./STM32_control_card/STM32_Control_Card_Architecture.md)).
- **Communication & Auxiliary Power:** Galvanically isolated **CANbus** (`STM32G474QE` `FDCAN` interface, working registry in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)) with local control bias from `AUX_LOGIC_24V` and `24 VDC` contactors from E-Stop-interlocked `AUX_COIL_24V`.

## Two-Stage Power Architecture & Sensing Requirements
- **Stage 1 — Two-Phase Interleaved Active PFC (`120/240 VAC` → Internal `400 VDC` Link):**
  - Two `180°` out-of-phase interleaved boost PFC legs (`HRTIM1_CHA1`, `HRTIM1_CHB1`) to reduce input current ripple, shrink EMI filter volume, and achieve `> 0.99` unity power factor.
  - High-voltage `650 V` Superjunction MOSFETs (**SuperFET III** / **X2-Class**) and SiC Schottky rectifiers driven by isolated gate drivers.
- **Stage 2 — Galvanically Isolated DC-DC Step-Down (`Internal 400 VDC` → `96 VDC` Energy Bus):**
  - Full-Bridge LLC (`FB-LLC`) or Phase-Shifted Full-Bridge (`PSFB`) converter (`HRTIM1_CHC1/CHC2`, `HRTIM1_CHD1/CHD2`) with a **reinforced-isolation (`≥ 4 kVrms`) HF transformer** stepping the internal `400 VDC` PFC link down to `91.2–100.8 VDC` with closed-loop current-limited charging onto the `96VDC Energy Bus`.
  - Secondary rectification (`200 V` synchronous MOSFETs or low-`V_F` Schottky/SiC diodes) blocks reverse current from the `96 VDC` bus.
- **Isolated Grid & Bus Sensing:**
  - **AC Input Voltage & Zero-Crossing:** Galvanically isolated differential sensing via **TI `AMC1311`** across a high-impedance resistive divider to track instantaneous line phase angle and amplitude for unity-PF current shaping and grid-window/anti-islanding checks.
  - **Internal `400 VDC` Link Voltage & `96 VDC` Bus Voltage:** Separate **TI `AMC1311`** isolated differential amplifiers.
  - **AC Input, PFC Phase, and `96 VDC` Output Currents:** Isolated **Allegro `ACS37002`** Hall-effect sensors for cycle-by-cycle current loop regulation and hardware overcurrent protection (`HRTIM1_FLT1`).
- **Grid & Bus Disconnects:** `24 VDC` coil safety contactors providing physical air-gap isolation on the AC utility input (2-pole `L1`/`L2` or `L1`/`N` per service config) and standardized `Q_PRE` + `Q_ISOL` on the `96VDC Energy Bus` output.
