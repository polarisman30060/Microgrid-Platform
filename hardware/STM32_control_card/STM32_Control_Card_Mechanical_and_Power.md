# STM32 Control Card Power, Isolation, Connectors, and Mechanical Design

Status: Draft hardware baseline (v0.1). Component values, isolation creepage/clearance, thermal dissipation, and mechanical clearances must be verified in schematic capture and PCB layout before fabrication.

**Layout hold:** This is a concept, not a validated circuit or assembly procedure. In particular, the proposed TVS does not establish protection for the buck's 40 V input; the proposed CAN converter can overvolt the transceiver at light load and the isolation system is not qualified. Connector mating, clock circuits, and debugger pinout also require independent checks. Do not order or energize a board from this draft.

## 1. Power Tree and Input Protection

The host power board supplies regulated, isolated **24 VDC** (`+24V_IN` / `GND_CTRL`) across `J1` pins 1–4. The control card regulates `5.0 V` and `3.3 V` locally and creates a galvanically isolated `5.0 V` domain for the CAN bus transceiver.

```text
J1 (+24V_IN)
  |
  +--> [MF-R050] 0.5 A PTC Fuse
  |
  +--> [B360A-13-F] Reverse-Polarity Schottky + [SMBJ33A] TVS to GND_CTRL
  |
  v
[LM2675M-5.0/NOPB] 40 V, 1 A Buck (8-SOIC, 260 kHz) --> +5V_CTRL
  |
  +---> [TLV76133DCYR] 1 A Low-Noise LDO (SOT-223) ---> +3V3_DIG (MCU, LCD HAT, ISO1042 VCC1)
  |        |
  |        +---> Ferrite Bead / 10 Ohm + 10 nF + 1 uF -> +3V3_ANA (VDDA, VREF+)
  |
  +---> [isolated DC/DC: candidate under review] -> +5V_CAN_ISO / GND_CAN_ISO
                                                            |
                                                            v
                                                    [ISO1042DWV] CAN Bus Side (VCC2)
```

### Design Rationale & Hand-Solderability

- **Input Protection (`MF-R050` + `B360A-13-F` + `SMBJ33A`):** Proposed fuse, reverse-polarity diode and TVS require verification. The TVS's listed `53.3 V` maximum clamp exceeds the buck's `40 V` operating ceiling; this combination does not demonstrate adequate transient protection. Define the host input range and transients, then design and validate suitable protection.
- **24 V → 5.0 V Buck (`LM2675M-5.0/NOPB`):** Selected in **8-SOIC (no exposed bottom thermal pad)** specifically for easy hand soldering, fixed `5.0 V` output (no feedback divider needed), and `6.5 V–40 V` input range. Per [TI_LM2675.pdf](../../docs/datasheet/TI_LM2675.pdf) (Table 7-2 & 7-3), pair with `100 µH / 1.7 A` shielded inductor (`SRR1260-101M`), `B360A-13-F` catch diode, `22 µF / 50 V` input cap (`EEU-FR1H220`) + `100 nF`, `10 nF` bootstrap cap (`CC0805KRX7R9BB103`), and `100 µF / 25 V` low-ESR output cap (`EEU-FR1E101`).
- **5.0 V → 3.3 V LDO (`TLV76133DCYR`):** Selected in **SOT-223** (large leaded pads, easy to hand solder), `1 A` rating, `60 dB` PSRR at `1 kHz`, and **stable with standard `1 µF` ceramic capacitors** ([TI_TLV761.pdf](../../docs/datasheet/TI_TLV761.pdf)). Filter `3V3_DIG` into `VDDA`/`VREF+` through Murata `BLM21AG601SN1D` (`600 Ω @ 100 MHz`, `0805`). Estimated `3.3 V` load is ~120–150 mA (`~0.25 W` LDO dissipation at `5.0 V → 3.3 V`, well within SOT-223 thermal limits).
- **Load Budget:** The earlier estimates for `+5V_CTRL` and `3V3_DIG` are provisional and must be recalculated for the selected isolated converter, display, startup, fault conditions and temperature. They do not establish current or thermal margin.

---

## 2. Isolated CAN Bus Interface

