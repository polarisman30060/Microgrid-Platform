# STM32 Control Card Bill of Materials

Status: Rev 0.4 Verified-MPN & Priced Draft BOM (2026-11-22) — **Component selection for both P1 power/isolation holds (`LM2576HVT-5.0/NOPB` + `SMBJ30A` and `ADuM6020-5BRIZ`) is now resolved with stocked DigiKey parts; still requires EasyEDA schematic sync, ERC/DRC, and live cart verification before ordering.** Bare 4-layer PCB fabrication, shipping, and tax are not included.

*Repository BOM Standard:* DigiKey (`digikey.com`) is preferred for all components, with active MPNs, verified availability notes, and **both Qty 1 and Qty 10 USD pricing** (`Ext. @ Qty 1` = `Qty × Qty 1 Price`; `Ext. @ Qty 10` = `Qty × Qty 10 Price`, showing per-card cost at 10-card volume breaks). Where a Qty 10 tier was not exposed in search, `Check cart` is shown and the Qty 1 unit price is held conservatively in the `*` subtotal.

Manufacturer PDFs linked below are local copies, not distributed in Git; see the [datasheet note](../../docs/datasheet/README.md) for obtaining them.

---

## Control Card Assembly BOM (1 Control Card + 1 Mounted Display)

### 1. Active ICs, Power, Protection & Isolation

