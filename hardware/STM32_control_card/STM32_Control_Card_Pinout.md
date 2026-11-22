# STM32 Control Card Master Pinout and Peripheral Mapping

Status: Draft master pin allocation (v0.1); **not independently verified or approved for fabrication**. A uniqueness check of listed pin numbers does not verify pin names, alternate functions, connector mapping, or electrical behavior. Check every assignment against [stm32g474qe.pdf](../../docs/datasheet/stm32g474qe.pdf) (DS12288 Rev 6, Tables 12 and 13) and the [Waveshare 1.3-inch LCD HAT schematic](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Schematic.pdf). Bench bring-up, ADC sampling schedules, and HRTIM timing are not yet validated.

## Architecture Overview

- **MCU:** STMicroelectronics `STM32G474QET6` (`LQFP128`, 128 pins: 18 supply/ground/reference pins, 2 dedicated reset/boot/oscillator pins shared with GPIO, 108 GPIO/peripheral I/O total).
- **Card-Fixed Pins (Not routed to Host `J1`/`J2`):**
  - System power, analog reference, reset, `BOOT0`, 24 MHz HSE, 32.768 kHz LSE, and card status LED
  - `SWD` programming header (`J5`, 1×6, 2.54 mm)
  - Isolated `FDCAN1` (`PD0`/`PD1` → `ISO1042DWV`)
  - Directly mounted Waveshare 1.3-inch LCD HAT (`J4`, 2×20, 2.54 mm)
