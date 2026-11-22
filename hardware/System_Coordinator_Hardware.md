# System Coordinator & Display Node Hardware Baseline

## Module Scope & Reference Platform
- **Hardware Platform:** **Waveshare ESP32-S3-Touch-LCD-7** (SKU `27078` / V1.2 hardware revision).
- **Operational Role:** Central supervisory controller, CANbus telemetry aggregator, local touchscreen HMI, and Wi-Fi web server for remote system monitoring, configuration, and software circuit-breaker resets.
- **System Boundary Rule:** Executes high-level energy dispatch, state orchestration, fault logging, and user configuration. It never participates in sub-millisecond power-stage switching loops or direct cell-level BMS polling.

---

## Core Compute & Memory (`ESP32-S3-WROOM-1-N16R8`)
- **Processor:** Xtensa 32-bit LX7 dual-core microprocessor running up to **240 MHz**.
- **Memory Architecture:**
  - **Internal SRAM / ROM:** `512 KB` SRAM, `384 KB` ROM.
  - **External Flash:** `16 MB` Quad SPI Flash (or `8 MB` on `-N8R8` variant).
  - **External PSRAM:** `8 MB` Octal SPI PSRAM (required for double-buffered LVGL RGB565 framebuffers).
- **Wireless Radio:** Integrated `2.4 GHz` Wi-Fi (`802.11 b/g/n`) and Bluetooth 5 (LE) with onboard PCB antenna (or IPEX external antenna on `-7-U` variant).

---

## Display & Capacitive Touch Subsystem
- **LCD Panel:** `7.0-inch` IPS TFT display, **800 × 480** resolution, `65K` colors (`RGB565`), `170°` viewing angle, `345 cd/m²` typical brightness.
- **LCD Driver IC:** **Sitronix ST7262** parallel `16-bit` RGB interface (`~21 MHz` typical `PCLK`).
- **Backlight Driver:** **AP3032** step-up LED backlight boost converter enabled via `CH422G EXIO2`.
- **Touch Controller:** **Goodix GT911** 5-point capacitive touch controller over `I2C` (`0x5D` / `0x14`).

### Complete Display & Touch GPIO Map

| Signal Group | Pin / Expander | Function | Notes |
| :--- | :--- | :--- | :--- |
| **RGB Red (`R3–R7`)** | `GPIO1`, `GPIO2`, `GPIO42`, `GPIO41`, `GPIO40` | Parallel Red Data (`5-bit`) | `R3` = LSB, `R7` = MSB |
| **RGB Green (`G2–G7`)** | `GPIO39`, `GPIO0`, `GPIO45`, `GPIO48`, `GPIO47`, `GPIO21` | Parallel Green Data (`6-bit`) | `G2` = LSB, `G7` = MSB |
| **RGB Blue (`B3–B7`)** | `GPIO14`, `GPIO38`, `GPIO18`, `GPIO17`, `GPIO10` | Parallel Blue Data (`5-bit`) | `B3` = LSB, `B7` = MSB |
| **RGB Sync & Clock** | `GPIO46` (`HSYNC`), `GPIO3` (`VSYNC`), `GPIO5` (`DE`), `GPIO7` (`PCLK`) | Panel Timing Signals | Driven by ESP32-S3 LCD_CAM peripheral |
| **Touch Interface** | `GPIO8` (`SDA`), `GPIO9` (`SCL`), `GPIO4` (`TP_IRQ`) | `I2C` Touch Data & Interrupt | Shared `I2C` bus with `CH422G` |
| **IO Expander (`CH422G`)** | `EXIO1` (`TP_RST`), `EXIO2` (`BL_EN`), `EXIO3` (`LCD_RST`), `EXIO6` (`LCD_VDD_EN`) | Reset & Backlight Control | Controlled over `I2C` (`GPIO8`/`GPIO9`) |

---

## Coordinator Field Bus (`CANbus` / `TWAI`)
- **Controller & Signal Routing:**
  - ESP32-S3 integrated **TWAI** (Two-Wire Automotive Interface) controller mapped to **`GPIO20` (`CANTX`)** and **`GPIO19` (`CANRX`)**.
  - Onboard **NXP TJA1051T/3** high-speed CAN transceiver routed to the `PH2.0` 2-pin header (`CANH`, `CANL`).
