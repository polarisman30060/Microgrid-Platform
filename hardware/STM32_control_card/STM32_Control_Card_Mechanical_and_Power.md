# STM32 Control Card Power, Isolation, Connectors, and Mechanical Design

Status: Draft hardware baseline (v0.2, 2026-11-22). Both P1 power-input surge and isolated CAN supply component holds are resolved in the BOM (`LM2576HVT-5.0/NOPB` + `SMBJ30A` + `SRR1280-331K` and `ADuM6020-5BRIZ`). Final schematic capture (EasyEDA Pro), ERC/DRC, isolation creepage/clearance layout verification, and bench thermal/transient testing remain required before PCB fabrication.

## 1. Power Tree and Input Protection

The host power board supplies regulated, isolated **24.0 VDC** (`AUX_LOGIC_24V` / `GND_CTRL`) across `J1` pins 1–4. The control card regulates `5.0 V` and `3.3 V` locally and creates a galvanically isolated, regulated `5.0 V` domain for the CAN bus transceiver.

**Design Target (Resolved 2026-11-22):** At the card input, nominal voltage is `24.0 VDC` (`21.6–28.0 V` normal diode-OR'd operating window, `30.0 V` upper steady ceiling, positive surge clamped `≤ 48.4 V` by `SMBJ30A`). Pairing **`SMBJ30A`** (`V_RWM = 30.0 V`, `V_C max = 48.4 V`) with the **`60 V`-rated `LM2576HVT-5.0/NOPB`** (`63 V` abs max), `60 V` `B360A-13-F` diodes, and `63 V` `EEU-FC1J101` input capacitor establishes a **`≥ 11.6 V` guaranteed voltage margin** across worst-case TVS clamping.

```text
J1 (+24V_IN / AUX_LOGIC_24V)
  |
  +--> [MF-R050] 0.5 A PTC Fuse (60 V)
  |
  +--> [B360A-13-F] Reverse-Polarity Schottky (60 V, 3 A) + [SMBJ30A] TVS (V_C <= 48.4 V) to GND_CTRL
  |
  v
[LM2576HVT-5.0/NOPB] 60 V, 3 A Buck (TO-220-5, 52 kHz, 330 uH SRR1280-331K) --> +5V_CTRL
  |
  +---> [TLV76133DCYR] 1 A Low-Noise LDO (SOT-223) ---> +3V3_DIG (MCU, LCD HAT, ISO1042 VCC1)
  |        |
  |        +---> Ferrite Bead (BLM21AG601SN1D) + 10 nF + 1 uF -> +3V3_ANA (VDDA, VREF+)
  |
  +---> [ADuM6020-5BRIZ] 5 kVrms Regulated Isolated DC/DC (16-SOICW) -> +5V_CAN_ISO / GND_CAN_ISO
                                                                              |
                                                                              v
                                                                      [ISO1042DWVR] CAN Bus Side (VCC2)
```

### Design Rationale & Hand-Solderability

- **Input Protection (`MF-R050` + `B360A-13-F` + `SMBJ30A`):** `SMBJ30A` stays non-conducting across the full `21.6–30.0 V` operating range (`V_BR min = 33.3 V`) and clamps surges to `≤ 48.4 V`, leaving `11.6 V` of margin below the `60 V` `LM2576HV` operating ceiling, `60 V` `B360A-13-F` reverse-polarity/catch diodes, and `63 V` `EEU-FC1J101` input electrolytic.
- **24 V → 5.0 V Buck (`LM2576HVT-5.0/NOPB`):** Selected in **through-hole `TO-220-5`** (stocked on DigiKey, effortless hand-soldering, fixed `5.0 V` output with zero feedback resistors, `60 V` max operating input / `63 V` abs max). Pair with stocked **`Bourns SRR1280-331K`** (`330 µH`, `1.1 A` RMS / `1.4 A` sat, exceeding `1.13 A` peak at `1 A` load), `B360A-13-F` catch diode, `100 µF / 63 V` low-ESR input cap (`EEU-FC1J101`) + `100 nF`, and `1,000 µF / 25 V` low-ESR output cap (`EEU-FC1E102`).
- **5.0 V → 3.3 V LDO (`TLV76133DCYR`):** Selected in **SOT-223** (large leaded pads, easy to hand solder), `1 A` rating, `60 dB` PSRR at `1 kHz`, and **stable with standard `1 µF` ceramic capacitors** ([TI_TLV761.pdf](../../docs/datasheet/TI_TLV761.pdf)). Filter `3V3_DIG` into `VDDA`/`VREF+` through Murata `BLM21AG601SN1D` (`600 Ω @ 100 MHz`, `0805`). Estimated `3.3 V` load is ~120–150 mA (`~0.25 W` LDO dissipation at `5.0 V → 3.3 V`, well within SOT-223 thermal limits).
- **Load Budget:** Recalculate and bench-verify thermal rise under worst-case `1 A` load, startup, and ambient cabinet temperature during board bring-up.

### Working Supply Design Targets

Design the `5V_CTRL` buck for **1 A continuous** across `21.6–30.0 V` input and survive `48.4 V` TVS-clamped transients.

| 5 V load allocation | Allowance |
| --- | ---: |
| 3.3 V LDO input, including MCU, display, logic-side CAN and host reference loads | 400 mA |
| Isolated CAN converter (`ADuM6020-5BRIZ`) input (~`180–220 mA` at full `ISO1042` dominant load) | 250 mA |
| Reserve for uncertainty and future card loads | 350 mA |
| **Total buck design load** | **1 A** |

Within the 400 mA 3.3 V allowance, reserve up to **100 mA for host circuitry** on `3V3_REF`; the other 300 mA is for the control card and display. At 400 mA, the LDO dissipates `(5.0 - 3.3) × 0.4 = 0.68 W` at nominal voltages; provide generous SOT-223 copper pour on `L1`/`L3` for heatsinking.

**Buck Inductor Verification (`Bourns SRR1280-331K`):** Per TI's `LM2576HV` `5 V` selection chart (`H330` region for `1 A` and `40–48 V` max input), `330 µH` at `52 kHz` yields `ΔI_pp ≈ 0.255 A` and `I_pk ≈ 1.13 A` at `1 A` continuous load. Stocked DigiKey **`Bourns SRR1280-331K`** (`330 µH ±10%`, shielded `12.5×12.5×8 mm` SMD) is rated **`1.1 A` continuous / `1.4 A` saturation** (`24%` headroom above `1.13 A` peak). Verify temperature rise and startup/short-circuit behavior on the prototype bench.

---

## 2. Isolated CAN Bus Interface

- **Transceiver:** **TI `ISO1042DWVR`** (or `ISO1042DWV` / `ISO1042BDWVR`, `8-SOIC` wide-body, `1.27 mm` pin pitch—very easy to hand solder). Rated for **`5.0 kVrms`** withstand isolation, `1060 Vrms` working voltage, `±70 V` bus fault protection, and `8 mm` minimum creepage/clearance ([TI_ISO1042.pdf](../../docs/datasheet/TI_ISO1042.pdf)).
- **Regulated Isolated Supply (Resolved 2026-11-22):** **Analog Devices `ADuM6020-5BRIZ`** (`505-ADUM6020-5BRIZ-ND`, **in stock on DigiKey**, wide-body `16-SOIC` `1.27 mm` pitch — effortless to hand-solder compared to `0.65 mm` DHA packages!).
  - Rated **`5.0 kVrms` for 1 minute (UL 1577)** with **`8.3 mm` minimum creepage and clearance** matching `ISO1042DWVR`.
  - **Fully regulated `5.0 V ±5%` (`4.75–5.25 V`) down to `0 mA` no-load**, completely resolving the `MEJ1S0505SC` light-load overvoltage failure mode at `ISO1042`'s `2.8–4.1 mA` recessive idle current.
  - Rated **`500 mW` (`100 mA @ 5.0 V`)** output, providing ample margin over `ISO1042`'s `73.4 mA` max dominant current.
  - Bypass both `VDDP` and `VISO` immediately at the pins with **`10 µF` (`CC0805KKX7R7BB106`) + `100 nF` (`CC0805KRX7R9BB104`)** ceramic capacitors per the ADI datasheet, and stitch a clean `≥ 8.0 mm` copper moat across all 4 layers.
  - *(Note: TI `UCC33421DHAR` and `UCC12050DVER` were checked on 2026-11-22 and found non-stock / out-of-stock on DigiKey; `ADuM5020-5BRWZ` is a stocked `3 kVrms` lower-cost fallback if `5 kVrms` is not required.)*
- **CAN Protection & Split Termination:**
  - **ESD / Surge:** `onsemi NUP2105LT1G` (`SOT-23`) placed directly at the CAN connector between `CANH`/`CANL` and `GND_CAN_ISO`.
  - **Selectable End Termination:** Two `60.4 Ω` (`1%`, `0805`) resistors in series across `CANH`–`CANL` with a `4.7 nF / 100 V` midpoint capacitor to `GND_CAN_ISO`, enabled **only** when a 2-pin `2.54 mm` jumper shunt (`J_TERM`, `PRPC002SAAN-RC`) is installed at physical bus ends.
  - **CAN Connector (`J_CAN`):** **Phoenix Contact `1729131`** (`MKDSN 1,5/ 3-5,08`, 3-position `5.08 mm` screw terminal: `CANH`, `CANL`, `GND_CAN_ISO`).
- **PCB Isolation Barrier Rules:**
  - Keep the control and CAN-side domains separated on all layers. Determine the actual clearance, creepage, component geometry and PCB keep-outs from the complete isolation requirements and applicable standards; an arbitrary `8.0 mm` copper gap alone does not establish compliance. Never route `GND_CTRL` planes under the CAN connector or termination components.

---

## 3. MCU Support Hardware (Nucleo-G474RE Style)

- **Oscillators:**
  - **HSE:** `24.000 MHz` `5.0 × 3.2 mm` 4-SMD crystal (`ECS-240-8-30B-CKM`, `8 pF` load, `30 Ω` ESR; `gm_crit ≈ 0.33 mA/V` vs `1.5 mA/V` max in DS12288 Table 41) with two `10 pF` `0805` NP0 load capacitors (`CC0805JRNPO9BN100`) placed immediately next to `PF0`/`PF1`.
  - **LSE:** `32.768 kHz` wide-lead `4-SOJ` crystal (`ECS-.327-6-17X-TR`, `6 pF` load, `50 kΩ` ESR; `gm_crit ≈ 0.54 µA/V`) with two `6.8 pF` `0805` NP0 load capacitors (`CC0805CRNPO9BN6R8`) on `PC14`/`PC15`.
- **Decoupling (Per DS12288 Rev 6 Figure 16):**
  - `8× 100 nF` (`CC0805KRX7R9BB104`, `0805` X7R) — one directly adjacent to each `VDD`/`VSS` pin pair (plus LCD, CAN, keypad debounce, and NRST)
  - `1× 4.7 µF` (`CC0805KKX7R7BB475`, `0805` X7R) bulk cap near `VDD`
  - `1× 10 nF` (`CC0805KRX7R9BB103`) + `1× 1 µF` (`CC0805KKX7R9BB105`) on `VDDA`/`VSSA`
  - `1× 100 nF` + `1× 1 µF` on `VREF+` (pins 43–44) to `VSSA`
  - `1× 100 nF` on `VBAT` (pin 6)
- **Debug (`J5` SWD Header):**
  - **1×6 `2.54 mm` header (`PRPC006SAAN-RC`)**, proposed pinout below; check the actual debugger and cable pinout before connecting:
    1. `VDD_TARGET` (`3V3_DIG` sense — do NOT power the board from pin 1 without a Schottky isolator)
    2. `SWCLK` (`PA14`)
    3. `GND` (`GND_CTRL`)
    4. `SWDIO` (`PA13`)
    5. `NRST` (`PG10-NRST`)
    6. `SWO` (`PB3`, `JTDO-TRACESWO` AF0 — optional `100 Ω` series damping)
- **Passives Package Standard:** Use **`0805` (2012 metric)** resistors, capacitors, and LEDs everywhere possible (minimum `0603` only where pin density around LQFP128 requires it) so the entire card can be comfortably hand-soldered with standard tweezers and a chisel/bevel iron tip.

---

## 4. Standard 2.54 mm Host Headers & Mechanical Layout

Because the card will be largely hand-soldered and plugged onto multiple host power boards, all board-to-board connections use **standard `2.54 mm` (`0.100"`) pitch headers**:

- **Host Connectors (`J1` and `J2`):**
  - Control Card uses **two `2×20` (`40-pin`) keyed shrouded male box headers** (**Sullins `SBH11-PBPC-D20-ST-BK`**) mounted on the **bottom side** facing down into the host board.
  - Host Power Boards use matching **two `2×20` (`40-pin`) keyed female board-to-board sockets** (**Sullins `SFH11-PBPC-D20-ST-BK`**).
  - **Polarization / Anti-Reversal:** The shrouded box wall and center polarization slot on `SBH11`/`SFH11` mechanically prevent off-by-one row/column shifts and reversed insertion. Placing `J1` (Analog/Power) and `J2` (Fast PWM/Fault/Sync) on opposite parallel edges of the card keeps switching `dV/dt` away from ADC lines and gives mechanical stability.
- **Expansion Header (`J3`):**
  - **One `2×12` (`24-pin`) `2.54 mm` header** (`PRPC012DAAN-RC`) exposing the 19 remaining unallocated MCU pins + 5 `GND_CTRL` pins (populate top-side or bottom-side as needed per host).
- **Waveshare 1.3-inch LCD HAT Mounting (`J4`):**
  - Mounted on the **top side** of the control card via one **`2×20` (`40-pin`) `2.54 mm` unshrouded male pin header** (**Sullins `PRPC020DAAN-RC`**) plugging directly into the HAT's underside 40-pin Raspberry Pi socket.
  - **Mechanical Support:** Include **4× M2.5 mounting holes** aligned to the Waveshare HAT (`65.0 × 30.2 mm` outline; verify exact hole pitch — nominally `58.0 × 23.0 mm` RPi Zero pattern — with physical calipers on the HAT before Gerber release). Secure the HAT with **4× `11 mm` M2.5 hex standoffs** (**Würth `970110151`**) and **8× `M2.5 × 6 mm` pan-head nylon screws** (**Essentra `50M025045P006`**) so button/joystick presses never flex the solder joints.
- **PCB Stackup & Target Dimensions:**
  - **4-Layer FR-4** (`1.6 mm`, `1 oz` copper):
    - `L1 (Top)`: Signal + `3V3_DIG` / `24V` local pours + LCD HAT & MCU
    - `L2 (Inner 1)`: Solid `GND_CTRL` reference plane (with ≥8 mm keep-out moat under CAN isolation barrier)
    - `L3 (Inner 2)`: Power plane (`3V3_DIG`, `3V3_ANA`, `5V_CTRL`) + `GND_CTRL`
    - `L4 (Bottom)`: Signal + Host headers `J1`/`J2`
  - **Suggested Card Outline:** ~`95 mm × 70 mm` (wide enough for the `65 × 30.2 mm` LCD HAT in the center, `J1`/`J2` along left/right edges, `CAN` terminal + isolation moat on top/end edge away from `J1`/`J2`, and `SWD` + `Reset` accessible even with the HAT mounted).

---

## 5. Hand-Soldering Assembly Order Recommendation

1. Solder `STM32G474QET6` (`LQFP128`, `0.4 mm` pitch) first using tack-and-drag with plenty of no-clean gel flux and solder wick. Inspect all 128 pins under magnification for bridges before installing tall parts.
2. Solder SMD power, CAN, and clock ICs (`TLV76133DCYR`, `ISO1042DWVR`, `ADuM6020-5BRIZ`, `B360A-13-F`, `SMBJ30A`, `NUP2105LT1G`, `SRR1280-331K`, `ECS-240-8-30B-CKM`, `ECS-.327-6-17X-TR`, `BLM21AG601SN1D`) and `0805` passives/LEDs.
3. Solder through-hole power & interface parts (`MF-R050`, `LM2576HVT-5.0/NOPB`, `EEU-FC1J101`, `EEU-FC1E102`, `1729131` CAN terminal, `B3F-1000` reset button, `J5` SWD header, `J_TERM` / `BOOT0` jumpers).
4. **Power-On Smoke Test (Before mounting LCD HAT or plugging onto Host):** Apply bench `24 VDC` (current-limited to `100 mA`) to `J1` pins 1/3, verify `+5V_CTRL = 5.0 V`, `+3V3_DIG = 3.3 V`, `+5V_CAN_ISO = 5.0 V`, and flash a blink/CAN test over `SWD`.
5. Solder bottom-side host headers `J1`/`J2` (using a mating host socket or jig to keep pins perpendicular) and top-side `J4` LCD header, then bolt the Waveshare HAT onto the M2.5 standoffs.
