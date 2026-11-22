# 10 kW 2-Phase FB-LLC Boost Module — Bill of Materials (DigiKey Preferred)

Status: Rev 0.2 Verified-MPN & Priced Draft BOM (2026-11-22) — **Provisional engineering BOM; verify real-time DigiKey stock, exact quantity breaks, and tariffs/shipping in cart before purchasing.** Matches the `45.0 kHz` 100% SOT-227 + M4 Copper Busbar + `TDK U93/76/30` baseline ([`Boost_Converter_Architecture.md`](./Boost_Converter_Architecture.md)).

*Repository BOM Standard:* Every BOM prioritizes **DigiKey** (`digikey.com`) active, stocked manufacturer part numbers (MPNs), explicit DigiKey SKUs/links, dated availability notes, and **both Qty 1 and Qty 10 USD pricing** (`Ext. @ Qty 1` = `Qty × Qty 1 Price`; `Ext. @ Qty 10` = `Qty × Qty 10 Price`, showing per-module cost at volume tier breaks). Where a specific Qty 10 break was not exposed in live search, `Check cart` is explicitly marked rather than estimated.

---

## 1. Power Semiconductors, Contactors, Fusing & Cooling (Zero PCB High Power)

| Function | Manufacturer | MPN | DigiKey SKU / Link | Package | Qty | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Ext. @ Qty 10 | Availability / Notes (Checked 2026-11-22) |
| :--- | :--- | :--- | :--- | :--- | ---: | ---: | ---: | ---: | ---: | :--- |
| **Primary MOSFETs (`Q1–Q8`) — Preferred** | IXYS / Littelfuse | `IXFN300N20X3` | `IXFN300N20X3-ND` ([DigiKey](https://www.digikey.com/en/products/result?keywords=IXFN300N20X3)) | SOT-227B | 8 | $54.98 | $41.64 | $439.84 | $333.12 | **In stock (~606)**. `200 V`, `300 A`, `~3.5 mΩ`, Kelvin Source. **TIP: Buying 10 pcs ($416.39) is $23.45 cheaper than buying 8 pcs ($439.84) — 2 free spares!** |
| *Primary MOSFET Alt (High Stock / Lower Cost)* | IXYS / Littelfuse | `IXFN230N20T` | `238-IXFN230N20T-ND` ([DigiKey](https://www.digikey.com/en/products/result?keywords=IXFN230N20T)) | SOT-227B | *(Alt 8)* | *$45.39* | *$33.99* | *$363.12* | *$271.92* | **In stock (~1,786)**. `200 V`, `220 A`, `7.5 mΩ`. Again, 10 pcs ($339.90) is cheaper than 8 pcs ($363.12). |
| **Secondary Dual SiC Diodes (`D1–D8`)** | SemiQ | `GHXS100B065S-D3` | [DigiKey](https://www.digikey.com/en/products/result?keywords=GHXS100B065S-D3) | SOT-227-4 | 4 | $40.08 | $29.77 | $160.32 | $119.08 | Active (`650 V`, dual `100 A` SiC Schottky). *Silicon FRED fallback:* Microchip `APT2X101D60J` ($28.99 @ Qty 1). |
| **Main `96 V` DC Contactor (`Q_ISOL`)** | TE / Kilovac | `EV200AAANA` | `A101161-ND` ([DigiKey](https://www.digikey.com/en/products/result?keywords=EV200AAANA)) | Chassis | 1 | $176.03 | $156.74 | $176.03 | $156.74 | **In stock**. `500 A`, `9–36 VDC` economizer coil. *(Check `EV200HAANA` if integral `1 Form A` mirror aux contact is desired.)* |
| **Pre-Charge Contactor (`Q_PRE`)** | TE / Kilovac | `LEV100A5ANG` | `A101164-ND` ([DigiKey](https://www.digikey.com/en/products/result?keywords=LEV100A5ANG)) | Chassis | 1 | $141.14 | $125.68 | $141.14 | $125.68 | **In stock**. `100 A`, `900 VDC`, `24 VDC` coil. *(A compact `30 A` DC relay can save `~$100` here since pre-charge peak is only `~4.4 A`.)* |
| **Forced-Air Cooling Fans (`120 mm`, `24 VDC`)** | Delta Electronics | `AFB1224VH` | `603-1351-ND` ([DigiKey](https://www.digikey.com/en/products/result?keywords=AFB1224VH)) | `120×25 mm` | 2 | $20.62 | $16.62 | $41.24 | $33.24 | **In stock**. `24 VDC`, high static pressure (`~82 CFM` ea) for extruded heatsink. |
| **`96 V` Input Class-T Fuse (`200 A`)** | Bussmann / Littelfuse | `JJN-200` / `JLLN200` | [DigiKey](https://www.digikey.com/en/products/result?keywords=JJN-200) | Class-T | 1 | *$204.34 (DK)* | *Check alt* | *(Buy ext.)* | *(Buy ext.)* | **COST CATCH:** Non-stock & overpriced on DigiKey ($204). Source `Blue Sea 5117` / `Littelfuse JLLN200` + holder at solar/marine distributors (`~$35–$45`). |
| **`±200 V` Output Semiconductor Fuses (`HV+`, `HV-`)** | Eaton / Bussmann | `FWP-50A14FA` | [DigiKey](https://www.digikey.com/en/products/result?keywords=FWP-50A14FA) | `14×51 mm` | 2 | Check cart | Check cart | Check cart | Check cart | Active, stocked on DigiKey (`50 A`, `700 VAC / 800 VDC` aR) + `14×51` fuse holders. *(Avoid Mersen `A70P50-4` — non-DigiKey & `~$150` ea.)* **Never fuse `HV_MID`.** |
| **Section 1 Verified Subtotal (excl. 3 fuses)** | | | | | | | | **$958.57** | **$767.86** | *(Or **$935.12** @ Qty 1 by ordering `10×` `IXFN300N20X3`!)* |

---

## 2. Magnetics & High-Current Busbar Capacitors

| Function | Manufacturer | MPN | DigiKey SKU / Link | Package | Qty | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Ext. @ Qty 10 | Availability / Notes (Checked 2026-11-22) |
| :--- | :--- | :--- | :--- | :--- | ---: | ---: | ---: | ---: | ---: | :--- |
| **Main Transformer Cores (`T1`, `T2`)** | TDK / Epcos | `B67345B0001X087` | [DigiKey](https://www.digikey.com/en/products/result?keywords=B67345B0001X087) | `U93/76/30` (`N87`) | 4 | $28.43 | Check cart | $113.72 | $113.72* | **4 halves = 2 closed U-U pairs.** *(Do NOT order `B67345B0002X087`, which is the `I`-bar!)* Alt: Ferroxcube `U93/76/30-3C90` ($48.01 ea). |
| **External `L_r` Cores (`L_r1`, `L_r2`)** | TDK / Epcos | `B66335G0000X187` | [DigiKey](https://www.digikey.com/en/products/result?keywords=B66335G0000X187) | `E55/28/21` (`N87`) | 4 | $5.44 | $3.70 | $21.76 | $14.82 | **In stock**. **4 halves = 2 E-E pairs** (`A_e = 353 mm²`, `3 T` `< 160 mT` @ `140 A` pk). |
| **Local `96 V` Input Electrolytics** | Nichicon | `LNT2C103MSE` | [DigiKey](https://www.digikey.com/en/products/result?keywords=LNT2C103MSE) | Screw Can | 2 | $40.70 | Check cart | $81.40 | $81.40* | `10,000 µF`, `160 VDC`, screw terminals (replaces non-stock `CDE 500X103M160CD2B`). Bolts onto `96 V+ / 96 V-` bars. |
| **Pass-Through Tank CT Cores (`1:200`, `150 A` pk)** | TDK / Epcos | `B64290L0048X087` | [DigiKey (`3910123`)](https://www.digikey.com/en/products/detail/tdk-corporation/B64290L0048X087/3910123) | Toroid `35.5 mm` OD | 2 | $6.52 | $4.43 | $13.04 | $8.86 | **In stock**. Large ID slips over `15 mm²` `26 AWG` primary bundle; hand-wind `200 T` of `32 AWG`. |
| **Resonant Film Caps (`C_r1`, `C_r2`, `6.0 µF`/ph)** | Vishay | `MKP386M515200YT4` | `BC2794-ND` ([DigiKey](https://www.digikey.com/en/products/detail/vishay-beyschlag-draloric-bc-components/MKP386M515200YT4/11624128)) | Screw-Mount Box | 8 | Check cart | Check cart | Check cart | Check cart | Active (`1.5 µF ±5%`, `2000 VDC / 700 VAC`, heavy screw lugs, `4×`/phase). Verify live cart price (`~$10–$15` ea est.). |
| **Per-Leg DC-Link Snubbers (`Q1–Q8`)** | Vishay | `MKP386M522100JT3` | [DigiKey](https://www.digikey.com/en/products/result?keywords=MKP386M522100JT3) | Screw-Mount Box | 4 | Check cart | Check cart | Check cart | Check cart | `2.2 µF`, `1000 VDC` screw MKP. *(Check live stock; `1.25 kV` variant `MKP386M522125JT3` is `~$20.92` @ Qty 1.)* |
| **Section 2 Verified Subtotal (excl. 12× MKP386)** | | | | | | | | **$229.92** | **$218.80*** | *\*Uses Qty 1 where Qty 10 break requires live cart check.* |

---

## 3. 2-Layer Signal & Gate-Drive Host PCB (`< 1 A` Active ICs & Sensors)

| Function | Manufacturer | MPN | DigiKey SKU / Link | Package | Qty | Qty 1 (USD) | Qty 10 (USD) | Ext. @ Qty 1 | Ext. @ Qty 10 | Availability / Notes (Checked 2026-11-22) |
| :--- | :--- | :--- | :--- | :--- | ---: | ---: | ---: | ---: | ---: | :--- |
| **Isolated Dual Gate Drivers (`U_GD1–4`)** | Texas Instruments | `UCC21520DWR` | [DigiKey](https://www.digikey.com/en/products/result?keywords=UCC21520DWR) | 16-SOIC Wide | 4 | $4.76 | $3.62 | $19.04 | $14.48 | **In stock**. `5.7 kVrms`, `4 A / 6 A`, HW dead-time. Replaces non-active tube suffix `UCC21520DW`. |
| **Isolated `24 V → 15 V` `2 W` Gate Bias Bricks** | Murata Power | `NXJ2S2415MC-R7` | [DigiKey](https://www.digikey.com/en/products/result?keywords=NXJ2S2415MC-R7) | SMD | 8 | $6.32 | $6.32 | $50.56 | $50.56 | **In stock** (next break Reel 135 = $5.63). **Promoted over Recom `RI-2415S` ($10.34 ea — saves `~$32`/module).** |
| **Isolated Voltage Amps (`V_96`, `V_HV+`, `V_HV-`)** | Texas Instruments | `AMC1311BDWVR` | [DigiKey](https://www.digikey.com/en/products/result?keywords=AMC1311BDWVR) | 8-SOIC Wide | 3 | $5.90 | $4.52 | $17.70 | $13.56 | **In stock**. `5 kVrms` reinforced, `0–2.0 V` linear input. |
| **Isolated `5 V → 5 V` `1 W` `AMC1311` Bias Bricks** | Recom Power | `R1SX-0505/H-R` | [DigiKey](https://www.digikey.com/en/products/result?keywords=R1SX-0505%2FH-R) | SMD | 3 | $5.38 | $5.10 | $16.14 | $15.29 | **In stock**. `3 kVDC` high-isolation (`3×` cheaper than `R1S-0505-R` @ $8.60). |
| **Diff-to-SE Op-Amps (`G = 1.50`)** | Texas Instruments | `OPA2376AIDR` | [DigiKey](https://www.digikey.com/en/products/result?keywords=OPA2376AIDR) | SOIC-8 | 2 | $2.76 | $2.06 | $5.52 | $4.12 | **In stock**. Dual precision RRIO for `AMC1311` `V_96`/`V_HV+`/`V_HV-` differential outputs. |
| **CT Precision Rectifier Op-Amp** | Texas Instruments | `TLV271IDBVT` | [DigiKey](https://www.digikey.com/en/products/result?keywords=TLV271IDBVT) | SOT-23-5 | 1 | $1.23 | $0.89 | $1.23 | $0.89 | **In stock** (use `-T` cut-tape suffix; `-R` often backordered). |
| **Fast OC / OV Window Comparators** | Texas Instruments | `TLV3502AIDR` | [DigiKey](https://www.digikey.com/en/products/result?keywords=TLV3502AIDR) | SOIC-8 | 2 | $5.15 | $3.93 | $10.30 | $7.86 | **In stock**. `4.5 ns` dual comparator for `±2.15 V` (`130 A` pk) tank OC & `±218 V` HV OV. |
| **Hardware Fault SR Latch (`< 300 ns`)** | Texas Instruments | `SN74LVC1G74DCUR` | [DigiKey](https://www.digikey.com/en/products/result?keywords=SN74LVC1G74DCUR) | 8-VSSOP | 1 | $0.63 | $0.45 | $0.63 | $0.45 | **In stock**. Drives `HRTIM1_FLT1` (`J2:19`) + `UCC21520 DIS` directly in hardware. |
| **Busbar Hall Sensor (`I_96`, `150 A`)** | LEM USA | `HO 150-S` | [DigiKey](https://www.digikey.com/en/products/result?keywords=HO+150-S) | Panel Pass-Thru | 1 | $20.12 | $17.64 | $20.12 | $17.64 | **In stock** (`5 V` supply; add `10 kΩ / 15 kΩ` `0.1%` divider to `3.3 V` ADC). Avoids special-order `/SP33` lead time! |
| **Busbar Hall Sensor (`I_400`, `50 A`)** | LEM USA | `HO 50-S` | [DigiKey](https://www.digikey.com/en/products/result?keywords=HO+50-S) | Panel Pass-Thru | 1 | $19.99 | Check cart | $19.99 | $19.99* | **In stock** (`5 V` supply + `10 kΩ / 15 kΩ` divider). Standard stocked alternative to special-order `HO 50-S/SP33`. |
| **Contactor Low-Side FETs (`Q_PRE`, `Q_ISOL`, `NC`)** | onsemi | `NVD5117PLT4G` | [DigiKey](https://www.digikey.com/en/products/result?keywords=NVD5117PLT4G) | DPAK | 3 | $4.15 | $2.72 | $12.45 | $8.16 | **In stock**. Matches project hardware standard (+ `3× MUR120` flyback diodes). |
| **Control Card Mating Sockets (`J1`, `J2`)** | Sullins | `SFH11-PBPC-D20-ST-BK` | `S9200-ND` ([DigiKey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/SFH11-PBPC-D20-ST-BK/1990093)) | 2×20 Female | 2 | $1.73 | Check cart | $3.46 | $3.46* | **In stock**. Matches [`STM32_Control_Card_BOM.md`](../STM32_control_card/STM32_Control_Card_BOM.md). |
| **E-Stop Optocoupler & Aux TVS** | Vishay / Littelfuse | `SFH615A-3` + `SMBJ28A` | [DigiKey](https://www.digikey.com/en/products/result?keywords=SFH615A-3) | DIP-4 / SMB | 1 + 1 | $0.83 + $0.35 | Check cart | $1.18 | $1.18* | **In stock** (`SMBJ28A` `~$0.20–$0.50` across vendors). |
| **Section 3 Verified Subtotal (excl. jellybean R/C/D)** | | | | | | | | **$178.32** | **$157.64*** | *Add `~$25–$35` for 1206 HV divider resistors, Zeners, passives & 2-layer PCB fab.* |

---

## 4. Per-Module Cost Rollup (10 kW Boost Module)

| Cost Category | Verified @ Qty 1 | Verified @ Qty 10 Tier | Notes |
| :--- | ---: | ---: | :--- |
| **Section 1** — Power Silicon, Contactors & Fans (excl. 3 fuses) | $958.57 | $767.86 | **Single-module hack:** Ordering `10×` `IXFN300N20X3` drops Sec. 1 to **$935.12** (with 2 free spares). `IXFN230N20T` alt saves another `$76`. |
| **Section 2** — Ferrite Cores, Local `96 V` Caps & CT Cores (excl. 12× MKP386) | $229.92 | $218.80* | Check live DigiKey cart for `8× MKP386M515200YT4` + `4× MKP386M522100JT3` (`~$160–$200` est.). |
| **Section 3** — Signal & Gate-Drive PCB Active ICs & Sensors | $178.32 | $157.64* | Plus `~$30` passives/PCB fab + `1×` common STM32 Control Card (`~$35–$45` est.). |
| **Total Verified DigiKey Active/Core Parts (Per 10 kW Module)** | **$1,366.81** | **$1,144.30*** | **Excludes:** `12×` MKP386 film caps (`~$180` est.), `3×` fuses (`~$80` est.), raw copper/heatsink/wire (`~$150–$200` est.). |
| **Realistic All-In Estimated Build Cost (Per 10 kW Module)** | **~$1,750–$1,850** | **~$1,500–$1,600** | `~$0.15–$0.18 / W` for a 100% SOT-227 + busbar isolated `10 kW` (`15 kW` pk) converter. |

---

## 5. Centralized Bulk Capacitor Bank Reference (System-Wide — Major Cost Catch!)

> [!WARNING]
> **$2,200+ Bulk Cap Cost Catch (2026-11-22):** Original screw-can candidate `CDE CGH103T250X4L` (`10,000 µF / 250 V`) is **$255.82 EACH** on DigiKey (`12×` = **$3,070**!). Switching to stocked `2,200 µF / 250 V` snap-in cans on a simple busbar carrier board drops the entire `±200 V` `60 mF/rail` bank to **~$866** — saving over **$2,200** while lowering total ESR and improving cooling!

| System-Wide Bank | Manufacturer | MPN | DigiKey Link | Qty (System) | Qty 1 (USD) | Qty 10 (USD) | Est. System Total | Availability / Notes (Checked 2026-11-22) |
| :--- | :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- |
| **`±200 V` Split Bank (`61.6 mF`/rail) — Recommended** | Nichicon | `LLG2E222MELC50` | [DigiKey](https://www.digikey.com/en/products/result?keywords=LLG2E222MELC50) | 56 (`28`/rail) | $15.47 | Check cart | **~$866** *(less w/ 50-pc break)* | **In stock (~177+)**. `2,200 µF`, `250 VDC`, snap-in (`35×50 mm`). Replaces `$255.82` ea `CGH103T250X4L`! |
| **`96 VDC` Zero-Dump Bank (`330 mF`)** | Nichicon | `LNT2C103MSE` | [DigiKey](https://www.digikey.com/en/products/result?keywords=LNT2C103MSE) | 33 | $40.70 | Check cart | **~$1,343** *(less w/ 25-pc break)* | `10,000 µF`, `160 VDC`, screw-terminal can (or `68×` stocked `4,700 µF / 160 V` snap-ins to cut cost further). |
| **Fail-Safe Discharge Resistors (`±200 V` `100 Ω` ×2)** | TE / CGS | `HSC250100RJ` | [DigiKey](https://www.digikey.com/en/products/result?keywords=HSC250100RJ) | 2 | $87.32 | Check cart | **$174.64** | `250 W`, `100 Ω ±5%` aluminum chassis-mount (`< 9 s` to `< 48 VDC`, `0 W` running). *(Note: `2× 200 Ω 100 W` in parallel often costs `~$25–$35`/rail instead of `$87`!)* |

---

## 6. Non-DigiKey Mechanical & Winding Raw Materials
- **Copper Busbar Stock (`C110`):** `1/8" × 1.00"` (`96 V` laminated bars), `1/8" × 3/4"` (`SW`/`C_r` straps), `1/8" × 1/2"` (`±200 V` output bars) — McMaster-Carr / OnlineMetals (`~$55–$80`/module).
- **Magnet Wire:** `2 lb` spool **`26 AWG` `MW35-C` `200 °C`** heavy-build enameled copper wire — Remington Industries / Temco (`~$35–$45`).
- **Insulation & Clamping:** `0.5 mm` (`20 mil`) Nomex 410 sheet, `1.5 mm` / `3.0 mm` / `6.0 mm` `G10/FR4` sheet, `2 mil` Kapton tape (`25 mm` & `50 mm`), `4 kV` PTFE/silicone sleeving, stainless M4/M6 hardware & Belleville lock washers — McMaster-Carr (`~$30–$45`).
- **Heatsink:** `~250 × 300 × 45 mm` extruded aluminum heatsink (`≤ 0.13 °C/W` forced-air) — HeatsinkUSA / Wakefield-Vette (`~$65–$90`).
