# Microgrid Platform — Engineering Punchlist ("Digital Punchcard")

Last Updated: 2026-11-20  
Status: Living tracking document for open architectural, electrical, hardware, protocol, and firmware gaps across the repository.

Use this list to pick the next high-impact item, verify exit criteria, and check items off as decisions are finalized and verified.

---

## P0 — Critical Electrical & Safety Architecture (Must Resolve Before Power-Stage Hardware)

- [ ] **Resolve `30 V / 50 kHz` Auxiliary Lifeline Multi-Source Parallelism & Fault Isolation**
  - **Issue:** Multiple Battery Nodes are currently specified to drive a shared `30 V 50 kHz` square-wave AC lifeline ([Hardware_Standards_and_Control_Power.md](Hardware_Standards_and_Control_Power.md), [Battery_and_BMS_Hardware.md](../hardware/Battery_and_BMS_Hardware.md)). Unsynchronized square-wave inverters tied to a common AC bus will short-circuit against one another, and a single shorted module tap could collapse control power for the entire microgrid.
  - **Decision needed:** Choose one topology — (a) diode-OR'd isolated DC auxiliary rail (e.g., `24–30 VDC`) feeding local isolated DC/DC taps, (b) phase-locked master/slave 50 kHz AC bus with impedance/current limiting, or (c) single-master lifeline with automatic failover.
  - **Done when:** Waveform (RMS/peak), voltage tolerance, per-tap short-circuit current limiting, multi-battery sharing/failover, and host-board rectifier/regulator ratings are specified in [Hardware_Standards_and_Control_Power.md](Hardware_Standards_and_Control_Power.md).

- [ ] **Create Full-System Galvanic Isolation, Grounding & Neutral-Bonding Map**
  - **Issue:** Inverters are transformerless (`380 VDC → 120/240 VAC`), where residential AC Neutral bonds to Earth (`PE`). Because the 5-phase Boost (`96 V → 380 V`) and Battery (`48 V → 96 V`) converters are non-isolated, Earth-bonding AC Neutral places the `96 VDC` bus, batteries, generator, and rooftop Solar arrays at hazardous potentials relative to Earth — and creates a direct short-circuit path if a non-isolated Utility-Tie PFC connects to Utility Neutral while Inverters are active.
  - **Decision needed:** Define exact galvanic isolation boundaries across Grid Interface, Boost, Battery, Solar, and Inverter stages, plus DC Ground Fault Detection & Interruption (GFDI) and AC GFCI/RCD strategy.
  - **Done when:** An authoritative isolation & grounding diagram is added to [System_Architecture.md](../architecture/System_Architecture.md) confirming zero ground/neutral short loops across all operating modes.

- [ ] **Define Boost `96 V → 380 V` Non-Isolated Fault Path, Fusing & Contactor Break Coordination**
  - **Issue:** In a standard boost topology, gating off the MOSFETs does not block DC fault current from flowing from the `96 VDC` bus through the inductors and SiC boost diodes into a `380 VDC` short. Reliance solely on `Q_ISOL` opening under massive DC fault current risks welded contactors and arc flash.
  - **Decision needed:** Specify passive DC overcurrent protection (Class-T / semiconductor fuses or solid-state DC breaker) on every Battery pack, `96 V` branch, Boost input, and `380 V` branch; verify `Q_ISOL` DC interrupt ratings; define `380 V` bulk-capacitor active/passive discharge time to `< 48 VDC`.
  - **Done when:** A system overcurrent coordination and stored-energy discharge table is documented and cross-checked against hardware baselines.

- [ ] **Design Hardwired Emergency Stop (E-Stop) & Rapid Shutdown Interlock**
  - **Issue:** Local MCU trips and software breakers are defined, but there is no physical, software-independent E-Stop loop (e.g., breaking the `24 VDC` contactor coil supply rail so all main contactors drop open immediately) or NEC 690.12 rooftop Solar Rapid Shutdown strategy.
  - **Done when:** Hardwired E-Stop topology (keeping control/logging logic alive where safe, or defining full shutdown behavior) and rooftop Solar shutdown requirements are documented.

---

## P1 — STM32 Common Control Card Hardware Holds (Before PCB Routing / Ordering)

Located in [hardware/STM32_control_card/](../hardware/STM32_control_card/):

- [ ] **Fix `24 V` Input Protection vs. Buck Voltage Ceiling**
  - **Issue:** Proposed `SMBJ33A` TVS clamps up to `53.3 V`, exceeding the `LM2675M-5.0/NOPB` `40 V` max input rating ([STM32_Control_Card_Mechanical_and_Power.md](../hardware/STM32_control_card/STM32_Control_Card_Mechanical_and_Power.md)).
  - **Done when:** Host `24 V` supply range/transients are bounded and either a higher-voltage buck (e.g., `60 V+` rated) or lower-clamp/active surge stopper is selected, verified, and updated in the BOM and schematic.

