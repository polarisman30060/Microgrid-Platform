# System Architecture Specification

## 1. Executive Summary & Design Philosophy
This document defines the physical, electrical, control, and isolation architecture of the decentralized modular hybrid residential DC microgrid.

The system aggregates power from solar PV arrays, battery storage banks, utility grid inputs, and backup petrol generators onto a common primary DC bus, steps that energy up to a high-voltage DC inverter link, and delivers split-phase 120/240 VAC (60 Hz) power through modular, software-protected output stages.

### Core Architectural Principles
1. **Decentralized Real-Time Autonomy:** High-speed PWM regulation, phase interleaving, and microsecond-level hardware fault protection execute strictly inside each power module's local microcontroller. The system never relies on communication links for cycle-by-cycle stability.
2. **Supervisory Coordination Without Single-Point Failure:** A central Coordinator manages startup sequencing, slow-loop voltage/current trim, load dispatch, and user interfaces. If the Coordinator or CAN bus fails, active power nodes seamlessly transition to autonomous V-I droop regulation.
3. **Galvanic Isolation by Design:** Every power stage maintains strict galvanic isolation between high-power conductors and low-voltage logic/communication domains.
4. **High-Capacitance Bus Damping:** Primary and high-voltage DC links incorporate dedicated bulk electrolytic and film capacitor banks to absorb transient load steps, decouple stage dynamics, and relax control-loop bandwidth requirements.
5. **Uninterruptible Auxiliary Lifeline:** Control electronics, gate drivers, sensors, and contactor coils run from an independent auxiliary power distribution bus so node intelligence remains alive during main bus black-starts or short-circuit trips.

---

## 2. System Topology & Bus Hierarchy
The microgrid is organized into four distinct electrical buses that separate bulk energy transport, high-voltage AC synthesis, auxiliary logic power, and digital telemetry.

```text
  [Solar PV]       [Battery Banks]       [Backup Gen]       [Utility Grid]
      |                   |                   |                   |
  +---+---+           +---+---+           +---+---+           +---+---+
  | Solar |           | Isol. |           |  Gen  |           | Isol. |
  | MPPT  |           |  DAB  |           | Rect/ |           |PFC+LLC|
  | Nodes |           | + BMS |           | Cnvrt |           | Node  |
  +---+---+           +---+---+           +---+---+           +---+---+
      |               ===ISO===               |               ===ISO===
======+===================+===================+===================+======  PRIMARY LV DC BUS
      | (LV_BUS- bonded to PE via single-point DC GFDI)              |     96 VDC Nominal (+/- 5% Reg)
      v                                                          +---+---+ Bulk Electrolytic + Film Bank
   [DC GFDI] -> PE                                               | 2-Ph  | 10 kW Modular FB-LLC Stage(s)
                                                                 |FB-LLC | (90° Interleaved, Center-Tapped Sec)
                                                                 +---+---+
                                                                 ===ISO=== >= 4 kVrms Galvanic Barrier
                                                                     |
=====================================================================+===  HV DC SPLIT INVERTER LINK
     +200 VDC (HV+) / 0V (HV_MID = N = PE) / -200 VDC (HV-)          |     400 VDC Nominal (+/- 200 V Split)
                    |                                   |            |     Symmetrical Split Bulk Cap Bank
                +---+---+                           +---+---+        v
                | 120V  |                           | 240V  |     [PE Bond]
                | 1-Leg |                           | 2-Leg |     Single-Point N-G Bond
                |Half-Br|                           |Half-Br|     Transformerless Current-Mode
                +---+---+                           +---+---+     Inverter / Software Breakers
                    |                                   |
              [120 VAC Loads]                   [120/240 VAC Loads]

-------------------------------------------------------------------------  SPLIT 24 VDC AUXILIARY LIFELINE
  AUX_LOGIC_24V (Diode-OR'd Isolated, Always-On) + AUX_COIL_24V (E-Stop Safety Relay)  Per-Tap eFuse/Fuses
--------------------------------------------------------------------------  MAIN CAN 2.0B BUS (500 kbps)
  Master Coordinator / HMI  <===>  Main-bus Nodes + Solar Gateways         Telemetry, Trim & State Sync
                                      |
                           +----------+----------+
                           | Isolated local CAN  |
                           | Solar children 0-31 |
```

---