| Function | Manufacturer | Part / SKU | Package | Qty | Supplier Link | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Ext. @ Qty 10 | Availability / Notes (Checked 2026-11-22) | Local Datasheet |
| :--- | :--- | :--- | :--- | ---: | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| **MCU** | STMicroelectronics | `STM32G474QET6` | LQFP128 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/stmicroelectronics/STM32G474QET6/10326778) | $12.97 | $10.13 | $12.97 | $10.13 | Check live tray stock; T&R cut-tape `STM32G474QET6TR` is $10.06 @ Qty 10. | [stm32g474qe.pdf](../../docs/datasheet/stm32g474qe.pdf) |
| **LCD and Keypad HAT** | Waveshare | `1.3inch LCD HAT` (`14972`) | Module (RPi HAT) | 1 | [Waveshare](https://www.waveshare.com/product/raspberry-pi/displays/lcd-oled/1.3inch-lcd-hat.htm) | $13.99 | Check cart | $13.99 | $13.99* | Direct from Waveshare (no matching DigiKey SKU verified). | [Manual](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Manual.pdf); [Schematic](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Schematic.pdf) |
| **24 V → 5.0 V Buck (`60 V` Rated)** | Texas Instruments | `LM2576HVT-5.0/NOPB` | TO-220-5 (TH) | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=LM2576HVT-5.0%2FNOPB) | $6.89 | Check cart | $6.89 | $6.89* | **In stock (~460–480)** (`LM2576HVT-5.0/NOPB-ND`). `60 V` op / `63 V` abs max, `3 A`, fixed `5.0 V`. Replaces `40 V` `LM2675`. | [TI_LM2576HV.pdf](../../docs/datasheet/TI_LM2576HV.pdf) |
| **Buck Inductor (`330 µH`, Shielded)** | Bourns Inc. | `SRR1280-331K` | SMD `12.5×12.5×8 mm` | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=SRR1280-331K) | $1.16 | Check cart | $1.16 | $1.16* | **In stock (thousands)**. `I_rms = 1.1 A`, `I_sat = 1.4 A` (> `1.13 A` pk @ `1 A` load). Replaces non-DigiKey `MSS1583-334KED`. | — |
| **Schottky Diode (`60 V, 3 A`; Rev Pol + Catch)** | Diodes Incorporated | `B360A-13-F` | DO-214AC (SMA) | 2 | [DigiKey](https://www.digikey.com/en/products/detail/diodes-incorporated/B360A-13-F/724950) | $0.57 | $0.33 | $1.14 | $0.66 | **In stock**. `60 V` rating clears `SMBJ30A` `48.4 V` max clamp by `11.6 V`. | [Diodes_B360A.pdf](../../docs/datasheet/Diodes_B360A.pdf) |
| **5 V → 3.3 V LDO (`1 A`)** | Texas Instruments | `TLV76133DCYR` | SOT-223-4 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/texas-instruments/TLV76133DCYR/18716472) | $0.55 | $0.39 | $0.55 | $0.39 | **In stock**. Hand-solderable SOT-223, stable with `1 µF` ceramic. | [TI_TLV761.pdf](../../docs/datasheet/TI_TLV761.pdf) |
| **24 V Input PTC Fuse (`60 V, 500 mA`)** | Bourns Inc. | `MF-R050` | Radial TH | 1 | [DigiKey](https://www.digikey.com/en/products/detail/bourns-inc/MF-R050/259965) | $0.47 | $0.36 | $0.47 | $0.36 | **In stock**. | [Bourns_MF-R050.pdf](../../docs/datasheet/Bourns_MF-R050.pdf) |
| **24 V Input TVS Diode (`30 V_RWM`, `48.4 V` Clamp)** | Bourns / Littelfuse | `SMBJ30A` | DO-214AA (SMB) | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=SMBJ30A) | $0.45 | $0.28 | $0.45 | $0.28 | **In stock** ($0.26–$0.31 @ Qty 10). Clamps `≤ 48.4 V`, safely under `60 V` `LM2576HV` & `B360A`. Replaces `SMBJ33A` (`53.3 V`). | — |
| **Isolated CAN Transceiver (`5 kVrms`)** | Texas Instruments | `ISO1042DWVR` | 8-SOIC Wide | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=ISO1042DWVR) | $5.42 | $4.14 | $5.42 | $4.14 | Check live stock (`ISO1042DWV` / `ISO1042DWVR` / `ISO1042BDWVR` $5.03 / $3.83; verify in-stock suffix in cart). | [TI_ISO1042.pdf](../../docs/datasheet/TI_ISO1042.pdf) |
| **Regulated Isolated `5 V → 5 V` CAN Supply (`5 kVrms`)** | Analog Devices | `ADuM6020-5BRIZ` | 16-SOIC Wide (`1.27 mm`) | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=ADUM6020-5BRIZ) | $11.14 | $10.56 | $11.14 | $10.56 | **In stock (~147+ tubes + reels)** (`505-ADUM6020-5BRIZ-ND`). `5 kVrms` reinforced, `8.3 mm` creepage, **regulated `5.0 V ±5%` down to `0 mA`**, `100 mA` out. Replaces held `MEJ1S0505SC` & out-of-stock `UCC33421`/`UCC12050`. | — |
| **CAN Bus ESD/TVS Protection** | onsemi | `NUP2105LT1G` | SOT-23-3 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/onsemi/NUP2105LT1G/1749081) | $0.44 | $0.27 | $0.44 | $0.27 | **In stock**. | [onsemi_NUP2105L.pdf](../../docs/datasheet/onsemi_NUP2105L.pdf) |
| **24.000 MHz HSE Crystal (`8 pF, 30 Ω`)** | ECS Inc. | `ECS-240-8-30B-CKM` | 4-SMD (`5.0×3.2 mm`) | 1 | [DigiKey](https://www.digikey.com/en/products/detail/ecs-inc/ECS-240-8-30B-CKM/2781847) | $0.98 | Check cart | $0.98 | $0.98* | **In stock**. | [ECS_ECX-53B_24MHz.pdf](../../docs/datasheet/ECS_ECX-53B_24MHz.pdf) |
| **32.768 kHz LSE Crystal (`6 pF, 50 kΩ`)** | ECS Inc. | `ECS-.327-6-17X-TR` | 4-SOJ (`8.0×3.8 mm`) | 1 | [DigiKey](https://www.digikey.com/en/products/detail/ecs-inc/ECS-327-6-17X-TR/827413) | $0.65 | Check cart | $0.65 | $0.65* | **In stock** ($0.40 @ full reel). | [ECS_ECX-306X_32kHz.pdf](../../docs/datasheet/ECS_ECX-306X_32kHz.pdf) |
| **Section 1 Subtotal** | | | | | | | | **$56.25** | **$50.46*** | *Budget `3 kVrms` stocked CAN-supply alt: `ADuM5020-5BRWZ` ($8.27 / $7.84, thousands in stock).* | |

### 2. Connectors, Switches, LEDs & Mechanical Hardware

| Function | Manufacturer | Part / SKU | Package | Qty | Supplier Link | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Ext. @ Qty 10 | Availability / Notes | Local Datasheet |
| :--- | :--- | :--- | :--- | ---: | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| **CAN Bus Screw Terminal (3-Pos, `5.08 mm`)** | Phoenix Contact | `1729131` | TH Terminal | 1 | [DigiKey](https://www.digikey.com/en/products/detail/phoenix-contact/1729131/260616) | $1.44 | Check cart | $1.44 | $1.44* | Active, stocked. | [Phoenix_1729131_MKDSN_3pos.pdf](../../docs/datasheet/Phoenix_1729131_MKDSN_3pos.pdf) |
| **Host Header `J1`, `J2` (`2×20` Keyed Male, `2.54 mm`)** | Sullins | `SBH11-PBPC-D20-ST-BK` | TH `2×20` Box | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SBH11-PBPC-D20-ST-BK/1990068) | $0.66 | Check cart | $1.32 | $1.32* | Active, stocked. | [Sullins_SBH11_Box_Header.pdf](../../docs/datasheet/Sullins_SBH11_Box_Header.pdf) |
| **LCD HAT Header `J4` (`2×20` Male, `2.54 mm`)** | Sullins | `PRPC020DAAN-RC` | TH `2×20` Pin | 1 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC020DAAN-RC/2775274) | $0.95 | Check cart | $0.95 | $0.95* | Active, stocked. | [Sullins_PRPC020DAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC020DAAN_Header.pdf) |
| **Expansion Header `J3` (`2×12` Male, `2.54 mm`)** | Sullins | `PRPC012DAAN-RC` | TH `2×12` Pin | 1 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC012DAAN-RC/2775282) | $0.45 | Check cart | $0.45 | $0.45* | Active, stocked. | [Sullins_PRPC020DAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC020DAAN_Header.pdf) |
| **SWD Header `J5` (`1×6` Male, `2.54 mm`)** | Sullins | `PRPC006SAAN-RC` | TH `1×6` Pin | 1 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC006SAAN-RC/2775248) | $0.12 | Check cart | $0.12 | $0.12* | Active, stocked. | [Sullins_PRPC006SAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC006SAAN_Header.pdf) |
| **Jumper Headers (`J_TERM`, `BOOT0`, `1×2`)** | Sullins | `PRPC002SAAN-RC` | TH `1×2` Pin | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC002SAAN-RC/2775252) | $0.10 | Check cart | $0.20 | $0.20* | Active, stocked. | [Sullins_PRPC006SAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC006SAAN_Header.pdf) |
| **Jumper Shunts (`2.54 mm`)** | Sullins | `SPC02SYAN` | 2-Pos Shunt | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SPC02SYAN/76375) | $0.10 | Check cart | $0.20 | $0.20* | Active, stocked. | — |
| **Reset Tactile Switch (`SPST-NO`, `6×6 mm`)** | Omron / Aratas | `B3F-1000` | TH `6×6 mm` | 1 | [DigiKey](https://www.digikey.com/en/products/detail/aratas-formerly-omron-components/B3F-1000/33150) | $0.35 | Check cart | $0.35 | $0.35* | Active, stocked. | — |
| **Indicator LEDs (Power + Status, Green)** | Lite-On | `LTST-C170GKT` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/liteon/LTST-C170GKT/269226) | $0.17 | Check cart | $0.34 | $0.34* | Active, stocked. | [LiteOn_LTST-C170GKT_LED.pdf](../../docs/datasheet/LiteOn_LTST-C170GKT_LED.pdf) |
| **LCD HAT Standoffs (`M2.5×0.45`, `11 mm` F-F)** | Würth Elektronik | `970110151` | M2.5 Hex | 4 | [DigiKey](https://www.digikey.com/en/products/detail/w%C3%BCrth-elektronik/970110151/6174769) | $0.42 | Check cart | $1.68 | $1.68* | Active, stocked. | — |
| **LCD HAT Screws (`M2.5×0.45`, `6 mm` Nylon)** | Essentra | `50M025045P006` | `M2.5×6 mm` | 8 | [DigiKey](https://www.digikey.com/en/products/detail/essentra-components/50M025045P006/11638590) | $0.14 | Check cart | $1.12 | $1.12* | Active, stocked. | — |
| **Section 2 Subtotal** | | | | | | | | **$8.17** | **$8.17*** | *Qty-10 cart breaks typically drop connectors/hardware ~15–25%.* | |

### 3. Passives (`0805` SMD & Electrolytic Bulk)

| Function | Manufacturer | Part / SKU | Package | Qty | Supplier Link | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Ext. @ Qty 10 | Notes | Local Datasheet |
| :--- | :--- | :--- | :--- | ---: | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| **Ferrite Bead (`VDDA` filter, `600 Ω @ 100 MHz`)** | Murata | `BLM21AG601SN1D` | 0805 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/murata-electronics/BLM21AG601SN1D/584251) | $0.26 | Check cart | $0.26 | $0.26* | Active, stocked. | [Murata_BLM21AG601SN1D.pdf](../../docs/datasheet/Murata_BLM21AG601SN1D.pdf) |
| **Cap Electrolytic (`100 µF, 63 V`, `LM2576HV` IN)** | Panasonic | `EEU-FC1J101` | Radial TH | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=EEU-FC1J101) | $0.77 | Check cart | $0.77 | $0.77* | `63 V > 48.4 V` `SMBJ30A` clamp; per TI `LM2576HV` datasheet. | — |
| **Cap Electrolytic (`1000 µF, 25 V`, `LM2576HV` OUT)** | Panasonic | `EEU-FC1E102` | Radial TH | 1 | [DigiKey](https://www.digikey.com/en/products/result?keywords=EEU-FC1E102) | $1.49 | $0.96 | $1.49 | $0.96 | Low-ESR output bulk sized for `52 kHz` `LM2576HV` stability. | — |
| **Cap Ceramic (`100 nF, 50 V` X7R — decoupling, `ADuM6020`)** | Yageo | `CC0805KRX7R9BB104` | 0805 | 23 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KRX7R9BB104/302874) | $0.18 | Check cart | $4.14 | $4.14* | Matches 83-part schematic; ordering `100+` strips cuts unit price ~75%. | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| **Cap Ceramic (`1 µF, 50 V` X7R — `VDDA`, LDO IN/OUT)** | Yageo | `CC0805KKX7R9BB105` | 0805 | 3 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KKX7R9BB105/5195292) | $0.30 | Check cart | $0.90 | $0.90* | Matches schematic (`C12`, `C36`, `C37`). | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| **Cap Ceramic (`4.7 µF, 16 V` X7R — `VDD` bulk)** | Yageo | `CC0805KKX7R7BB475` | 0805 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KKX7R7BB475/5195289) | $0.53 | Check cart | $0.53 | $0.53* | Matches schematic (`C9`). | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| **Cap Ceramic (`10 µF, 16 V` X7R — `ADuM6020` `VDDP`/`VISO`)** | Yageo | `CC0805KKX7R7BB106` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/result?keywords=CC0805KKX7R7BB106) | Check cart | Check cart | Check cart | Check cart | Matches schematic (`C24`, `C25`) alongside `100 nF` HF bypass. | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| **Cap Ceramic (`10 nF, 50 V` X7R — `VDDA`)** | Yageo | `CC0805KRX7R9BB103` | 0805 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KRX7R9BB103/302870) | $0.14 | Check cart | $0.14 | $0.14* | (`LM2576HV` needs no bootstrap cap.) | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| **Cap Ceramic (`4.7 nF, 100 V` X7R — CAN split term.)** | Yageo | `CC0805KRX7R0BB472` | 0805 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KRX7R0BB472/5884194) | $0.15 | Check cart | $0.15 | $0.15* | Active, stocked. | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| **Cap Ceramic (`10 pF, 50 V` NP0 — 24 MHz HSE load)** | Yageo | `CC0805JRNPO9BN100` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805JRNPO9BN100/302833) | $0.15 | Check cart | $0.30 | $0.30* | Active, stocked. | — |
| **Cap Ceramic (`6.8 pF, 50 V` NP0 — 32.768 kHz LSE load)** | Yageo | `CC0805CRNPO9BN6R8` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805CRNPO9BN6R8/5195238) | $0.16 | Check cart | $0.32 | $0.32* | Active, stocked. | — |
| **Resistor (`10 kΩ, 1%` — pull-ups / pull-downs)** | Yageo | `RC0805FR-0710KL` | 0805 | 15 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0710KL/727535) | $0.10 | Check cart | $1.50 | $1.50* | Matches schematic (`R1`, `R4–R14`, `R19–R21`); `100+` strips cut unit price ~80%. | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| **Resistor (`60.4 Ω, 1%` — CAN split termination)** | Yageo | `RC0805FR-0760R4L` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0760R4L/728073) | $0.10 | Check cart | $0.20 | $0.20* | Active, stocked. | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| **Resistor (`33 Ω, 1%` — SPI1 damping)** | Yageo | `RC0805FR-0733RL` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0733RL/727862) | $0.10 | Check cart | $0.20 | $0.20* | Active, stocked. | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| **Resistor (`1 kΩ, 1%` — LED current limit)** | Yageo | `RC0805FR-071KL` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-071KL/727444) | $0.10 | Check cart | $0.20 | $0.20* | Matches schematic (`LED1` + `LED2`). | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| **Section 3 Verified Subtotal (excl. 2× `10 µF`)** | | | | | | | | **$11.10** | **$10.57*** | *Buying `0805` passives in 10–100 strips cuts this section to ~$4–$5/card.* | |