- **Transceiver:** **TI `ISO1042DWV`** (`8-SOIC` wide-body, `1.27 mm` pin pitch—very easy to hand solder). Rated for **`5.0 kVrms`** withstand isolation, `1060 Vrms` working voltage, `±70 V` bus fault protection, and `8 mm` minimum creepage/clearance ([TI_ISO1042.pdf](../../docs/datasheet/TI_ISO1042.pdf)).
- **Isolated Supply:** **Murata `MEJ1S0505SC`** (`5 V` → `5 V`, `1 W`, `200 mA`, `7-SIP` through-hole package) is a candidate. Its `5.2 kVDC` test rating alone does not establish a kVrms rating, reinforced insulation, working-voltage suitability, or compliance with the communications specification. Verify its datasheet and the complete barrier against the system requirements.
- **Supply compatibility — unresolved:** The [MEJ1 datasheet](../../docs/datasheet/Murata_MEJ1S0505SC.pdf), page 5, requires at least 10% of rated load (20 mA for this model) to meet its specifications and says output voltage may rise to typically twice nominal below 5% load. The [ISO1042 datasheet](../../docs/datasheet/TI_ISO1042.pdf), page 9, specifies CAN-side recessive current of 2.8 mA typical / 4.1 mA maximum under its stated test conditions, and VCC2's recommended operating range is 4.5–5.5 V. With only the transceiver loading it, the converter cannot be assumed to provide a safe output at idle. Do not connect this converter directly to VCC2 as an approved design; select and validate a suitable regulated supply or a complete regulation/protection circuit over startup, load and input conditions.
- **Converter isolation details:** The MEJ1 datasheet, pages 2–3, lists a 5.2 kVDC production withstand test for one second, UL 62368-1 recognition at 150 Vrms reinforced or 300 Vrms basic working voltage, and 2 mm creepage/clearance. Those distinct figures are not interchangeable. The system specification's greater-than-2.5 kVrms wording also needs a defined test method and duration, working voltage, insulation class and applicable standard before compatibility can be determined.
- **Replacement candidate:** TI `UCC12050DVER` is a regulated isolated converter in a 16-pin surface-mount DVE package. The [datasheet](../../docs/datasheet/TI_UCC12050.pdf) specifies 5 kVrms for one minute under UL 1577 and 1.2 kVrms maximum working voltage; these are component ratings, not board-level approval. In 5.0 V mode, 4.7–5.3 V is specified at a 55 mA test load; the 0–100 mA load-regulation figure is typical, not a guaranteed full-range limit. ISO1042 specifies up to 73.4 mA dominant current at its stated test conditions. Verify the converter's output range and recommended operating area over the actual load, input and temperature, including idle, startup and transient behavior, and recalculate input power, before connecting or approving it. Its 10 µF input/output capacitors and other required connections must be designed from the datasheet. Availability, supplier SKU and price remain unverified.
- **Preferred candidate for further review:** TI `UCC33421DHAR` provides a selectable regulated 5.0 V isolated output. Its [datasheet](../../docs/datasheet/TI_UCC33421.pdf), page 9, specifies 4.85–5.15 V at 5.0 V input and 0–300 mA output, and ±4% output accuracy over 4.5–5.5 V input under the specified conditions. The 5.5 V output setting is unsuitable for ISO1042's 4.5–5.5 V operating range. The 16-pin DHA package has 0.65 mm lead pitch, more demanding to hand-solder than the original SIP. Its 5 kVrms/60 s component withstand test is not a working voltage or system approval; verify the system requirements, operating area, input supply, startup/overshoot, output capacitance and complete barrier before wiring or release. TI's order page showed out of stock on 2026-11-20; DigiKey SKU, price and availability are unverified.
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
2. Solder SMD power, CAN, and clock ICs (`LM2675M-5.0`, `TLV76133`, `ISO1042DWV`, `B360A-13-F`, `SMBJ33A`, `NUP2105LT1G`, `SRR1260-101M`, `ECS-240-8-30B-CKM`, `ECS-.327-6-17X-TR`, `BLM21AG601SN1D`) and `0805` passives/LEDs.
3. Solder through-hole power & interface parts (`MF-R050`, `EEU-FR1H220`, `EEU-FR1E101`, `MEJ1S0505SC`, `1729131` CAN terminal, `B3F-1000` reset button, `J5` SWD header, `J_TERM` / `BOOT0` jumpers).
4. **Power-On Smoke Test (Before mounting LCD HAT or plugging onto Host):** Apply bench `24 VDC` (current-limited to `100 mA`) to `J1` pins 1/3, verify `+5V_CTRL = 5.0 V`, `+3V3_DIG = 3.3 V`, `+5V_CAN_ISO = 5.0 V`, and flash a blink/CAN test over `SWD`.
5. Solder bottom-side host headers `J1`/`J2` (using a mating host socket or jig to keep pins perpendicular) and top-side `J4` LCD header, then bolt the Waveshare HAT onto the M2.5 standoffs.