The main bus supports the registry-assigned Battery, Boost, Generator, Grid Interface, Inverter, Display, and Solar Gateway instances. Solar scales behind up to 16 gateways, each serving at most 32 local child slots (512 total slots for the 500-node target). Gateways aggregate/filter child traffic upstream; they are not transparent repeaters. Main-bus IDs and Solar-local IDs may overlap numerically because the CAN segments are physically isolated. The gateway's scope, MCU/board evaluation, commercial gateway screening, and failure behavior are described in the [Solar Gateway Node design](./nodes/Solar_Gateway_Node.md); payloads and production hardware remain open.

## 3. Electrical Bus Specifications

| Bus Name | Nominal Rating | Operating Window | Primary Function | Protection & Isolation |
| :--- | :--- | :--- | :--- | :--- |
| **Primary LV DC Energy Bus** | `96.0 VDC` | `91.2 V - 100.8 V` (+/- 5% normal)<br>`84.0 V - 108.0 V` (fault bounds) | Common energy aggregation bus linking solar, batteries, generator, grid charger, and boost inputs. | Negative rail (`LV_BUS-`) single-point bonded to `PE` via DC GFDI; precharge-protected copper busbars, per-module DC contactors, DC fuses, hardware OV/UV comparators, bulk capacitor bank. |
| **High-Voltage (HV) DC Split Link** | `400.0 VDC`<br>(`±200.0 VDC` split) | `380.0 V - 415.0 V` (`±190.0 V` to `±207.5 V` normal)<br>`360.0 V - 435.0 V` (trip bounds) | Galvanically isolated, earth-referenced split DC bus (`HV+` / `HV_MID` / `HV-`) feeding transformerless `120 VAC` and `120/240 VAC` half-bridge inverter stages. | Reinforced transformer isolation (`>= 4 kVrms`) via FB-LLC Boost stages; `HV_MID` bonded to `PE` & AC Neutral; PSM soft-start, passive + active bleeder discharge (`< 48 VDC`), split-rail imbalance monitoring, fast HW OV/OC inhibit. |
| **Auxiliary Power Lifeline** | Split `24.0 VDC` (`22.8–25.2 V`, Diode-OR'd Isolated) | Continuous black-start & `0 ms` multi-battery failover | `AUX_LOGIC_24V` powers Coordinator, STM32 cards, and isolated CAN; `AUX_COIL_24V` (via latched E-Stop/GFDI safety relay) powers all contactor coils, `NC` discharge relays, `ESTOP_OK` (`FLT3`), and Solar Rapid Shutdown. | Isolated `48 V → 24 V` battery supplies + Ideal-Diode OR + per-tap fuses/eFuses (`1–3 A`); independent of main `96 V` / `400 V` bus contactors. |
| **Control & Telemetry Bus** | Main `CAN 2.0B` bus at `500 kbps`, plus isolated Solar segments | Differential `5 V` / `3.3 V` isolated | Carries supervisory commands, module telemetry, fault reports, and HMI status; Solar child traffic is aggregated by gateways. | Galvanically isolated CAN transceivers (`> 2.5 kVrms`; control-card baseline `5 kVrms` `ISO1042DWV`), `120 Ohm` end terminations per physical segment, bounded Bus-Off recovery. |

---

## 4. Functional Subsystem Breakdown

### 4.1 Energy Storage & BMS Subsystem (Battery Nodes)
* **Function:** Anchors primary `96 VDC` bus stability and stores surplus renewable or generator energy.
* **Architecture:** Supports battery banks (such as `48 V` nominal LiFePO4, NMC, Lead-Acid, and Sodium-Ion) interfaced through **galvanically isolated bidirectional Dual Active Bridge (DAB)** DC-DC stages (`1:2` turns ratio; see [`../hardware/Battery_Isolation_Topology_Study.md`](../hardware/Battery_Isolation_Topology_Study.md)) so battery state-of-charge (SoC) voltage swings never disturb the `96 VDC` bus and every battery pack + DALY BMS remains isolated from the earth-referenced `LV_BUS-`.
* **BMS Integration:** Interfaces directly with pack-level BMS telemetry (cell min/max voltages, pack temperatures, charge/discharge enable flags) to enforce smooth current derating before hard BMS cutoff thresholds are reached.
* **Black-Start Role:** Provides auxiliary lifeline support so the microgrid can cold-boot without grid or solar presence.

### 4.2 Renewable & Fuel Generation Subsystems (Solar, Generator, Grid Nodes)
* **Solar MPPT Nodes:** Step down or boost PV string voltages onto the `96 VDC` bus using maximum power point tracking, transitioning automatically to constant-voltage curtailment if the bus approaches its upper regulation limit (`100.8 VDC`). Because `LV_BUS-` is earth-referenced through DC GFDI, PV overcurrent protection resides on the ungrounded `PV+` conductor.
* **Petrol Generator Module:** Rectifies and conditions power from a backup fuel generator onto the `96 VDC` bus with programmable current/load ramping to keep the engine inside its peak fuel-efficiency band and prevent stall on step loads. Negative-grounded `24 V` alternator reference aligns with `LV_BUS-`, while main current travels strictly over heavy copper return busbars.
* **Grid Input / Charger Module:** Provides **galvanically isolated** AC-to-`96 VDC` supplemental charging (two-stage: interleaved active PFC to internal `400 VDC` link, followed by an isolated DC-DC LLC/PSFB stage to `96 VDC`) so Utility AC Neutral (`PE`) never creates a ground loop or short-circuit path with the earth-referenced `96 VDC` bus.

### 4.3 High-Voltage DC-DC Boost Subsystem (Boost Nodes)
* **Function:** Galvanically isolates and steps the `96 VDC` primary energy bus up to the `400 VDC` (`±200 VDC` split-rail) inverter link at **`10 kW` continuous** (`12–15 kW` short-term peak) modular capacity per unit.
* **Topology:** **2-phase (`2 × 5 kW`) interleaved Full-Bridge LLC (FB-LLC) resonant converter** (`f_r = 45.0 kHz`, **`90°` primary phase interleaving** for `180°` output ripple cancellation at `2× f_sw`, `100%` SOT-227 + M4 copper busbar construction, hand-wound `TDK U93/76/30` transformers) with **center-tapped full-bridge SiC Schottky diode rectifiers** tied to the split-capacitor midpoint (`HV_MID = N = PE`). See [`../hardware/boost_converter/Boost_Converter_Architecture.md`](../hardware/boost_converter/Boost_Converter_Architecture.md).
* **Benefits of 2-Phase FB-LLC with Center-Tapped Split Output:**
  * **Reinforced Galvanic Isolation & Earth Grounding:** Decouples the `400 VDC` (`±200 VDC`) inverter bus from `96 VDC`, enabling safe single-point earth bonding of `HV_MID` (= AC Neutral) for residential safety and GFCI operation.
  * **Inherent DC Fault-Path Elimination:** Series resonant capacitors (`C_r`) and HF power transformers block DC fault current immediately when primary MOSFET gates turn off—eliminating the uncontrolled `96 V → HV` short-circuit diode path inherent to non-isolated boost topologies.
  * **Self-Balancing Split Rails & Zero Core Bias:** Center-tapped full-bridge rectification magnetically clamps `+200 VDC` and `-200 VDC` equal under unbalanced `120 VAC` loads with zero DC transformer core bias, while SiC diodes passively block reverse back-feed between parallel Boost modules.
  * **ZVS/ZCS Efficiency, Hybrid Balancing & Shedding:** Achieves `~96.5%` peak efficiency via primary ZVS and secondary ZCS, uses `PFM` for voltage regulation, small `PSM` trim for active Phase 1 / Phase 2 current sharing (while preserving exact `90°` interleaving), `PSM` soft-start into uncharged bulk capacitors, light-load single-phase shedding, and `HRTIM1` hardware burst mode at no-load.
* **Telemetry:** `TELEM_BOOST_FAST` (`0x300+`, `50 ms`) reports bus voltages, input current, temperature, active LLC phases (`Ph1`/`Ph2`), LLC operating mode (`Standby`/`SoftStart_PSM`/`Normal_PFM`/`Burst`), and split-rail balance status; `TELEM_BOOST_SLOW` (`0x310+`, `1 s`) reports switching frequency (`10 Hz/bit`), split-rail imbalance (`V_HV+ − |V_HV-|`), Phase 1 & Phase 2 tank RMS currents, magnetics temperature, and LLC diagnostic flags.

### 4.4 AC Inverter & Software Circuit Breaker Subsystem (AC Output Nodes)
* **Function:** Synthesizes clean `60 Hz` sinusoidal AC power directly from the earth-referenced `400 VDC` (`±200 VDC` split-rail) link while acting as solid-state/hybrid residential branch breakers.
* **Module Variants (Referenced to Earth-Bonded `HV_MID = N = PE`):**
  * **Single-Phase Module (`120 VAC`):** **1-leg half-bridge** (`L1` switching between `+200 V` and `-200 V`, referenced to `HV_MID / N`), `20 A RMS` continuous / `40 A Peak` surge. `±200 V` nominal provides `30 V` headroom above `170 V` (`120 VAC` peak) for clean modulation under surge loads.
  * **Split-Phase Module (`120/240 VAC`):** **2-leg half-bridge** (`L1` and `L2` `180°` out of phase, each switching between `+200 V` and `-200 V` referenced to `HV_MID / N`), `20 A RMS` continuous / `40 A Peak` surge, parallelable up to `100 A` service capacity.
* **Protection & Control:** Current-mode inner control loops clamp short-circuit fault currents cycle-by-cycle, emulating thermal-magnetic trip curves (`I2t` overload + instantaneous short-circuit trip) with remote reset capability via the Coordinator web UI, 2-pole DC disconnect (`HV+`/`HV-`), and AC line air-gap disconnect (`L1` or `L1`/`L2`, leaving Neutral bonded).

### 4.5 Supervisory Coordinator & Display Subsystem
* **Hardware Platform:** Dual-core Wi-Fi/BLE application processor paired with an integrated 7-inch capacitive touch LCD (`ESP32-S3` class).
* **Responsibilities:**
  * Orchestrates system-wide precharge, contactor sequencing, and shutdown (`System_State_MAchine.md`).
  * Computes slow-loop voltage trim and load-sharing commands across parallel modules (`Coordinator_Functions.md`).
  * Hosts the local touchscreen dashboard and embedded Wi-Fi web server for live power-flow visualization, alarm logging, and software breaker resets.

---

## 5. Hardware Partitioning & Mechanical Architecture
To maximize serviceability, noise immunity, and thermal performance, every high-power converter/inverter node is physically partitioned into three distinct assemblies:

```text
+-----------------------------------------------------------------------+
| 1. LOGIC & ISOLATION DAUGHTER CARD (Low-Voltage Domain)               |
|  - Local MCU (STM32G4 Series w/ High-Resolution Timer HRTIM)          |
|  - Galvanic CAN Isolator + Isolated Aux DC-DC Supply                  |
|  - Isolated Voltage (AMC-series) & Current Sensor Conditioning        |
|  - Local Waveshare 1.3" SPI IPS LCD + Keypad HAT & Hardware Fault Latches |
+-----------------------------------------------------------------------+
                  ||  Board-to-Board Signal Header (Isolated Signals)
+-----------------------------------------------------------------------+
| 2. HIGH-POWER SWITCHING BOARD (High-Current / High-dV/dt Domain)      |
|  - Heavy Copper Busbars + Through-Hole / Chassis Power Switches       |
|  - Local Gate Driver ICs (Placed tightly at MOSFET gates, VDD/VSS)    |
|  - Hall-Effect Isolated Current Sensors + Custom Wound Magnetics      |
|  - 24 VDC Main & Precharge Contactor Coil Drivers + Aux Feedback      |
+-----------------------------------------------------------------------+
                  ||  Low-Inductance Busbar Interconnect
+-----------------------------------------------------------------------+
| 3. BULK CAPACITOR BANK BOARD (Energy Damping Domain)                  |
|  - Parallel Array of Bulk Electrolytic Capacitors (Energy Storage)    |
|  - Polypropylene Film Decoupling Capacitors (High-Freq Ripple Absorb) |
|  - Bleeder / Active Discharge Resistors & Bus Voltage Sense Taps      |
+-----------------------------------------------------------------------+
```

### Key Hardware Standards Across Nodes
* **Daughter Card Standardization:** Using a common microcontroller pinout and daughter-card footprint lets multiple power stages (2-Phase FB-LLC Boost, Half-Bridge Inverter, Isolated DAB Battery, Generator, Isolated Grid Interface) share core firmware drivers, ADC DMA pipelines, and CAN stacks.
* **Gate Drive Placement:** Gate drivers sit directly on the power board adjacent to the power switches using unipolar `VDD/VSS` drive rails and Kelvin source returns where supported (such as 4-lead `TO-247-4L` or `miniBLOC` packages) to prevent parasitic `dI/dt` false turn-on.
* **Contactor Standard:** All high-power isolation and precharge relays standardize on `24 VDC` coils with auxiliary mirror/feedback contacts wired back to the MCU to detect welded contacts prior to state transitions.

---

## 6. Layered Control & Protection Hierarchy
System stability and safety rely on four decoupled time horizons so communication latency can never cause catastrophic hardware failure:

| Layer | Execution Domain | Response Time | Mechanism & Responsibility |
| :--- | :--- | :--- | :--- |
| **Layer 0: Hardware Fault Protection** | Analog Comparators & MCU `HRTIM` Fault Inputs | `< 1 us` | Direct hardware overcurrent, desaturation, and bus overvoltage comparators immediately shut off gate-driver PWM outputs without waiting for CPU interrupts. |
| **Layer 1: Fast Inner Loops** | Local Node MCU (`STM32` ISR / DMA) | `25 us - 100 us` (`10-40 kHz`) | Inductor current regulation, active phase current balancing across interleaved legs, and cycle-by-cycle AC sinusoidal current synthesis. |
| **Layer 2: Local Outer Loops & Droop** | Local Node MCU | `1 ms - 10 ms` (`100 Hz - 1 kHz`) | Bus voltage regulation, MPPT perturbation, thermal derating curves, and autonomous V-I droop fallback if Coordinator heartbeat times out (`> 500 ms`). |
| **Layer 3: Supervisory Dispatch** | Master Coordinator (`ESP32-S3` over CAN) | On change to `500 ms`; `20 ms` sharing updates for active parallel Boost and Inverter modules | Precharge sequencing, secondary voltage trim, parallel module load sharing, SoC reserve management, generator auto-start, and UI telemetry. Non-paralleled Inverters do not need routine `20 ms` commands. Inverter sharing details and bus capacity require validation. |

---

## 7. Grounding, Galvanic Isolation, EMI & Safety Architecture

### 7.1 Authoritative Galvanic Isolation & Grounding Map
The platform eliminates ground/neutral short loops across all operating modes by partitioning power and logic into six clearly bounded galvanic domains:

```text
  [DOMAIN A: UTILITY AC]     [DOMAIN B: BATTERY PACKS]     [DOMAIN C: 96 VDC ENERGY BUS]
  120/240 VAC (N bonded PE    48 V Nominal (40-58.4 V)      0 V (LV_BUS-) to +96 VDC
  at Utility Service Panel)   Floating SELV/ELV per pack    Single-point LV_BUS- -> [DC GFDI] -> PE
           |                         |                        ^        ^        ^
     [Grid Interface]          [Battery DAB]                  |        |        |
    PFC + Isol. LLC/PSFB      Isol. 1:2 HF Xfmr         [Solar MPPT] [Gen]   [Boost FB-LLC Pri]
    === >= 4 kVrms ISO ===   === >= 2.5 kVrms ISO ===         |        |        |
           +-------------------------+------------------------+--------+        |
                                                                                |
                                                                    === >= 4 kVrms ISO ===
                                                                    (T1/T2 HF Transformers)
                                                                                |
  [DOMAIN E: LOCAL LOGIC]    [DOMAIN F: CAN TRUNK]         [DOMAIN D: 400 VDC SPLIT / AC OUT]
  AUX_LOGIC_24V / 3.3 V      Isolated 5 V CAN Bus          +200 V (HV+) / 0 V (HV_MID) / -200 V (HV-)
  (Diode-OR'd isolated 24V   (5 kVrms ISO1042DWV +          Single-point HV_MID -> PE (AC Neutral Bond)
   + per-tap eFuse/iso brick) card isolated DC/DC)          Feeds 120V/240V Half-Bridge Inverters + GFCI
```

| Domain | Reference Potential | Isolation Boundary | Safety & Ground-Fault Protection |
| :--- | :--- | :--- | :--- |
| **Chassis / Earth (`PE`)** | Earth Ground (`0 V`) | Bonded to all metal enclosures, heatsinks, cabinet frames, and SPDs. | Continuous low-impedance equipment grounding conductor (EGC). |
| **Domain A — Utility AC Grid** | Utility Neutral bonded to `PE` at main service | Reinforced isolation (`>= 4 kVrms`) inside Grid Interface second-stage DC-DC (`PFC → Isolated LLC/PSFB → 96 V`). | Prevents Utility Neutral (`PE`) from shorting into `LV_BUS-` (`PE`) or `HV_MID` (`PE`). |
| **Domain B — Battery Packs (`48 V`)** | Floating `< 60 VDC` per pack (isolated from `PE` and from other packs) | Galvanic isolation (`>= 2.5–4 kVrms`) across each Battery DAB transformer + isolated `48 V → 24 V` auxiliary supply + isolated DALY BMS UART/RS485. | Prevents accidental `B-` to `PE` faults from bypassing DALY BMS low-side disconnect FETs; eliminates multi-pack ground loops. |
| **Domain C — `96 VDC` Energy Bus** | `LV_BUS-` (`0 V_LV`) bonded to `PE` at **one single point** through a `1–5 A` DC GFDI breaker | Isolated from Utility AC, Battery Packs, `400 VDC` bus, and Control/CAN logic. | Prevents floating `96 V` bus potentials; DC GFDI aux contact trips the hardwired latched E-Stop Safety Relay on any `LV_BUS+`/`PV+` ground fault; Class-T/DC fuses on every branch. |
| **Domain D — `400 VDC` Split Bus & AC Output** | `HV_MID` (`0 V`) single-point bonded to `PE` = AC `Neutral` (`N`); `HV+ = +200 V`, `HV- = -200 V` | Reinforced isolation (`>= 4 kVrms`) across Boost FB-LLC transformers (`T1`/`T2`). | Establishes true residential Earth-grounded `120/240 VAC` Neutral-Ground bond for standard AC GFCI/RCD personnel protection; 2-pole `500 VDC` fuses + DC disconnect on `HV+`/`HV-`, AC line contactor on `L1`/`L2`. |
| **Domain E — Local Module Logic (`GND_CTRL`)** | Regulated `24.0 VDC` (`AUX_LOGIC_24V`, per-tap fuse/eFuse + optional `24:24 V` iso brick) | Isolated from all power buses via battery auxiliary DC-DCs, isolated gate drivers (`UCC21520` / `1ED3122`), `AMC1311` voltage isolators, and `ACS37002` Hall sensors. | Zero high-voltage ground reference on MCU daughter card; stays online during E-Stop for logging/HMI; see [`docs/System_Safety_and_Protection_Architecture.md`](../docs/System_Safety_and_Protection_Architecture.md). |
| **Domain F — Main & Solar CAN Trunks (`GND_CAN_ISO`)** | Isolated differential CAN bus domain | `5 kVrms` `ISO1042DWV` + card-mounted isolated DC/DC on every node. | Zero inter-cabinet ground loops; isolated Solar segments behind gateways. |

### 7.2 Precharge & Stored-Energy Safety
* **Inrush Protection:** No `96 VDC` main contactor may close across an uncharged input capacitor bank (`Delta-V > 1.5 V` prohibited without precharge). The `400 VDC` (`±200 V`) bulk capacitor bank is ramped gently from `0 V` via Boost FB-LLC `PSM` soft-start (no separate `400 V` boost output precharge resistor needed).
* **Safe Discharge:** High-voltage split capacitor boards include fail-safe spring-closed **Normally-Closed (`NC`) `24 VDC` discharge relays** (powered from `AUX_COIL_24V`) across both `+200 V` and `-200 V` rails (`0 W` running loss; drops `< 48 VDC` in `< 10 s` on shutdown/E-Stop/power loss), backed up by high-ohm passive bleeders (`< 2 W`/rail) and HV-Present LEDs.

### 7.3 Fail-Safe Defaults
* **Hardwired E-Stop & Contactor Drop-Out:** Opening the latched E-Stop / DC GFDI safety loop immediately drops `AUX_COIL_24V` to `0 V`, triggering `< 1 µs` `HRTIM1_FLT3` PWM kill, opening all contactors, closing the `NC` capacitor discharge relays, and triggering NEC 690.12 rooftop Solar Rapid Shutdown—while `AUX_LOGIC_24V` keeps the Coordinator and STM32 cards online to log the fault.
* **Inherent Boost DC Blocking:** Loss of Boost FB-LLC primary gate drive immediately interrupts energy transfer across the resonant capacitor (`C_r`) and transformer—zero DC fault feed-through from `96 V` to `400 V`.
* **Comms Loss Continuity:** Loss of CAN communication leaves active critical loads powered via local autonomous droop while locking out remote contactor closures until Coordinator sync is re-established.

---

## 8. Cross-Reference Document Map
* **Operational States & Fault Transitions:** See [`architecture/System_State_MAchine.md`](./System_State_MAchine.md)
* **Supervisory Logic & HMI Scope:** See [`architecture/Coordinator_Functions.md`](./Coordinator_Functions.md)
* **CAN Frame IDs, Payloads & Timings:** See [`protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)
* **Solar Gateway Scope & Preliminary Hardware:** See [`architecture/nodes/Solar_Gateway_Node.md`](./nodes/Solar_Gateway_Node.md)
* **Individual Hardware Module Specs:** See node specifications under `nodes/`
