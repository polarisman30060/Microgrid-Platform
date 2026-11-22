# Microgrid Platform — Engineering Punchlist ("Digital Punchcard")

Last Updated: 2026-11-22  
Status: Living tracking document for open architectural, electrical, hardware, protocol, and firmware gaps across the repository.

Use this list to pick the next high-impact item, verify exit criteria, and check items off as decisions are finalized and verified.

---

## P0 — Critical Electrical & Safety Architecture (All Architectural Decisions Resolved 2026-11-22)

*All four P0 architectural blockers are now specified in [`System_Safety_and_Protection_Architecture.md`](System_Safety_and_Protection_Architecture.md) and [`System_Architecture.md`](../architecture/System_Architecture.md). Physical component selection (exact fuse/contactor/GFDI/safety-relay part numbers and bench trip verification) proceeds into hardware bring-up.*

- [x] **Resolve Auxiliary Lifeline Multi-Source Parallelism & Fault Isolation** → Migrated to Split Diode-OR'd `24.0 VDC` Regulated Isolated Auxiliary Bus (`AUX_LOGIC_24V` + `AUX_COIL_24V`) with per-tap fuses/eFuses.
- [x] **Create Full-System Galvanic Isolation, Grounding & Neutral-Bonding Map** → Completed in [`System_Architecture.md`](../architecture/System_Architecture.md) (`400 VDC` / `±200 V` FB-LLC isolated split bus with `HV_MID = N = PE`; `LV_BUS-` single-point grounded via DC GFDI; Battery DAB isolated; Utility-Tie two-stage isolated).
- [x] **Define DC Overcurrent Fusing, DC GFDI, Contactor Coordination & `400 VDC` Stored-Energy Discharge** → Completed in [`System_Safety_and_Protection_Architecture.md`](System_Safety_and_Protection_Architecture.md) (Class-T / `500 VDC` 2-pole fuses, hermetically sealed DC contactors, `1–5 A` DC GFDI breaker, and fail-safe spring-closed `NC` discharge relays + backup bleeders).
- [x] **Design Hardwired Emergency Stop (E-Stop) & NEC 690.12 Rapid Shutdown Interlock** → Completed in [`System_Safety_and_Protection_Architecture.md`](System_Safety_and_Protection_Architecture.md) (latched safety relay cutting `AUX_COIL_24V` + `HRTIM1_FLT3` + roof Solar keep-alive while keeping `AUX_LOGIC_24V` live).

---

## P1 — STM32 Common Control Card Hardware Holds (Before PCB Routing / Ordering)

Located in [hardware/STM32_control_card/](../hardware/STM32_control_card/):

- [x] **Fix `24 V` Input Protection vs. Buck Voltage Ceiling (BOM/Selection Resolved 2026-11-22)**
  - **Resolution:** Swapped `40 V` `LM2675` + `SMBJ33A` (`53.3 V` clamp) to stocked DigiKey **`LM2576HVT-5.0/NOPB`** (`60 V` op / `63 V` abs max, TO-220-5) + **`SMBJ30A`** (`48.4 V` max clamp, `11.6 V` margin) + **`Bourns SRR1280-331K`** (`330 µH`, `1.4 A` sat) + `63 V` `EEU-FC1J101` / `1000 µF` `EEU-FC1E102`. Capture in EasyEDA power sheet and verify on prototype bench.

- [x] **Finalize Stocked Regulated Isolated CAN Supply (BOM/Selection Resolved 2026-11-22)**
  - **Resolution:** Both `UCC33421DHAR` and `UCC12050DVER` were non-stock/out-of-stock on DigiKey. Switched baseline in [STM32_Control_Card_BOM.md](../hardware/STM32_control_card/STM32_Control_Card_BOM.md) to stocked **`Analog Devices ADuM6020-5BRIZ`** (`5 kVrms` reinforced, `8.3 mm` creepage, easy-to-hand-solder `1.27 mm` `16-SOIC` wide body, regulated `5.0 V ±5%` down to `0 mA` no-load). Update `CAN Tranciever.esch2` footprint & `≥ 8 mm` PCB moat keep-outs before routing.

- [x] **Complete All 4 EasyEDA Pro Schematic Sheets & Netlist Audit (2026-11-22)**
  - **Captured & audited (83 parts across 4 sheets):** `Micro + Headers.esch2` (`U1` 128/128, `J1` 40/40, `J2` 40/40, `J3`, `J4`, `J5`, `LED2`), `Decoupling + Filtering.esch2` (`24 MHz` HSE, `32.768 kHz` LSE, `BOOT0`, `SW1`, decoupling, `R19–R21` safe-reset pull-downs), `CAN Tranciever.esch2` (`ISO1042DWV` + `ADuM6020-5BRIZ`), and `Power Supplies.esch2` (`MF-R050`, `B360A-13-F`, `SMBJ30A`, `LM2576HVT-5.0/NOPB`, `SRR1280-331K`, `TLV76133DCYR`, `LED1`). BOM reconciled 1-to-1 in [STM32_Control_Card_BOM.md](../hardware/STM32_control_card/STM32_Control_Card_BOM.md).
  - **Final pre-PCB check:** Confirm `J3` Pin 9 (`EXP-PE9` → `EXP_PE9`) saved & EasyEDA ERC passes cleanly before pushing netlist to `PCB1.epcb2`.

