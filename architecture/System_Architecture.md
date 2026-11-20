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
  | Solar |           | Bi-Dir|           |  Gen  |           | Grid  |
  | MPPT  |           | DC-DC |           | Rect/ |           | Chrgr |
  | Nodes |           | + BMS |           | Cnvrt |           | Node  |
  +---+---+           +---+---+           +---+---+           +---+---+
      |                   |                   |                   |
======+===================+===================+===================+======  PRIMARY LV DC BUS
                                                                     |     96 VDC Nominal (+/- 5% Reg)
                                                                 +---+---+ Bulk Electrolytic + Film Bank
                                                                 | 5-Ph  |
                                                                 | Boost | 10 kVA Modular Stage(s)
                                                                 | Stage |
                                                                 +---+---+
                                                                     |
=====================================================================+===  HV DC INVERTER LINK
                    |                                   |                  380 VDC Nominal
                +---+---+                           +---+---+              Bulk Electrolytic + Film Bank
                | 120V  |                           | 240V  |
                | 1-Ph  |                           | Split |              Transformerless Current-Mode
                | AC Out|                           | AC Out|              Inverter / Software Breakers
                +---+---+                           +---+---+
                    |                                   |
              [120 VAC Loads]                   [120/240 VAC Loads]

-------------------------------------------------------------------------  COMMON AUXILIARY LIFELINE
  30V / 50kHz High-Freq AC Lifeline & 24 VDC Contactor Power Rail          Feeds Isolated Local Supplies
--------------------------------------------------------------------------  MAIN CAN 2.0B BUS (500 kbps)
  Master Coordinator / HMI  <===>  Main-bus Nodes + Solar Gateways         Telemetry, Trim & State Sync
                                      |
                           +----------+----------+
                           | Isolated local CAN  |
                           | Solar children 0-31 |