- **Host Board Interface (Standard 2.54 mm / 0.100" Pitch Headers for Hand Soldering):**
  - **`J1` (2×20 Keyed Box Header, 40 pins):** Host 24 V power input, control/analog grounds, 16 ADC channels (spanning `ADC1–ADC5`), DALY BMS `USART2`/RS485, Fan PWM/Tach (`TIM4`), 3 contactor outputs, 3 contactor feedbacks, and auxiliary `UART4`/`SPI3` port.
  - **`J2` (2×20 Keyed Box Header, 40 pins):** 8 interleaved `GND_CTRL` return pins, all 12 `HRTIM1` outputs (`CHA1–CHF2`), 2 hardware fault inputs (`FLT1`, `FLT3`), Inverter parallel sync (`SCOUT`, `SCIN`), shared gate-driver enable, engine/auxiliary `TIM15`/`TIM5` channels, and spare GPIO.
  - **`J3` (2×12 Pin Header, 24 pins):** Brings out all 19 remaining unallocated MCU I/O pins + 5 `GND_CTRL` pins so zero usable MCU pins are stranded.

---

## 1. Power, Clocks, Reset, Boot, and Card-Fixed Functions

| LQFP128 Pin(s) | STM32 Pin Name | Assigned Function | Peripheral / AF | Hardware Notes (DS12288 Rev 6) |
| --- | --- | --- | --- | --- |
| 13, 31, 47, 64, 79, 95, 111, 128 | `VDD` | `3V3_DIG` | Supply | 1× `100 nF` close to each pin + 1× `4.7 µF` bulk (Figure 16). |
| 12, 30, 46, 63, 78, 94, 110, 127 | `VSS` | `GND_CTRL` | Ground | Solid ground plane return under LQFP128. |
| 45 | `VDDA` | `3V3_ANA` | Analog Supply | Fed from `3V3_DIG` via ferrite bead / `10 Ω` + `10 nF` + `1 µF`. |
| 42 | `VSSA` | `GND_CTRL` (Analog star) | Analog Ground | Tied to `GND_CTRL` at single clean analog reference point. |
| 43, 44 | `VREF+` | `3V3_ANA` (`VREF+`) | ADC/DAC Ref | Tied to `VDDA` + `100 nF` + `1 µF`. Disable internal `VREFBUF` in firmware. |
| 6 | `VBAT` | `3V3_DIG` | Backup Supply | Tied to `VDD` + `100 nF` (or coin cell if backup RTC required). |
| 21 | `PG10-NRST` | `NRST` | System Reset | Tactile reset switch + `100 nF` to `GND_CTRL` + routed to `J5` SWD pin 5. |
| 123 | `PB8-BOOT0` | `BOOT0` | Boot Mode | `10 kΩ` pull-down to `GND_CTRL` + 2-pin jumper/pad to `3V3_DIG` for ROM bootloader recovery. |
| 19 | `PF0-OSC_IN` | `HSE_IN` | `OSC_IN` | `24.000 MHz` crystal (`ECS-240-8-30B-CKM`, `8 pF` load, `30 Ω` ESR) + `10 pF` NP0 caps. |
| 20 | `PF1-OSC_OUT` | `HSE_OUT` | `OSC_OUT` | `24.000 MHz` crystal output. |
| 8 | `PC14-OSC32_IN` | `LSE_IN` | `OSC32_IN` | `32.768 kHz` crystal (`ECS-.327-6-17X-TR`, `6 pF` load) + `6.8 pF` NP0 caps. |
| 9 | `PC15-OSC32_OUT` | `LSE_OUT` | `OSC32_OUT` | `32.768 kHz` crystal output. |
| 96 | `PA13` | `SWDIO` | `SWDIO-JTMS` (AF0) | Routed to `J5` SWD pin 4 (`100 Ω` series optional). |
| 98 | `PA14` | `SWCLK` | `SWCLK-JTCK` (AF0) | Routed to `J5` SWD pin 2 (`100 Ω` series optional). |
| 118 | `PB3` | `SWO` | `JTDO-TRACESWO` (AF0) | Routed to `J5` SWD pin 6 (`100 Ω` series optional). |
| 108 | `PD0` | `CAN1_RX` | `FDCAN1_RX` (AF9) | To `ISO1042DWV` `RXD` (zero ADC/HRTIM conflict). |
| 109 | `PD1` | `CAN1_TX` | `FDCAN1_TX` (AF9) | To `ISO1042DWV` `TXD` (with `10 kΩ` pull-up to `3V3_DIG` to keep recessive during reset). |
| 5 | `PE6` | `CARD_LED` | GPIO Output | Onboard status LED (`1 kΩ` series to `GND_CTRL`, ~1.3 mA). |

### Waveshare 1.3-inch LCD HAT Mapping (`J4`, 2×20 2.54 mm Male Header)

All 8 keypad/joystick switches on the [Waveshare 1.3-inch LCD HAT schematic](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Schematic.pdf) switch directly to `GND` with no pull-ups on the HAT. Add `10 kΩ` pull-ups to `3V3_DIG` plus `100 nF` debounce capacitors to `GND_CTRL` on the control card. Leave RPi 5 V pins (`J4` pins 2 and 4) **unconnected (NC)** on the control card.

| Signal | HAT Pin (`J4`) | RPi BCM | LQFP128 Pin | STM32 Pin | Peripheral / Mode | Card Conditioning |
| --- | ---: | --- | ---: | --- | --- | --- |
| `3.3 V` | 1, 17 | `3V3` | — | — | `3V3_DIG` | Decoupled with `10 µF` + `100 nF` at `J4` |
| `GND` | 6, 9, 14, 20, 25, 30, 34, 39 | `GND` | — | — | `GND_CTRL` | Tied to `GND_CTRL` plane |
| `LCD_SCK` | 23 | `BCM11` (`SCLK`) | 84 | `PG2` | `SPI1_SCK` (AF5) | `33 Ω` series damping resistor |
| `LCD_MOSI` | 19 | `BCM10` (`MOSI`) | 86 | `PG4` | `SPI1_MOSI` (AF5) | `33 Ω` series damping resistor |
| `LCD_CS` | 24 | `BCM8` (`CE0`) | 103 | `PG5` | GPIO Output | `10 kΩ` pull-up to `3V3_DIG` (deselected on reset) |
| `LCD_DC` | 22 | `BCM25` | 85 | `PG3` | GPIO Output | Direct 3.3 V CMOS |
| `LCD_RST` | 13 | `BCM27` | 104 | `PG6` | GPIO Output | `10 kΩ` pull-down or pull-up per ST7789 init |
| `LCD_BL` | 18 | `BCM24` | 49 | `PF12` | `TIM20_CH1` (AF2) / GPIO | HAT has `10 kΩ` pull-down (`Q1` S8050); defaults OFF |
| `KEY1` | 40 | `BCM21` | 50 | `PF13` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `KEY2` | 38 | `BCM20` | 51 | `PF14` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `KEY3` | 36 | `BCM16` | 52 | `PF15` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `JOY_UP` | 31 | `BCM6` | 105 | `PG7` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `JOY_DOWN` | 35 | `BCM19` | 106 | `PG8` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `JOY_LEFT` | 29 | `BCM5` | 107 | `PG9` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `JOY_RIGHT` | 37 | `BCM26` | 82 | `PG0` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |
| `JOY_PRESS` | 33 | `BCM13` | 83 | `PG1` | GPIO Input | `10 kΩ` pull-up + `100 nF` RC debounce |

All remaining `J4` pins (2, 3, 4, 5, 7, 8, 10, 11, 12, 15, 16, 21, 26, 27, 28, 32) are **NC** on the control card (and unused on the HAT schematic).

---

## 2. Host Connector `J1` — Power, Analog Inputs, Contactors, Fan & BMS

Connector: **`SBH11-PBPC-D20-ST-BK`** (2×20, 2.54 mm keyed shrouded male box header on card; mates with **`SFH11-PBPC-D20-ST-BK`** on host). Placed on the **Analog / Power edge** of the card, physically separated from `J2` PWM switching lines.

| J1 Pin | Signal Name | LQFP128 Pin | STM32 Pin | Primary Function / ADC | J1 Pin | Signal Name | LQFP128 Pin | STM32 Pin | Primary Function / ADC |
| ---: | --- | ---: | --- | --- | ---: | --- | ---: | --- | --- |
| **1** | `+24V_IN` | — | — | Isolated 24 VDC from host | **2** | `+24V_IN` | — | — | Isolated 24 VDC from host |
| **3** | `GND_CTRL` | — | — | 24 V return / control GND | **4** | `GND_CTRL` | — | — | 24 V return / control GND |
| **5** | `3V3_REF` | — | — | Fused/limited 3.3 V logic ref | **6** | `VSSA_AGND` | 42 | `VSSA` | Quiet analog return ref |
| **7** | `GND_CTRL` | — | — | Control GND | **8** | `GND_CTRL` | — | — | Control GND |
| **9** | `AIN_01` | 27 | `PA0` | `ADC12_IN1` | **10** | `AIN_02` | 28 | `PA1` | `ADC12_IN2` |
| **11** | `AIN_03` | 29 | `PA2` | `ADC1_IN3` | **12** | `AIN_04` | 32 | `PA3` | `ADC1_IN4` |
| **13** | `AIN_05` | 33 | `PA4` | `ADC2_IN17` | **14** | `AIN_06` | 34 | `PA5` | `ADC2_IN13` |
| **15** | `AIN_07` | 35 | `PA6` | `ADC2_IN3` | **16** | `AIN_08` | 36 | `PA7` | `ADC2_IN4` |
| **17** | `AIN_09` | 22 | `PC0` | `ADC12_IN6` | **18** | `AIN_10` | 23 | `PC1` | `ADC12_IN7` |
| **19** | `AIN_11` | 24 | `PC2` | `ADC12_IN8` | **20** | `AIN_12` | 25 | `PC3` | `ADC12_IN9` |
| **21** | `AIN_13` | 39 | `PB0` | `ADC3_IN12` / `ADC1_IN15` | **22** | `AIN_14` | 60 | `PE14` | `ADC4_IN1` |
| **23** | `AIN_15` | 70 | `PD8` | `ADC4_IN12` / `ADC5_IN12` | **24** | `AIN_16` | 71 | `PD9` | `ADC4_IN13` / `ADC5_IN13` |
| **25** | `GND_CTRL` | — | — | Analog/Digital guard GND | **26** | `GND_CTRL` | — | — | Analog/Digital guard GND |
| **27** | `BMS_TX` | 115 | `PD5` | `USART2_TX` (AF7) | **28** | `BMS_RX` | 116 | `PD6` | `USART2_RX` (AF7) |
| **29** | `BMS_DE` | 114 | `PD4` | `USART2_RTS_DE` (AF7) | **30** | `FAN_PWM` | 74 | `PD12` | `TIM4_CH1` (AF2) |
| **31** | `FAN_TACH` | 75 | `PD13` | `TIM4_CH2` (AF2) | **32** | `CONT_CMD1` | 112 | `PD2` | Contactor / Disconnect Cmd 1 |
| **33** | `CONT_CMD2` | 117 | `PD7` | Contactor / Disconnect Cmd 2 | **34** | `CONT_CMD3` | 76 | `PD14` | Contactor / Disconnect Cmd 3 |
| **35** | `CONT_FB1` | 2 | `PE3` | Contactor Feedback 1 | **36** | `CONT_FB2` | 3 | `PE4` | Contactor Feedback 2 |
| **37** | `CONT_FB3` | 4 | `PE5` | Contactor Feedback 3 | **38** | `HOST_UART4_TX` | 100 | `PC10` | `UART4_TX` (AF5) / `SPI3_SCK` |
| **39** | `HOST_UART4_RX` | 101 | `PC11` | `UART4_RX` (AF5) / `SPI3_MISO` | **40** | `HOST_SPI3_MOSI` | 102 | `PC12` | `SPI3_MOSI` (AF6) / `UART5_TX` |

*Note on simultaneous ADC sampling:* `AIN_01..16` intentionally include pins across **all five ADCs** (`PA0..PA3` on `ADC1`, `PA4..PA7` on `ADC2`, `PB0` on `ADC3`, `PE14` on `ADC4`, and `PD8/PD9` on `ADC5`) so 2-phase FB-LLC split-rail/tank channels, DAB bridge currents, or 3-phase generator currents can be sampled simultaneously in hardware via HRTIM ADC triggers.

---

## 3. Host Connector `J2` — HRTIM PWM, Faults, Sync, Gate Enable & Engine I/O

Connector: **`SBH11-PBPC-D20-ST-BK`** (2×20, 2.54 mm keyed shrouded male box header on card; mates with **`SFH11-PBPC-D20-ST-BK`** on host). Placed on the **High-Speed Switching edge** of the card.

| J2 Pin | Signal Name | LQFP128 Pin | STM32 Pin | Primary Function / Timer | J2 Pin | Signal Name | LQFP128 Pin | STM32 Pin | Primary Function / Timer |
| ---: | --- | ---: | --- | --- | ---: | --- | ---: | --- | --- |
| **1** | `PWM_A1` | 89 | `PA8` | `HRTIM1_CHA1` (AF13) | **2** | `PWM_A2` | 90 | `PA9` | `HRTIM1_CHA2` (AF13) |
| **3** | `GND_CTRL` | — | — | Return for CHA/CHB | **4** | `GND_CTRL` | — | — | Return for CHA/CHB |
| **5** | `PWM_B1` | 91 | `PA10` | `HRTIM1_CHB1` (AF13) | **6** | `PWM_B2` | 92 | `PA11` | `HRTIM1_CHB2` (AF13) |
| **7** | `PWM_C1` | 66 | `PB12` | `HRTIM1_CHC1` (AF13) | **8** | `PWM_C2` | 67 | `PB13` | `HRTIM1_CHC2` (AF13) |
| **9** | `GND_CTRL` | — | — | Return for CHC/CHD | **10** | `GND_CTRL` | — | — | Return for CHC/CHD |
| **11** | `PWM_D1` | 68 | `PB14` | `HRTIM1_CHD1` (AF13) | **12** | `PWM_D2` | 69 | `PB15` | `HRTIM1_CHD2` (AF13) |
| **13** | `PWM_E1` | 87 | `PC8` | `HRTIM1_CHE1` (AF3) | **14** | `PWM_E2` | 88 | `PC9` | `HRTIM1_CHE2` (AF3) |
| **15** | `GND_CTRL` | — | — | Return for CHE/CHF | **16** | `GND_CTRL` | — | — | Return for CHE/CHF |
| **17** | `PWM_F1_FLD` | 80 | `PC6` | `HRTIM1_CHF1` (AF13) / `TIM3_CH1` | **18** | `PWM_F2` | 81 | `PC7` | `HRTIM1_CHF2` (AF13) / `TIM3_CH2` |
| **19** | `FLT1_TRIP` | 93 | `PA12` | `HRTIM1_FLT1` (AF13) — Common Trip | **20** | `FLT3_AUX` | 62 | `PB10` | `HRTIM1_FLT3` (AF13) — Aux Trip |
| **21** | `SYNC_OUT` | 40 | `PB1` | `HRTIM1_SCOUT` (AF13) | **22** | `SYNC_IN` | 121 | `PB6` | `HRTIM1_SCIN` (AF12) |
| **23** | `GND_CTRL` | — | — | Return for Fault/Sync | **24** | `GND_CTRL` | — | — | Return for Fault/Sync |
| **25** | `GATE_EN` | 77 | `PD15` | Shared Gate-Driver Enable (`10 kΩ` PD) | **26** | `ENG_PWM1` | 17 | `PF9` | `TIM15_CH1` (AF3) — Throttle/Choke |
| **27** | `ENG_PWM2` | 18 | `PF10` | `TIM15_CH2` (AF3) — Aux PWM | **28** | `ENG_RPM_CAP` | 15 | `PF7` | `TIM5_CH2` (AF6) — RPM / Crank Cap |
| **29** | `ENG_CAP2` | 16 | `PF8` | `TIM5_CH3` (AF6) — Aux Cap | **30** | `ENG_IGN_EN` | 26 | `PF2` | GPIO / `TIM20_CH3` — Ignition/Fuel |
| **31** | `ENG_STRT_EN` | 10 | `PF3` | GPIO / `I2C3_SCL` — Starter Interlock | **32** | `GPIO_PF4` | 11 | `PF4` | GPIO / `I2C3_SDA` / `COMP1_OUT` |
| **33** | `GPIO_PF5` | 14 | `PF5` | GPIO / `TIM20_CH2N` | **34** | `GPIO_PF6` | 97 | `PF6` | GPIO / `TIM5_CH1` / `I2C2_SCL` |
| **35** | `GPIO_PE0` | 125 | `PE0` | GPIO / `TIM16_CH1` | **36** | `GPIO_PE1` | 126 | `PE1` | GPIO / `TIM17_CH1` |
| **37** | `GPIO_PE2` | 1 | `PE2` | GPIO / `TIM3_CH1` / `SPI4_SCK` | **38** | `GPIO_PB9` | 124 | `PB9` | GPIO / `TIM17_CH1` / `I2C1_SDA` |
| **39** | `GPIO_PA15` | 99 | `PA15` | GPIO / `HRTIM1_FLT2` / `TIM2_CH1` | **40** | `GPIO_PB11` | 65 | `PB11` | GPIO / `HRTIM1_FLT4` / `ADC12_IN14` |

### Expansion Header `J3` (2×12, 2.54 mm Unshrouded Header — Remaining 19 MCU Pins + 5 GND)

Connector: **`PRPC012DAAN-RC`** (`2×12`, `2.54 mm`). Ordered monotonically along the LQFP128 perimeter (`37 → 122`) with `TIM1` complementary channels grouped contiguously on pins `7–14` and `5× GND_CTRL` guard returns so zero usable MCU pins are stranded:

| J3 Pin | Signal Name | LQFP128 Pin | STM32 Pin | Key Alternate Functions | J3 Pin | Signal Name | LQFP128 Pin | STM32 Pin | Key Alternate Functions |
| ---: | --- | ---: | --- | --- | ---: | --- | ---: | --- | --- |
| **1** | `GND_CTRL` | — | — | Analog/Expansion GND | **2** | `GND_CTRL` | — | — | Analog/Expansion GND |
| **3** | `EXP_PC4` | 37 | `PC4` | `ADC2_IN5` / `OPAMP2_VOUT` | **4** | `EXP_PC5` | 38 | `PC5` | `ADC2_IN11` / `OPAMP1_VINM` |
| **5** | `EXP_PB2` | 41 | `PB2` | `ADC2_IN12` / `OPAMP3_VINM` | **6** | `EXP_PF11` | 48 | `PF11` | GPIO / `TIM20_ETR` |
| **7** | `EXP_PE7` | 53 | `PE7` | `ADC3_IN4` / `TIM1_ETR` | **8** | `EXP_PE8` | 54 | `PE8` | `ADC345_IN6` / `TIM1_CH1N` |
| **9** | `EXP_PE9` | 55 | `PE9` | `ADC3_IN2` / `TIM1_CH1` | **10** | `EXP_PE10` | 56 | `PE10` | `ADC345_IN14` / `TIM1_CH2N` |
| **11** | `EXP_PE11` | 57 | `PE11` | `ADC345_IN15` / `TIM1_CH2` | **12** | `EXP_PE12` | 58 | `PE12` | `ADC345_IN16` / `TIM1_CH3N` |
| **13** | `EXP_PE13` | 59 | `PE13` | `ADC345_IN3` / `TIM1_CH3` | **14** | `EXP_PE15` | 61 | `PE15` | `ADC4_IN2` / `TIM1_BKIN` |
| **15** | `GND_CTRL` | — | — | Mid-header guard GND | **16** | `GND_CTRL` | — | — | Mid-header guard GND |
| **17** | `EXP_PD10` | 72 | `PD10` | `ADC345_IN7` / `USART3_CK` | **18** | `EXP_PD11` | 73 | `PD11` | `ADC345_IN8` / `USART3_CTS` |
| **19** | `EXP_PD3` | 113 | `PD3` | `USART2_CTS` / `TIM2_CH1` | **20** | `EXP_PB4` | 119 | `PB4` | `TIM3_CH1` / `TIM16_CH1` *(UCPD `PA10`)* |
| **21** | `EXP_PB5` | 120 | `PB5` | `TIM3_CH2` / `FDCAN2_RX` | **22** | `EXP_PB7` | 122 | `PB7` | `I2C1_SDA` / `USART1_RX` |
| **23** | `EXP_PC13` | 7 | `PC13` | `RTC_TAMP1` / GPIO *(3 mA max)* | **24** | `GND_CTRL` | — | — | End GND return |

---

## 4. Cross-Module Signal Allocation Summary

| Interface Group | Boost (2-Ph FB-LLC) | Inverter (`120 V` / `240 V` Half-Br) | Generator | Battery (Isol. DAB) | Utility-Tie (PFC + Isol. DC-DC) |
| --- | --- | --- | --- | --- | --- |
| **HRTIM PWM (`J2:1..18`)** | `CHA1/2, CHB1/2` (Ph1) + `CHC1/2, CHD1/2` (Ph2) (**8**) | `CHA1/2` (120 V) / `CHA1/2, CHB1/2` (240 V) (**2 / 4**, `CHC1/2` spare) | `CHA1/2..CHC1/2` (3-ph) + `CHD1/2, CHE1/2` (buck-boost) + `CHF1` (field) (**11**) | `CHA1/2, CHB1/2` (48 V) + `CHC1/2, CHD1/2` (96 V) (**8**) | `CHA1, CHB1` (PFC) + `CHC1/2, CHD1/2` (Isol. DC-DC) (**6**) |
| **Fault Trip (`J2:19..20`)** | `FLT1` (Tank OC + HV OV + drv) + `FLT3` (Mag snap-disc / E-Stop) | `FLT1` (drv + OC + GFCI) | `FLT1` (OC + driver) | `FLT1` (`I_tank`/port OC + drv) + `FLT3` (Mag snap-disc) | `FLT1` (PFC/Stg2 OC + 400V OV) + `FLT3` (Mag snap-disc) |
| **Sync (`J2:21..22`)** | Unused | `SCOUT` / `SCIN` (2) | Unused | Unused | Unused |
| **Gate Enable (`J2:25`)** | `GATE_EN` (1) | `GATE_EN` (1) | `GATE_EN` (1) | `GATE_EN` (1) | `GATE_EN` (1) |
| **Analog (`J1:9..24`)** | 9–10 (`V_96`, `V_HV+`, `V_HV-`, `I_96`, `I_400`, `2× I_res`, `T_hs`, `T_mag`, opt `I_mid`) | 6–7 (120 V) / 8–9 (240 V) (`V_HV±`, `V_ac`, `I_ac`, `I_gfci`, `T_hs`) | 13 (`V_24`, `V_96`, `3× V_ph`, `3× I_ph`, `I_bb`, `I_fld`, `T_hs`, `T_cool`, `P_oil`) | 6–7 (`I_tank`, `I_bat`, `I_96`, `V_bat`, `V_96`, `T_hs`, opt `T_mag`) | 9–10 (`V_ac`, `I_ac`, `2× I_pfc`, `V_400`, `V_96`, `I_96`, `I_tank`, `T_hs`, opt `T_mag`) |
| **Contactors (`J1:32..37`)** | 3 cmd + 3 fb (`Q_PRE`, `Q_ISOL`, active bleed / HV disc) | 3 cmd + 2 fb (AC line, 2-pole DC, Precharge) | 2 cmd + 2 fb (Precharge, Main) | 2 cmd + 2 fb (Precharge, Main) | 3 cmd + 3 fb (AC, Precharge, Main) |
| **Fan (`J1:30..31`)** | `FAN_PWM` + `FAN_TACH` | `FAN_PWM` + `FAN_TACH` | `FAN_PWM` + `FAN_TACH` | `FAN_PWM` + `FAN_TACH` | `FAN_PWM` + `FAN_TACH` |
| **BMS / Engine** | — | — | `ENG_PWM1..2`, `ENG_RPM_CAP`, `ENG_IGN_EN`, `ENG_STRT_EN` | `USART2` (`BMS_TX/RX/DE`) | — |

---

## 5. Critical Silicon & Startup Hardware Caveats (Verify in Schematic & Firmware)

1. **UCPD Dead-Battery Pull-Down (`PA9`/`PA10` vs `PB6`/`PB4`, DS12288 Table 12 Footnote 6):**
   - A high voltage on `PA9` (`HRTIM1_CHA2`) activates a `5.1 kΩ` internal pull-down on `PB6` (`HRTIM1_SCIN`), and a high voltage on `PA10` (`HRTIM1_CHB1`) activates a `5.1 kΩ` internal pull-down on `PB4` until firmware sets `PWR_CR3.UCPD1_DBDIS = 1`.
   - Mitigation: Both `PA9` and `PA10` must have host/card pull-downs so gate outputs are LOW at reset, AND firmware must set `UCPD1_DBDIS = 1` during early `SystemInit` before enabling HRTIM outputs or reading `PB6`.
2. **HRTIM Outputs & `GATE_EN` Safe Reset State:**
   - STM32 GPIO pins initialize in analog (high-Z) state during/after reset (DS12288 Section 3.11.5). Every active-high PWM line (`J2:1..18`), `GATE_EN` (`PD15`), and contactor/ignition command (`PD2`, `PD7`, `PD14`, `PF2`, `PF3`) **must** have an external pull-down resistor (`10 kΩ`) on the host board (and/or card) so power stages remain strictly OFF when the MCU is in reset, unpowered, or unplugged.
3. **SWD, SWO & JTAG Pins (`PA13`, `PA14`, `PA15`, `PB3`, `PB4`):**
   - `PA13` (`SWDIO`), `PA14` (`SWCLK`), and `PB3` (`JTDO-TRACESWO`) are routed directly to `J5` (`SWD` + `SWO`). Out of reset, `PA15` (`JTDI`) and `PB4` (`JTRST`) are also JTAG alternate functions with internal pull-ups. Firmware should configure debug mode for **SWD + Asynchronous Trace (`SWO`)** (or 2-wire SWD when trace is unused) to release `PA15` (`J2:39`) and `PB4` (`J3`). Moving `HRTIM1_SCOUT` to `PB1` avoids any JTAG reset contention on the inverter sync output.
4. **Boot0 Pin (`PB8-BOOT0`):**
   - Keep `PB8` dedicated to `BOOT0` with a `10 kΩ` pull-down to `GND_CTRL` so noise never latches the MCU into the ROM bootloader.
