# STM32 Control Card Bill of Materials

Status: Provisional component-level draft BOM (v0.3), **not purchase-ready or complete**. Part suitability, quantities, supplier links, prices, and local datasheets require independent verification; some links may not match the listed part. Bare 4-layer PCB fabrication is not included.

Manufacturer PDFs linked below are local copies, not distributed in Git; see the [datasheet note](../../docs/datasheet/README.md) for obtaining them.

## Control Card Assembly BOM (1 Control Card + 1 Mounted Display)

### Active ICs, Power, Protection & Isolation

| Function | Manufacturer | Part / SKU | Package | Qty | Supplier link | Unit price (USD) | Ext. price (USD) | Price checked | Local Datasheet |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | --- | --- |
| MCU | STMicroelectronics | `STM32G474QET6` | LQFP128 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/stmicroelectronics/STM32G474QET6/10326778) | $12.97 | $12.97 | 2026-11-20 (Qty 1) | [stm32g474qe.pdf](../../docs/datasheet/stm32g474qe.pdf) |
| LCD and keypad HAT | Waveshare | `1.3inch LCD HAT` (`14972`) | Module (RPi HAT) | 1 | [Waveshare](https://www.waveshare.com/product/raspberry-pi/displays/lcd-oled/1.3inch-lcd-hat.htm) | $13.99 | $13.99 | 2026-11-20 (Qty 1) | [Manual](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Manual.pdf); [Schematic](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Schematic.pdf) |
| 24 V -> 5 V Buck Regulator | Texas Instruments | `LM2675M-5.0/NOPB` | 8-SOIC | 1 | [DigiKey](https://www.digikey.com/en/products/detail/texas-instruments/LM2675M-5-0-NOPB/271162) | $6.35 | $6.35 | 2026-11-20 (Qty 1) | [TI_LM2675.pdf](../../docs/datasheet/TI_LM2675.pdf) |
| Buck Inductor (100 uH, 1.7 A) | Bourns | `SRR1260-101M` | SMD Shielded (12x12 mm) | 1 | [DigiKey](https://www.digikey.com/en/products/detail/bourns-inc/SRR1260-101M/1969966) | $1.11 | $1.11 | 2026-11-20 (Qty 1) | [Bourns_SRR1260.pdf](../../docs/datasheet/Bourns_SRR1260.pdf) |
| Schottky Diode (60 V, 3 A; Rev Pol + Catch) | Diodes Incorporated | `B360A-13-F` | DO-214AC (SMA) | 2 | [DigiKey](https://www.digikey.com/en/products/detail/diodes-incorporated/B360A-13-F/724950) | $0.57 | $1.14 | 2026-11-20 (Qty 1) | [Diodes_B360A.pdf](../../docs/datasheet/Diodes_B360A.pdf) |
| 5 V -> 3.3 V LDO (1 A) | Texas Instruments | `TLV76133DCYR` | SOT-223-4 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/texas-instruments/TLV76133DCYR/18716472) | $0.55 | $0.55 | 2026-11-20 (Qty 1) | [TI_TLV761.pdf](../../docs/datasheet/TI_TLV761.pdf) |
| 24 V Input PTC Fuse (60 V, 500 mA) | Bourns | `MF-R050` | Radial TH | 1 | [DigiKey](https://www.digikey.com/en/products/detail/bourns-inc/MF-R050/259965) | $0.47 | $0.47 | 2026-11-20 (Qty 1) | [Bourns_MF-R050.pdf](../../docs/datasheet/Bourns_MF-R050.pdf) |
| 24 V Input TVS Diode (33 Vwm) | Bourns | `SMBJ33A` | DO-214AA (SMB) | 1 | [DigiKey](https://www.digikey.com/en/products/detail/bourns-inc/SMBJ33A/2254101) | $0.53 | $0.53 | 2026-11-20 (Qty 1) | [Bourns_SMBJ33A.pdf](../../docs/datasheet/Bourns_SMBJ33A.pdf) |
| Isolated CAN Transceiver (5 kVrms) | Texas Instruments | `ISO1042DWV` | 8-SOIC Wide | 1 | [DigiKey](https://www.digikey.com/en/products/detail/texas-instruments/ISO1042DWV/9860887) | $6.76 | $6.76 | 2026-11-20 (Qty 1) | [TI_ISO1042.pdf](../../docs/datasheet/TI_ISO1042.pdf) |
| Isolated 5 V-to-5 V DC/DC (candidate on hold; see note below) | Murata Power Solutions | `MEJ1S0505SC` | 7-SIP TH | 1 | [DigiKey](https://www.digikey.com/en/products/detail/murata-power-solutions-inc/MEJ1S0505SC/4037305) | $8.96 | $8.96 | 2026-11-20 (Qty 1) | [Murata_MEJ1S0505SC.pdf](../../docs/datasheet/Murata_MEJ1S0505SC.pdf) |
| CAN Bus ESD/TVS Protection | onsemi | `NUP2105LT1G` | SOT-23-3 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/onsemi/NUP2105LT1G/1749081) | $0.44 | $0.44 | 2026-11-20 (Qty 1) | [onsemi_NUP2105L.pdf](../../docs/datasheet/onsemi_NUP2105L.pdf) |
| 24.000 MHz HSE Crystal (8 pF, 30 Ohm) | ECS Inc. | `ECS-240-8-30B-CKM` | 4-SMD (5.0x3.2 mm) | 1 | [DigiKey](https://www.digikey.com/en/products/detail/ecs-inc/ECS-240-8-30B-CKM/2781847) | $0.98 | $0.98 | 2026-11-20 (Qty 1) | [ECS_ECX-53B_24MHz.pdf](../../docs/datasheet/ECS_ECX-53B_24MHz.pdf) |
| 32.768 kHz LSE Crystal (6 pF, 50 kOhm) | ECS Inc. | `ECS-.327-6-17X-TR` | 4-SOJ (8.0x3.8 mm) | 1 | [DigiKey](https://www.digikey.com/en/products/detail/ecs-inc/ECS-327-6-17X-TR/827413) | $0.61 | $0.61 | 2026-11-20 (Qty 1) | [ECS_ECX-306X_32kHz.pdf](../../docs/datasheet/ECS_ECX-306X_32kHz.pdf) |

### Connectors, Switches, LEDs & Mechanical Hardware

| Function | Manufacturer | Part / SKU | Package | Qty | Supplier link | Unit price (USD) | Ext. price (USD) | Price checked | Local Datasheet |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | --- | --- |
| CAN Bus Screw Terminal (3-Pos, 5.08 mm) | Phoenix Contact | `1729131` | TH Terminal | 1 | [DigiKey](https://www.digikey.com/en/products/detail/phoenix-contact/1729131/260616) | $1.44 | $1.44 | 2026-11-20 (Qty 1) | [Phoenix_1729131_MKDSN_3pos.pdf](../../docs/datasheet/Phoenix_1729131_MKDSN_3pos.pdf) |
| Host Header `J1`, `J2` (2x20 Keyed Male, 2.54 mm) | Sullins Connector Solutions | `SBH11-PBPC-D20-ST-BK` | TH 2x20 Box | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SBH11-PBPC-D20-ST-BK/1990068) | $0.66 | $1.32 | 2026-11-20 (Qty 1) | [Sullins_SBH11_Box_Header.pdf](../../docs/datasheet/Sullins_SBH11_Box_Header.pdf) |
| LCD HAT Header `J4` (2x20 Male, 2.54 mm) | Sullins Connector Solutions | `PRPC020DAAN-RC` | TH 2x20 Pin | 1 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC020DAAN-RC/2775274) | $0.95 | $0.95 | 2026-11-20 (Qty 1) | [Sullins_PRPC020DAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC020DAAN_Header.pdf) |
| Expansion Header `J3` (2x12 Male, 2.54 mm) | Sullins Connector Solutions | `PRPC012DAAN-RC` | TH 2x12 Pin | 1 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC012DAAN-RC/2775282) | $0.45 | $0.45 | 2026-11-20 (Qty 1) | [Sullins_PRPC020DAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC020DAAN_Header.pdf) |
| SWD Header `J5` (1x6 Male, 2.54 mm) | Sullins Connector Solutions | `PRPC006SAAN-RC` | TH 1x6 Pin | 1 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC006SAAN-RC/2775248) | $0.12 | $0.12 | 2026-11-20 (Qty 1) | [Sullins_PRPC006SAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC006SAAN_Header.pdf) |
| Jumper Headers (`J_TERM`, `BOOT0`, 1x2, 2.54 mm) | Sullins Connector Solutions | `PRPC002SAAN-RC` | TH 1x2 Pin | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PRPC002SAAN-RC/2775252) | $0.10 | $0.20 | 2026-11-20 (Qty 1) | [Sullins_PRPC006SAAN_Header.pdf](../../docs/datasheet/Sullins_PRPC006SAAN_Header.pdf) |
| Jumper Shunts (2.54 mm) | Sullins Connector Solutions | `SPC02SYAN` | 2-Pos Shunt | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SPC02SYAN/76375) | $0.10 | $0.20 | 2026-11-20 (Qty 1) | — |
| Reset Tactile Switch (SPST-NO, 6x6 mm) | Omron / Aratas | `B3F-1000` | TH 6x6 mm | 1 | [DigiKey](https://www.digikey.com/en/products/detail/aratas-formerly-omron-components/B3F-1000/33150) | $0.35 | $0.35 | 2026-11-20 (Qty 1) | — |
| Indicator LEDs (Power + Status, Green) | Lite-On | `LTST-C170GKT` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/liteon/LTST-C170GKT/269226) | $0.17 | $0.34 | 2026-11-20 (Qty 1) | [LiteOn_LTST-C170GKT_LED.pdf](../../docs/datasheet/LiteOn_LTST-C170GKT_LED.pdf) |
| LCD HAT Standoffs (M2.5x0.45, 11 mm F-F) | Würth Elektronik | `970110151` | M2.5 Hex | 4 | [DigiKey](https://www.digikey.com/en/products/detail/w%C3%BCrth-elektronik/970110151/6174769) | $0.42 | $1.68 | 2026-11-20 (Qty 1) | — |
| LCD HAT Screws (M2.5x0.45, 6 mm Pan Nylon) | Essentra Components | `50M025045P006` | M2.5x6 mm | 8 | [DigiKey](https://www.digikey.com/en/products/detail/essentra-components/50M025045P006/11638590) | $0.14 | $1.12 | 2026-11-20 (Qty 1) | — |

### Passives (`0805` SMD & Electrolytic Bulk)

| Function | Manufacturer | Part / SKU | Package | Qty | Supplier link | Unit price (USD) | Ext. price (USD) | Price checked | Local Datasheet |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | --- | --- |
| Ferrite Bead (`VDDA` filter, 600 Ohm @ 100 MHz) | Murata Electronics | `BLM21AG601SN1D` | 0805 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/murata-electronics/BLM21AG601SN1D/584251) | $0.26 | $0.26 | 2026-11-20 (Qty 1) | [Murata_BLM21AG601SN1D.pdf](../../docs/datasheet/Murata_BLM21AG601SN1D.pdf) |
| Cap Electrolytic (`22 uF, 50 V`, `LM2675` IN) | Panasonic Industry | `EEU-FR1H220` | Radial TH | 1 | [DigiKey](https://www.digikey.com/en/products/detail/panasonic-industry/EEU-FR1H220/2433586) | $0.39 | $0.39 | 2026-11-20 (Qty 1) | — |
| Cap Electrolytic (`100 uF, 25 V`, `LM2675` OUT) | Panasonic Industry | `EEU-FR1E101` | Radial TH | 1 | [DigiKey](https://www.digikey.com/en/products/detail/panasonic-industry/EEU-FR1E101/2433547) | $0.45 | $0.45 | 2026-11-20 (Qty 1) | — |
| Cap Ceramic (`100 nF, 50 V` X7R — decoupling, debounce, NRST) | Yageo | `CC0805KRX7R9BB104` | 0805 | 24 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KRX7R9BB104/302874) | $0.18 | $4.32 | 2026-11-20 (Qty 1) | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| Cap Ceramic (`1 uF, 50 V` X7R — `VDDA`, `VREF+`, LDO) | Yageo | `CC0805KKX7R9BB105` | 0805 | 4 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KKX7R9BB105/5195292) | $0.30 | $1.20 | 2026-11-20 (Qty 1) | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| Cap Ceramic (`4.7 uF, 16 V` X7R — `VDD`, LCD, `MEJ1`) | Yageo | `CC0805KKX7R7BB475` | 0805 | 4 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KKX7R7BB475/5195289) | $0.53 | $2.12 | 2026-11-20 (Qty 1) | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| Cap Ceramic (`10 nF, 50 V` X7R — `VDDA`, `LM2675` `CB`) | Yageo | `CC0805KRX7R9BB103` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KRX7R9BB103/302870) | $0.14 | $0.28 | 2026-11-20 (Qty 1) | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| Cap Ceramic (`4.7 nF, 100 V` X7R — CAN split term.) | Yageo | `CC0805KRX7R0BB472` | 0805 | 1 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805KRX7R0BB472/5884194) | $0.15 | $0.15 | 2026-11-20 (Qty 1) | [Yageo_CC0805_X7R.pdf](../../docs/datasheet/Yageo_CC0805_X7R.pdf) |
| Cap Ceramic (`10 pF, 50 V` NP0 — 24 MHz HSE load) | Yageo | `CC0805JRNPO9BN100` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805JRNPO9BN100/302833) | $0.15 | $0.30 | 2026-11-20 (Qty 1) | — |
| Cap Ceramic (`6.8 pF, 50 V` NP0 — 32.768 kHz LSE load) | Yageo | `CC0805CRNPO9BN6R8` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/CC0805CRNPO9BN6R8/5195238) | $0.16 | $0.32 | 2026-11-20 (Qty 1) | — |
| Resistor (`10 kOhm, 1%` — pull-ups / pull-downs) | Yageo | `RC0805FR-0710KL` | 0805 | 14 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0710KL/727535) | $0.10 | $1.40 | 2026-11-20 (Qty 1) | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| Resistor (`60.4 Ohm, 1%` — CAN split termination) | Yageo | `RC0805FR-0760R4L` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0760R4L/728073) | $0.10 | $0.20 | 2026-11-20 (Qty 1) | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| Resistor (`33 Ohm, 1%` — SPI1 damping) | Yageo | `RC0805FR-0733RL` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0733RL/727862) | $0.10 | $0.20 | 2026-11-20 (Qty 1) | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| Resistor (`1 kOhm, 1%` — LED current limit) | Yageo | `RC0805FR-071KL` | 0805 | 2 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-071KL/727444) | $0.10 | $0.20 | 2026-11-20 (Qty 1) | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |
| Resistor (`100 Ohm, 1%` — SWD/SWO series damping) | Yageo | `RC0805FR-07100RL` | 0805 | 3 | [DigiKey](https://www.digikey.com/en/products/detail/yageo/RC0805FR-07100RL/727543) | $0.10 | $0.30 | 2026-11-20 (Qty 1) | [Yageo_RC0805.pdf](../../docs/datasheet/Yageo_RC0805.pdf) |

**Total Estimated Control-Card BOM (at Qty-1 DigiKey pricing):** **$75.12 USD** (buying `0805` passives in Qty 10–25 strips typically cuts the passive subtotal by ~70%; bare 4-layer PCB, shipping, and tax excluded).

The total includes the **on-hold** MEJ1 candidate and is not a quote or a validated build cost. Its light-load output can exceed the CAN transceiver supply range; its DC withstand and working-voltage ratings must also be assessed separately. See the [power and isolation review](./STM32_Control_Card_Mechanical_and_Power.md#2-isolated-can-bus-interface).

**Alternatives under evaluation, not included in the total:**

- **Preferred candidate:** TI `UCC33421DHAR`, regulated isolated DC/DC, 16-pin DHA surface-mount package, 0.65 mm pitch. [TI product details](https://www.ti.com/product/UCC33421/part-details/UCC33421DHAR); [DigiKey search](https://www.digikey.com/en/products/result?keywords=UCC33421DHAR) (exact listing/SKU unverified); [local datasheet](../../docs/datasheet/TI_UCC33421.pdf). TI reported out of stock on 2026-11-20; DigiKey availability and price are unverified.
- TI `UCC12050DVER`, regulated isolated DC/DC, 16-pin DVE surface-mount package. [TI product details](https://www.ti.com/product/UCC12050/part-details/UCC12050DVER); [DigiKey search](https://www.digikey.com/en/products/result?keywords=UCC12050DVER) (exact listing/SKU unverified); [local datasheet](../../docs/datasheet/TI_UCC12050.pdf). Price and availability unverified.

Supporting passives and system suitability remain to be established for both alternatives.

For the provisional `UCC33421DHAR` enable pull-up, consider one 22 kOhm, 1%, 0805 resistor, candidate Yageo `RC0805FR-0722KL`: [DigiKey search](https://www.digikey.com/en/products/result?keywords=RC0805FR-0722KL), [series datasheet](../../docs/datasheet/Yageo_RC0805.pdf). Exact listing, price and suitability are unverified and not included in the total.

Provisional UCC33421 decoupling candidates (not included in the total): two Yageo `CC0805KRX7R9BB153` (15 nF, 50 V, X7R, 0805), one on each side, and one input-side `CC0805KKX7R7BB106` (10 uF, 16 V, X7R, 0805); see the [series datasheet](../../docs/datasheet/Yageo_CC0805_X7R.pdf) and [15 nF](https://www.digikey.com/en/products/result?keywords=CC0805KRX7R9BB153) / [10 uF](https://www.digikey.com/en/products/result?keywords=CC0805KKX7R7BB106) DigiKey searches. For the output, one Murata `GRM21BZ71A226ME15L` (22 uF, 10 V, X7R, +/-20%, 0805) is a candidate: [LCSC C907991](https://www.lcsc.com/product-detail/C907991.html), [DigiKey search](https://www.digikey.com/en/products/result?keywords=GRM21BZ71A226ME15L), [Murata reference sheet](../../docs/datasheet/Murata_GRM21BZ71A226ME15.pdf). Availability and prices are unverified. Confirm effective capacitance at bias and temperature, transient response, high-frequency layout and suitability against TI's recommendations. TI's layout example calls for an 0402 high-frequency capacitor alongside an 0805 bulk capacitor; the proposed 0805 15 nF substitution requires verification.

---

## Host Power-Board Mating Connector Reference (Required per Host Board)

| Function | Manufacturer | Part / SKU | Package | Qty per host | Supplier link | Unit price (USD) | Ext. price (USD) | Price checked | Local Datasheet |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | --- | --- |
| Mating Socket for `J1`, `J2` (2x20 Keyed Female, 2.54 mm) | Sullins Connector Solutions | `SFH11-PBPC-D20-ST-BK` | TH 2x20 Socket | 2 | [DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SFH11-PBPC-D20-ST-BK/1990093) | $1.73 | $3.46 | 2026-11-20 (Qty 1) | [Sullins_SFH11_Female_Header.pdf](../../docs/datasheet/Sullins_SFH11_Female_Header.pdf) |

Prices are snapshots, not quotes; verify availability, quantity breaks, tariffs, currency, shipping, and taxes before purchasing. DigiKey is preferred where available; Waveshare is listed for the display because a matching DigiKey SKU is not verified.

The local MCU datasheet is DS12288 Rev 6. Check ST's current documentation and errata before finalizing the design.