```

---

The main bus supports the registry-assigned Battery, Boost, Generator, Grid Interface, Inverter, Display, and Solar Gateway instances. Solar scales behind up to 16 gateways, each serving at most 32 local child slots (512 total slots for the 500-node target). Gateways aggregate/filter child traffic upstream; they are not transparent repeaters. Main-bus IDs and Solar-local IDs may overlap numerically because the CAN segments are physically isolated. The preliminary gateway scope, dual-CAN prototype direction, and failure behavior are described in the [Solar Gateway Node design](./nodes/Solar_Gateway_Node.md); payloads and production hardware remain open.

## 3. Electrical Bus Specifications

| Bus Name | Nominal Rating | Operating Window | Primary Function | Protection & Isolation |
| :--- | :--- | :--- | :--- | :--- |
| **Primary LV DC Energy Bus** | `96.0 VDC` | `91.2 V - 100.8 V` (+/- 5% normal)<br>`84.0 V - 108.0 V` (fault bounds) | Common energy aggregation bus linking solar, batteries, generator, grid charger, and boost inputs. | Precharge-protected copper busbars, per-module DC contactors, hardware OV/UV comparators, dedicated bulk capacitor bank. |
| **High-Voltage (HV) DC Link** | `380.0 VDC` | `360.0 V - 400.0 V` (normal)<br>`340.0 V - 420.0 V` (trip bounds) | Stiff DC source feeding transformerless 120 VAC and 240 VAC split-phase inverter output stages. | Soft-start via boost stage, active/bleeder discharge path on shutdown, fast hardware overvoltage crowbar/gate-inhibit. |
| **Auxiliary Power Lifeline** | `30 V` HF AC (`50 kHz`) / `24 VDC` Local | Continuous black-start & ride-through | Powers node daughter cards, isolated gate-driver supplies, local displays, and 24 VDC contactor coils. | Transformer-coupled galvanic isolation at every node tap; independent of main 96 V / 380 V bus contactors. |
| **Control & Telemetry Bus** | Main `CAN 2.0B` bus at `500 kbps`, plus isolated Solar segments | Differential `5 V` / `3.3 V` isolated | Carries supervisory commands, module telemetry, fault reports, and HMI status; Solar child traffic is aggregated by gateways. | Galvanically isolated CAN transceivers (`> 2.5 kVrms`), `120 Ohm` end terminations per physical segment, bounded Bus-Off recovery. |

---

## 4. Functional Subsystem Breakdown

### 4.1 Energy Storage & BMS Subsystem (Battery Nodes)
* **Function:** Anchors primary 96 VDC bus stability and stores surplus renewable or generator energy.
* **Architecture:** Supports battery banks (such as LiFePO4 and 16S Li-Ion) interfaced through bidirectional DC-DC buck-boost stages so battery state-of-charge (SoC) voltage swings never disturb the regulated 96 VDC bus.
* **BMS Integration:** Interfaces directly with pack-level BMS telemetry (cell min/max voltages, pack temperatures, charge/discharge enable flags) to enforce smooth current derating before hard BMS cutoff thresholds are reached.
* **Black-Start Role:** Provides auxiliary lifeline support so the microgrid can cold-boot without grid or solar presence.

### 4.2 Renewable & Fuel Generation Subsystems (Solar, Generator, Grid Nodes)
* **Solar MPPT Nodes:** Step down or boost PV string voltages onto the 96 VDC bus using maximum power point tracking, transitioning automatically to constant-voltage curtailment if the bus approaches its upper regulation limit (`100.8 VDC`).
* **Petrol Generator Module:** Rectifies and conditions power from a backup fuel generator onto the 96 VDC bus with programmable current/load ramping to keep the engine inside its peak fuel-efficiency band and prevent stall on step loads.
* **Grid Input / Charger Module:** Provides isolated AC-to-96 VDC supplemental charging when utility power is available and battery SoC drops below user-defined reserve thresholds.

### 4.3 High-Voltage DC-DC Boost Subsystem (Boost Nodes)
* **Function:** Steps the `96 VDC` primary bus up to the `380 VDC` inverter link at `10 kVA` modular capacity per unit.
* **Topology:** 5-phase interleaved boost architecture operating at low switching frequencies (`<= 40 kHz` per phase).
* **Benefits of 5-Phase Interleaving:**
  * Multiplies effective ripple frequency by `5x`, drastically shrinking input/output ripple current stresses on the capacitor banks.
  * Splits high primary bus currents across five parallel custom-wound inductors and power switch legs.
  * Enables phase-shedding at light loads to maximize standby conversion efficiency.

### 4.4 AC Inverter & Software Circuit Breaker Subsystem (AC Output Nodes)
* **Function:** Synthesizes clean `60 Hz` sinusoidal AC power directly from the `380 VDC` link while acting as solid-state/hybrid residential branch breakers.
* **Module Variants:**
  * **Single-Phase Module:** `120 VAC`, `20 A RMS` continuous / `40 A Peak` surge.
  * **Split-Phase Module:** `120/240 VAC`, `20 A RMS` continuous / `40 A Peak` surge, parallelable up to `100 A` service capacity.
* **Protection & Control:** Current-mode inner control loops inherently clamp short-circuit fault currents cycle-by-cycle, emulating thermal-magnetic trip curves (`I2t` overload + instantaneous short-circuit trip) with remote reset capability via the Coordinator web UI.

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
|  - Local 1.3" OLED Diagnostic Display & Hardware Fault Latches        |
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
* **Daughter Card Standardization:** Using a common microcontroller pinout and daughter-card footprint lets multiple power stages (Boost, Inverter, Battery Buck-Boost, Generator) share core firmware drivers, ADC DMA pipelines, and CAN stacks.
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
| **Layer 3: Supervisory Dispatch** | Master Coordinator (`ESP32-S3` over CAN) | On change to `500 ms`; conditional `20 ms` Boost balancing | Precharge sequencing, secondary voltage trim, parallel module load sharing, SoC reserve management, generator auto-start, and UI telemetry. Routine `20 ms` Inverter commands are not planned. |

---

## 7. Grounding, EMI & Safety Architecture

### 7.1 Galvanic Domain Separation
* **Chassis / Earth Ground (`PE`):** Bonded to all metal enclosures, heatsinks, and surge arrestors.
* **96 VDC Primary Return (`LV_BUS-`):** Isolated from low-voltage MCU logic ground (`GND_LOGIC`).
* **380 VDC Link & AC Neutral/Line Domain:** Monitored for ground-fault leakage current and isolated from low-voltage control domains across reinforced isolation barriers.
* **Logic & CAN Ground (`GND_ISO`):** Referenced locally per daughter card via isolated auxiliary power transformers, with CAN transceivers bridged across an additional galvanic isolation barrier to eliminate ground loops between cabinets.

### 7.2 Precharge & Stored-Energy Safety
* **Inrush Protection:** No main contactor may close across an uncharged capacitor bank (`Delta-V > 5 V` prohibited without precharge).
* **Safe Discharge:** High-voltage capacitor boards include passive bleeder resistors for fail-safe discharge below `< 48 VDC` within maintenance safety windows, supplemented by controlled active discharge when commanded into `SHUTDOWN`.

### 7.3 Fail-Safe Defaults
* **Contactor Drop-Out:** Loss of `24 VDC` coil power automatically drops all main and precharge contactors `OPEN`.
* **Comms Loss Continuity:** Loss of CAN communication leaves active critical loads powered via local autonomous droop while locking out remote contactor closures until Coordinator sync is re-established.

---

## 8. Cross-Reference Document Map
* **Operational States & Fault Transitions:** See [`architecture/System_State_MAchine.md`](./System_State_MAchine.md)
* **Supervisory Logic & HMI Scope:** See [`architecture/Coordinator_Functions.md`](./Coordinator_Functions.md)
* **CAN Frame IDs, Payloads & Timings:** See [`protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md)
* **Solar Gateway Scope & Preliminary Hardware:** See [`architecture/nodes/Solar_Gateway_Node.md`](./nodes/Solar_Gateway_Node.md)
* **Individual Hardware Module Specs:** See node specifications under `nodes/`