- [ ] **Finalize Isolated CAN Supply (`UCC33421DHAR`) & Full Isolation Barrier**
  - **Issue:** `MEJ1S0505SC` is on hold (requires `20 mA` min load vs. `ISO1042` `2.8–4.1 mA` recessive current). Schematic moved to `UCC33421`, but stock/pricing, `0805` vs. `0402` high-frequency decoupling substitution, and system working-voltage / creepage / clearance qualification remain open.
  - **Done when:** Regulated isolated DC/DC part + passives are verified in stock, BOM updated, and creepage/clearance rules set for `PCB1.epcb2`.

- [ ] **Complete Remaining EasyEDA Pro Schematic Sheets & Run ERC**
  - **Already captured:** `Micro + Headers.esch2` (`U1`, `J4`, `J5`), `Decoupling + Filtering.esch2` (37 parts), `CAN Tranciever.esch2` (16 parts).
  - **Remaining sheets/blocks:** `24 V → 5 V → 3.3 V` power supply, `24 MHz` HSE & `32.768 kHz` LSE crystals, host headers `J1` & `J2`, expansion header `J3`, status LEDs, and `10 kΩ` pull-downs on active-high PWM/`GATE_EN`/contactor lines (including `PA9`/`PA10` UCPD dead-battery caveat).
  - **Done when:** Full schematic passes ERC with zero unresolved errors, every pin/AF is verified against `DS12288 Rev 6` and the Waveshare HAT schematic, and BOM matches the schematic netlist.

- [ ] **Complete Mechanical Mock-Up & 4-Layer PCB Layout (`PCB1.epcb2`)**
  - **Done when:** HAT hole pitch and mating headers are caliper-verified on physical parts, 1:1 print check passes, isolation moat keep-outs are verified on all 4 layers, and DRC passes cleanly.

---

## P2 — Control Dynamics, Synchronization & Mechanical Integration

- [ ] **Define `96 VDC` Bus Voltage-Droop Curves & Load-Dump Overvoltage Protection**
  - **Issue:** No quantitative droop table defines how Batteries, Solar MPPT curtailment, Generator, and Grid share the `96 VDC` bus in `DEGRADED_AUTONOMOUS` mode, or how a sudden BMS charge-FET cutoff during high Solar harvest is absorbed without tripping `> 108 V`.
  - **Done when:** Exact droop voltage bands and load-dump mitigation (fast local Solar curtailment bandwidth, bulk bus capacitance sizing, and/or dump-load crowbar) are specified.

- [ ] **Define Inverter `60 Hz` Hardware Sync Architecture & Time-Master Failover**
  - **Issue:** `HRTIM1_SCOUT` outputs a 16-cycle pulse (~`94 ns`), not the `10 µs` zero-cross pulse assumed in older docs ([Inverter_Module_IO_Requirements.md](../hardware/STM32_control_card/Inverter_Module_IO_Requirements.md)), and Time-Master failover if the master inverter trips offline is undefined.
  - **Done when:** Timer/sync pulse format, isolation, PLL phase-alignment tolerance, and glitch-free master election/handoff are specified.

- [ ] **Define Enclosure, Thermal Cooling & Stacked-Busbar Mechanical Standard**
  - **Issue:** High-power modules (e.g., 10 kVA Boost rejecting ~`500 W` at 95% efficiency) need defined airflow paths, heatsink clearance, M4 busbar interconnect geometry, and safe maintenance lockout/tagout.
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

- [ ] **Promote Draft `TELEM_BOOST_SLOW` (`0x310 + Instance`) & Consider System Energy Counters**
  - **Issue:** `TELEM_BOOST_SLOW` (active phase count 1–5, phase-current spread, shedding/rotation status, inductor thermal status) is drafted in [CAN_Messages_Definitions.md](../protocol/CAN_Messages_Definitions.md); energy integration (`kWh` / `Ah` counters + SD persistence) is not yet specified.
  - **Done when:** `TELEM_BOOST_SLOW` fields are reviewed/approved and mirrored into Coordinator, PC simulator, and Nucleo Boost simulator; energy logging strategy is decided.

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

## Completed Items (2026-11-20)

- [x] Documented 5-phase Boost active phase shedding/balancing and drafted `TELEM_BOOST_SLOW` (`0x310 + Instance`) for active phase count and phase-balance diagnostics.
- [x] Authored initial [Fault_Model.md](../architecture/Fault_Model.md) and [Fault_Codes.md](../protocol/Fault_Codes.md) (replacing empty 0-byte stubs).
- [x] Reconciled documentation drift across node definitions, hardware baselines, `Project_Context.md`, and `System_State_MAchine.md` (heartbeat cadences, `96 VDC` `±5%` operating window, `ISO1042DWV` transceiver, Waveshare `1.3"` LCD HAT, and updated control-card contactor pins).