- [ ] **Complete Mechanical Mock-Up & 4-Layer PCB Layout (`PCB1.epcb2`)**
  - **Done when:** HAT hole pitch and mating headers are caliper-verified on physical parts, 1:1 print check passes, isolation moat keep-outs are verified on all 4 layers, and DRC passes cleanly.

---

## P2 — Control Dynamics, Synchronization & Mechanical Integration

- [x] **Define `96 VDC` & `400 VDC` Bulk Capacitance, Soft-Start Pre-Charge, Voltage-Droop Curves & Load-Dump Protection** → Completed in [`Bus_Capacitance_Droop_and_Transient_Spec.md`](Bus_Capacitance_Droop_and_Transient_Spec.md) (`60 mF`/rail `±200 V` bank; `330 mF` `96 V` zero-dump-load bank; Battery DAB soft-start cold pre-charge; deterministic `91.2–100.8 V` autonomous droop map).

- [ ] **Define Inverter `60 Hz` Hardware Sync Architecture & Time-Master Failover**
  - **Issue:** `HRTIM1_SCOUT` outputs a 16-cycle pulse (~`94 ns`), not the `10 µs` zero-cross pulse assumed in older docs ([Inverter_Module_IO_Requirements.md](../hardware/STM32_control_card/Inverter_Module_IO_Requirements.md)), and Time-Master failover if the master inverter trips offline is undefined.
  - **Done when:** Timer/sync pulse format, isolation, PLL phase-alignment tolerance, and glitch-free master election/handoff are specified.

- [ ] **Define Enclosure, Thermal Cooling & Stacked-Busbar Mechanical Standard**
  - **Issue:** High-power modules (e.g., `10 kW` 2-Phase FB-LLC Boost rejecting `~340 W` at `~96.5%` efficiency) need defined airflow paths, heatsink clearance, M4 busbar interconnect geometry, reinforced isolation clearances (`≥ 8 mm`), and safe maintenance lockout/tagout.
  - **Done when:** A mechanical/thermal baseline document defines rack/cabinet envelope, airflow budget, and busbar/connector clearances.

---

## P3 — CAN Protocol, Telemetry & Solar Gateway

- [ ] **Define Solar Gateway Payloads (`0x1A0–0x1BF`, `0x430–0x45F`), Child Fault Forwarding & Provisioning (`0x7C0`)**
  - **Issue:** IDs are reserved in [CAN_Messages_Definitions.md](../protocol/CAN_Messages_Definitions.md), but wire payloads for child-control routing, replies, `500 ms` summaries, on-demand child detail, local Solar emergency IDs, and unconfigured-child provisioning are still TBD.
  - **Done when:** 8-byte payloads are specified and validated against the 500-Solar-node (`16 × 32`) bus-utilization budget (`≤ 40%` sustained / `≤ 60%` peak).

- [ ] **Build & Measure `NUCLEO-G0B1RE` Solar Gateway Prototype**
  - **Done when:** Dual independent Classical CAN channels (`FDCAN1`/`FDCAN2`) pass the 32-child traffic, bus-off, and isolation verification gates in [Solar_Gateway_Node.md](../architecture/nodes/Solar_Gateway_Node.md).

- [ ] **Validate Parallel Inverter `20 ms` Current-Sharing Profile & Slow Telemetry**
  - **Issue:** `20 ms` parallel Inverter sharing semantics, feedback freshness (vs. `200 ms` telemetry), and timeout behavior are undefined; Inverter slow telemetry (`I²t` thermal accumulator %, peak surge, neutral imbalance, sync error) is also a useful future addition.
  - **Done when:** Parallel Inverter sharing contract and bus-load impact are validated and documented.

- [ ] **Wire `TELEM_BOOST_SLOW` (`0x310 + Instance`) into Coordinator & Define System Energy Counters**
  - **Issue:** `TELEM_BOOST_SLOW` (`0x310+`) is updated for the 2-phase FB-LLC topology (switching frequency, `±200 V` split-rail imbalance, Phase 1/2 tank RMS currents, magnetics temp, LLC diagnostic flags) and mirrored in the PC and Nucleo Boost 0 simulators; Coordinator RX decoding and system energy integration (`kWh` / `Ah` counters + SD persistence) remain open.
  - **Done when:** Coordinator decodes `0x300` + `0x310` and energy logging strategy is decided.