- **Hardware Multiplexing Requirement (`CH422G EXIO5`):**
  - `GPIO19` and `GPIO20` are shared between the native USB data lines (`D-`/`D+`) and the `TJA1051T/3` CAN transceiver via an onboard **FSUSB42UMX** analog switch.
  - **Firmware Mandate:** At system boot, the Coordinator must configure **`CH422G EXIO5` (`CAN_SEL`) = HIGH (`1`)** to connect `GPIO19/20` to the CAN interface.
- **External Galvanic CAN Isolator & Bus-Side Power:**
  - An **external galvanic CAN isolator** sits between the Waveshare board's CAN interface and the main microgrid CANbus trunk to preserve strict ground isolation across all modules.
  - **Bus-Side Transceiver Power:** The isolated bus-side rail of the CAN isolator is powered by a dedicated isolated `5.0 V` DC-DC supply derived from the un-interruptible **`AUX_LOGIC_24V`** rail (Split Diode-OR'd `24 VDC` auxiliary bus; see [`../docs/System_Safety_and_Protection_Architecture.md`](../docs/System_Safety_and_Protection_Architecture.md)).
  - **Termination:** `120Ω` termination resistor placed at the trunk end of the isolated CAN bus.
- **Application Layer Protocol:** The working CAN identifiers, payloads, and timing are documented in [`protocol/Microgrid_Communications_Specifications.md`](../protocol/Microgrid_Communications_Specifications.md) and [`protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md). Boost 0 has been reopened under the working registry and 2-phase FB-LLC profile; Solar gateway payloads remain open. See [`docs/Software_Design_Constraints.md`](../docs/Software_Design_Constraints.md) and [`docs/Coordinator_CAN_Implementation.md`](../docs/Coordinator_CAN_Implementation.md) for current implementation scope.

---

## Auxiliary Peripherals, Storage & Power Supply

### Onboard Connectors & Peripherals
- **MicroSD (TF) Card Slot:** `SPI` mode (`GPIO11` = `MOSI`, `GPIO12` = `SCK`, `GPIO13` = `MISO`, with `SD_CS` driven by **`CH422G EXIO4`**) used for persistent configuration profiles, node address maps, and timestamped fault event logging.
- **USB & UART Ports:**
  - Dual USB Type-C ports: one native USB (`GPIO19`/`GPIO20` when `EXIO5 = LOW`) and one dedicated **WCH CH343P** USB-to-UART bridge (`GPIO43` `TXD0` / `GPIO44` `RXD0`) for firmware flashing and serial debug console.
- **Secondary Headers (Unused / Spare):**
  - Onboard `SP3485EN` RS485 transceiver (`GPIO15` `RXD`, `GPIO16` `TXD`), `PH2.0` `I2C` header (`GPIO8`/`GPIO9` with `3.3V`/`5V` level selector), and `PH2.0` Sensor header (`GPIO6` ADC-capable input).

### Power Supply & Black-Start Architecture
- **Board Power Input:** `5.0V DC` (`~450 mA` typical draw at full backlight brightness) supplied via USB-C or regulated `5V` rail, stepped down internally via an **SGM2212-3.3** LDO to `3.3V` logic power.
- **Backup Cell Support:** Onboard **CS8501** single-cell `3.7V` LiPo charger/boost IC (`PH2.0` header, `580 mA` charge rate, `≤2000 mAh` cell recommended) for graceful shutdown or brownout ride-through.
- **Common Auxiliary Power Source (`AUX_LOGIC_24V`):**
  - Both the Coordinator's main `5.0 V DC` (`≥ 1.5 A`) logic/display supply (stepped down from `24.0 VDC` via a local `1 A`-fused/eFused buck converter) and the isolated bus-side rail of the CAN isolator derive their power from the un-interruptible **`AUX_LOGIC_24V`** rail of the Split Diode-OR'd `24 VDC` auxiliary bus, ensuring the Coordinator and CAN bus remain active during a full system blackout (`Black Start`) and during Hardware E-Stop events to log and display faults.