---

## Per-Card Cost Rollup (1 Control Card + 1 Mounted Display)

| Section | Verified @ Qty 1 | Verified @ Qty 10 Tier* | Notes |
| :--- | ---: | ---: | :--- |
| **1. Active ICs, Power, Protection & Isolation** | $56.25 | $50.46* | Includes Waveshare `1.3"` LCD HAT ($13.99); excludes bare 4-layer PCB |
| **2. Connectors, Switches, LEDs & Hardware** | $8.17 | $8.17* | Qty-10 cart breaks reduce further |
| **3. Passives (`0805` SMD & Electrolytics)** | $11.10 | $10.57* | Plus `2× CC0805KKX7R7BB106` (~$0.60); 10-card strip breaks cut ~$5–$7 |
| **Total Verified Per Control Card** | **$75.52** | **$69.20*** | **Realistic 10-card build cost (with passive/connector strips):** **~$59–$63 / card** (+ ~$5–$8 bare 4-layer PCB fab) |

*\*Conservative ceiling holding Qty 1 pricing on rows marked `Check cart`.*

---

## Notes on Retired / Alternative Parts (Not Included in Total)

- **Retired `LM2675M-5.0/NOPB` + `SMBJ33A`:** Removed because `SMBJ33A` clamps at `53.3 V`, exceeding `LM2675`'s `40 V` operating ceiling. Replaced by stocked **`LM2576HVT-5.0/NOPB`** (`60 V` op / `63 V` abs max) + **`SMBJ30A`** (`48.4 V` max clamp) + **`SRR1280-331K`** (`330 µH`) + `63 V` `EEU-FC1J101` input bulk cap.
- **Retired `MEJ1S0505SC`:** Removed because unregulated minimum-load requirements (`20 mA`) cause overvoltage at `ISO1042` idle current (`2.8–4.1 mA`).
- **Out-of-Stock TI Isolated DC/DC Alternatives:** Both `UCC33421DHAR` (non-stock on DigiKey, 1,000-pc reel only) and `UCC12050DVER` (out of stock / backorder on DigiKey, $6.89 Qty 1) were checked on 2026-11-22. Replaced as baseline by in-stock **`Analog Devices ADuM6020-5BRIZ`** (`5 kVrms` reinforced, `8.3 mm` creepage, easy-to-hand-solder `1.27 mm` `16-SOIC` wide body, regulated `5.0 V ±5%` down to `0 mA`). *(If a `3 kVrms` isolation barrier is acceptable on non-HV modules, in-stock **`ADuM5020-5BRWZ`** saves ~$2.80/card at $8.27 Qty 1 / $7.84 Qty 10.)*
- **Non-DigiKey Inductor `MSS1583-334KED`:** Replaced with stocked DigiKey **`Bourns SRR1280-331K`** ($1.16 vs. ~$3.75 at Mouser).

---

## Host Power-Board Mating Connector Reference (Required per Host Board)

| Function | Manufacturer | Part / SKU | Package | Qty per Host | Supplier Link | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Local Datasheet |
| :--- | :--- | :--- | :--- | ---: | :--- | ---: | ---: | ---: | :--- |
| **Mating Socket for `J1`, `J2` (`2×20` Keyed Female, `2.54 mm`)** | Sullins | `SFH11-PBPC-D20-ST-BK` | TH `2×20` Socket | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SFH11-PBPC-D20-ST-BK/1990093) | $1.73 | Check cart | $3.46 | [Sullins_SFH11_Female_Header.pdf](../../docs/datasheet/Sullins_SFH11_Female_Header.pdf) |

Prices are snapshots checked 2026-11-22, not quotes; verify live stock, quantity breaks, tariffs, currency, shipping, and taxes in cart before purchasing. The local MCU datasheet is DS12288 Rev 6; check ST's current documentation and errata before finalizing the design.