- [ ] **Define Command Retry, Duplicate-Command & Protocol Version Compatibility Rules**
  - **Done when:** In-flight command timeouts, idempotency/sequence rules, and malformed-command ACK policy are finalized across all node profiles.

---

## P4 — Firmware, Bench Verification & Field Lifecycle

- [ ] **Run Isolated CAN-Bench Integration Tests (Coordinator ↔ PC Simulator ↔ Flashed Nucleo Boards)**
  - **Issue:** Coordinator firmware, all 6 Nucleo simulator images, and the PC simulator compile/pass unit tests, but physical CAN-bench verification is still pending ([Coordinator_CAN_Implementation.md](Coordinator_CAN_Implementation.md)).
  - **Done when:** Full-duplex discovery, telemetry, timeout, node-loss, bus-off recovery, and recovery lifecycles are verified on an isolated bench with no power hardware attached.

- [ ] **Expand Coordinator Firmware Beyond Battery 0 RX**
  - **Remaining work:** Add bounded `CMD_NODE_CONTROL` TX + `CMD_ACK` tracking, Boost 0 decoding (`0x300`, `0x606`, `0x706`, `0x765`), remaining main-bus node decoders, commissioned topology persistence, SD fault logging, and live WebSocket/API wiring to [web/index.html](../firmware/coordinator_module/web/index.html).
  - **Done when:** Coordinator supports full supervisory discovery, telemetry, explicit commands/ACKs, and fault logging without violating the CPU 0 / CPU 1 core-affinity rules.

- [ ] **Design STM32 CAN Bootloader & Field OTA Strategy**
  - **Issue:** Updating firmware across dozens of modules (and up to 500 Solar MPPTs) manually via ST-LINK does not scale; 29-bit CAN IDs are already reserved for offline OTA.
  - **Done when:** Flash partition layout, bootloader entry/rollback safety, and Coordinator-driven OTA protocol are documented before production module firmware begins.

- [ ] **Define Security & Persistence Hardening for Coordinator**
  - **Done when:** NVS/flash encryption for stored Wi-Fi credentials, operator authentication/audit trail for remote breaker resets, and SD-absent fallback behavior are defined ([Software_Design_Constraints.md](Software_Design_Constraints.md)).

---

## Completed Items (2026-11-22)

- [x] Switched Boost Converter baseline from non-isolated 5-phase boost (`380 VDC`) to **10 kW 2-phase interleaved Full-Bridge LLC** (`96 VDC` → earth-referenced **`400 VDC` / `±200 VDC` split-rail** with center-tapped self-balancing SiC diode rectifiers) and authored [`Boost_Converter_Architecture.md`](../hardware/boost_converter/Boost_Converter_Architecture.md).
- [x] Authored the full-system **Galvanic Isolation, Grounding & Neutral-Bonding Map** in [`System_Architecture.md`](../architecture/System_Architecture.md) (`HV_MID` single-point bonded to `PE` / AC Neutral; `LV_BUS-` referenced to `PE` via DC GFDI; zero ground/neutral short loops).
- [x] Authored [`Battery_Isolation_Topology_Study.md`](../hardware/Battery_Isolation_Topology_Study.md) and migrated Battery Module baseline to a **2.5 kW Isolated Bidirectional Dual Active Bridge (DAB)** to eliminate the DALY BMS `B-` ground-fault bypass hazard.
- [x] Updated Inverter baseline to half-bridge topologies (`1-leg 120 VAC` / `2-leg 240 VAC` referenced to `HV_MID = N = PE`) and Utility-Tie baseline to a two-stage isolated `PFC + LLC/PSFB` topology.
- [x] Verified 100% I/O compatibility of the draft `STM32G474QET6` control card (`J1`/`J2`) across all updated topologies (`8` HRTIM outputs for Boost FB-LLC and Battery DAB; zero card PCB changes required).
- [x] Updated `TELEM_BOOST_FAST` (`0x300+`), `TELEM_BOOST_SLOW` (`0x310+`), `Fault_Codes.md`, PC simulator, unit tests, and Nucleo Boost 0 simulator for the 2-phase FB-LLC `400 VDC` split-rail architecture.

## Completed Items (2026-11-20)

- [x] Documented 5-phase Boost active phase shedding/balancing and drafted `TELEM_BOOST_SLOW` (`0x310 + Instance`) for active phase count and phase-balance diagnostics.
- [x] Authored initial [Fault_Model.md](../architecture/Fault_Model.md) and [Fault_Codes.md](../protocol/Fault_Codes.md) (replacing empty 0-byte stubs).
- [x] Reconciled documentation drift across node definitions, hardware baselines, `Project_Context.md`, and `System_State_MAchine.md` (heartbeat cadences, `96 VDC` `±5%` operating window, `ISO1042DWV` transceiver, Waveshare `1.3"` LCD HAT, and updated control-card contactor pins).
