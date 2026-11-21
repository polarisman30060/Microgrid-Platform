# Microgrid Fault Codes & Flag Definitions

Version: 0.1 (Draft)  
Status: Draft fault registry aligned with [`CAN_Messages_Definitions.md`](./CAN_Messages_Definitions.md) v1.1 and [`../architecture/Fault_Model.md`](../architecture/Fault_Model.md). Bits `0–7` of `EMERGENCY_FAULT` match the working CAN definitions; extended bitmask bits (`8–15`) and 16-bit detailed diagnostic fault codes are provisional reservations for future firmware and Coordinator fault logging.

---

## 1. `EMERGENCY_FAULT` (`0x001–0x047`) Wire Bitmask (`Bytes 2–3`, `uint16`)

Every main-bus `EMERGENCY_FAULT` frame carries a 16-bit `Fault_Bitmask` along with `Fault_State` (`0x05` = Recoverable `FAULT`, `0x06` = Latched `LOCKOUT`), `Snapshot_Voltage` (`0.1 V/bit`), and `Snapshot_Current` (`0.1 A/bit`).

### Working Bits `0–7` (Defined in `CAN_Messages_Definitions.md` Section 3.1)

| Bit | Mask | Fault Class | Default Severity | Typical Trigger Examples |
| ---: | :--- | :--- | :--- | :--- |
| **0** | `0x0001` | Hardware Overcurrent (`HRTIM_FLT`) | `LOCKOUT` | Phase/inductor OC comparator trip, gate-driver desat/fault, inverter hard short-circuit |
| **1** | `0x0002` | Bus Overvoltage (`96 V` or `380 V`) | `FAULT` / `LOCKOUT` | `96 V` bus `> 108.0 V`, `380 V` bus `> 420.0 V`, load-dump transient trip |
| **2** | `0x0004` | Bus Undervoltage (`96 V` or `380 V`) | `FAULT` | `96 V` bus `< 84.0 V`, `380 V` bus `< 340.0 V`, brownout under load |
| **3** | `0x0008` | Over-Temperature Shutdown | `FAULT` | Heatsink, MOSFET, alternator, battery pack, or inductor snap-disc overtemp |
| **4** | `0x0010` | Contactor Weld / Mirror Feedback Mismatch | `LOCKOUT` | `Q_PRE` or `Q_ISOL` auxiliary feedback disagrees with commanded state; precharge timeout |
| **5** | `0x0020` | BMS Critical Cell Alarm / Reverse Current | `LOCKOUT` | DALY BMS cell OV/UV/OT/OC cutoff, BMS comms lost while active, Solar/Gen reverse current |
| **6** | `0x0040` | Isolation / Ground Fault / Utility Anti-Islanding | `LOCKOUT` | DC/AC ground-fault leakage trip, utility voltage/frequency out-of-window, anti-islanding trip |
| **7** | `0x0080` | Watchdog / Internal MCU / Engine Overspeed | `LOCKOUT` | MCU watchdog reset, clock/ADC failure, internal rail fault, engine overspeed/low oil pressure |

### Provisional Extended Bits `8–15` (Draft Reservations)

| Bit | Mask | Provisional Fault Class | Default Severity | Intended Use |
| ---: | :--- | :--- | :--- | :--- |
| **8** | `0x0100` | Inverse-Time (`I²t`) Overload Trip | `LOCKOUT` (Breaker) | Inverter software circuit-breaker thermal overload trip (requires operator reset) |
| **9** | `0x0200` | Phase / Sync Loss | `FAULT` | Inverter zero-cross sync loss, Grid Interface PLL unlock |
| **10** | `0x0400` | Precharge Timeout / `ΔV` Interlock Fail | `FAULT` | Bus voltage failed to equalize (`ΔV ≤ 1.5 V`) within precharge timer |
| **11** | `0x0800` | Engine Start / Prime Mover Fault | `FAULT` / `LOCKOUT` | Generator crank timeout, stall, low oil pressure, coolant overtemp |
| **12** | `0x1000` | Phase Imbalance / Power-Stage Asymmetry | `FAULT` | Excessive Boost 5-phase current spread or 240 V split-phase neutral overload |
| **13** | `0x2000` | Configuration / Profile Mismatch | `LOCKOUT` | Swappable power-stage ID mismatch, invalid commissioned config, duplicate Node ID |
| **14** | `0x4000` | Gateway Local-Segment Fault | `FAULT` | Solar Gateway local CAN bus-off or child emergency forwarded (child-slot encoding TBD) |
| **15** | `0x8000` | External Hardware E-Stop / Interlock Open | `LOCKOUT` | Hardwired E-Stop or host-board hardware safety interlock opened |

---

## 2. Heartbeat Warning Flags & Command ACK Status Codes

### `NODE_HEARTBEAT` Byte 3 — `Active_Warning_Flags` (Tier 0 Warnings)

| Bit | Mask | Meaning |
| ---: | :--- | :--- |
| **0** | `0x01` | Thermal Derating Active |
| **1** | `0x02` | CAN Error Passive / Bus-Off Recovery Occurred |
| **2** | `0x04` | Sensor / Balancing Token Drift or Timeout |
| **3** | `0x08` | Coordinator Heartbeat Timeout (`> 500 ms`, `DEGRADED_AUTONOMOUS` active) |
| **4–7** | `0xF0` | Reserved (`0`) |

### `CMD_ACK` Byte 1 — `Ack_Status`

| Value | Enum Name | Meaning |
| :--- | :--- | :--- |
| `0x00` | `Accepted` | Command accepted and applied/initiated |
| `0x01` | `Rejected_Invalid_State` | Action not valid for this node type or current local state |
| `0x02` | `Rejected_Local_Fault` | Node is in `FAULT` or `LOCKOUT` and cannot accept the command |
| `0x03` | `Rejected_Precharge_DeltaV` | Precharge/close rejected because `ΔV > 1.5 V` |
| `0x04` | `Rejected_CRC_Error` | Command CRC-8 validation failed (profile-dependent; Battery/Boost 0 drop invalid CRC without ACK) |

---

## 3. Detailed 16-Bit Diagnostic Fault Code Registry (Draft — Logging & HMI)

For Coordinator fault logs, local module LCD displays, and future diagnostic detail frames, detailed fault codes are partitioned into 256-code subsystem blocks:

| Code Range | Subsystem |
| :--- | :--- |
| `0x0000` | No Fault |
| `0x0100–0x01FF` | Common Power-Stage, Control Card & CAN Faults |
| `0x0200–0x02FF` | Battery & DALY BMS Faults |
| `0x0300–0x03FF` | 5-Phase Boost Converter Faults |
| `0x0400–0x04FF` | Solar MPPT & Solar Segment Gateway Faults |
| `0x0500–0x05FF` | Integrated Starter-Generator Faults |
| `0x0600–0x06FF` | Grid Interface / Utility-Tie PFC Faults |
| `0x0700–0x07FF` | AC Inverter & Software Circuit Breaker Faults |
| `0x0800–0x08FF` | Coordinator, Topology & HMI/Storage Faults |

### Initial Code Assignments (Draft)

| Code | Name | Mapped Wire Bit | Default Severity | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0x0000` | `FLT_NONE` | — | None | No active fault |
| `0x0101` | `FLT_COM_HRTIM_HW_OC` | Bit 0 | `LOCKOUT` | Asynchronous `HRTIM1_FLT` hardware overcurrent or gate-driver trip |
| `0x0102` | `FLT_COM_BUS96_OV` | Bit 1 | `FAULT` | `96 VDC` energy bus exceeded upper fault limit (`> 108.0 V`) |
| `0x0103` | `FLT_COM_BUS96_UV` | Bit 2 | `FAULT` | `96 VDC` energy bus dropped below lower fault limit (`< 84.0 V`) |
| `0x0104` | `FLT_COM_HEATSINK_OT` | Bit 3 | `FAULT` | Power-stage heatsink exceeded shutdown temperature |
| `0x0105` | `FLT_COM_CONTACTOR_WELD` | Bit 4 | `LOCKOUT` | Contactor mirror feedback indicates welded or stuck-open contact |
| `0x0106` | `FLT_COM_PRECHARGE_TIMEOUT` | Bit 10 | `FAULT` | Precharge failed to reach `ΔV ≤ 1.5 V` within allowed window |
| `0x0107` | `FLT_COM_AUX_SUPPLY_FAULT` | Bit 7 | `FAULT` | Local `24 V` / `5 V` / `3.3 V` or isolated CAN rail out of specification |
| `0x0108` | `FLT_COM_MCU_WATCHDOG` | Bit 7 | `LOCKOUT` | Local MCU independent watchdog or clock security system trip |
| `0x0109` | `FLT_COM_ESTOP_OPEN` | Bit 15 | `LOCKOUT` | Hardwired Emergency Stop / host hardware safety interlock open |
| `0x0201` | `FLT_BAT_BMS_CELL_OV` | Bit 5 | `LOCKOUT` | DALY BMS reported critical cell overvoltage |
| `0x0202` | `FLT_BAT_BMS_CELL_UV` | Bit 5 | `LOCKOUT` | DALY BMS reported critical cell undervoltage |
| `0x0203` | `FLT_BAT_BMS_PACK_OT` | Bit 3 | `FAULT` | Battery pack temperature exceeded safe charge/discharge limit |
| `0x0204` | `FLT_BAT_BMS_COMMS_LOST` | Bit 5 | `FAULT` | Local STM32 lost communication with internal DALY BMS |
| `0x0205` | `FLT_BAT_LIFELINE_FAULT` | Bit 7 | `LOCKOUT` | Internal `30 V 50 kHz` auxiliary lifeline inverter fault |
| `0x0301` | `FLT_BST_BUS380_OV` | Bit 1 | `LOCKOUT` | `380 VDC` inverter link exceeded trip bound (`> 420.0 V`) |
| `0x0302` | `FLT_BST_BUS380_UV` | Bit 2 | `FAULT` | `380 VDC` inverter link dropped below trip bound (`< 340.0 V`) |
| `0x0303` | `FLT_BST_PHASE_OC` | Bit 0 | `LOCKOUT` | One or more of the 5 interleaved boost phases triggered `ACS37002` OC trip |
| `0x0304` | `FLT_BST_INDUCTOR_SNAP_OT` | Bit 3 | `FAULT` | Boost phase inductor overtemperature snap disc opened |
| `0x0305` | `FLT_BST_PHASE_IMBALANCE` | Bit 12 | `FAULT` | Per-phase current spread exceeded balance limit while active |
| `0x0401` | `FLT_SOL_PV_OV` | Bit 1 | `FAULT` | Photovoltaic input voltage exceeded power-card maximum rating |
| `0x0402` | `FLT_SOL_REVERSE_CURRENT` | Bit 5 | `LOCKOUT` | Reverse current detected from `96 VDC` bus toward PV panel |
| `0x0403` | `FLT_SOL_STAGE_ID_MISMATCH` | Bit 13 | `LOCKOUT` | Swappable Solar power-stage profile ID does not match configuration |
| `0x0404` | `FLT_SGW_LOCAL_BUS_OFF` | Bit 14 | `FAULT` | Solar Segment Gateway local CAN segment entered Bus-Off |
| `0x0501` | `FLT_GEN_CRANK_TIMEOUT` | Bit 11 | `FAULT` | Engine failed to reach ignition/self-sustaining RPM within crank limit |
| `0x0502` | `FLT_GEN_OVERSPEED` | Bit 7 | `LOCKOUT` | Engine RPM exceeded overspeed limit |
| `0x0503` | `FLT_GEN_LOW_OIL_PRESSURE` | Bit 11 | `LOCKOUT` | Engine oil pressure dropped below safe running threshold |
| `0x0504` | `FLT_GEN_COOLANT_OT` | Bit 3 | `FAULT` | Engine coolant temperature exceeded safe running threshold |
| `0x0601` | `FLT_GRD_AC_WINDOW_FAULT` | Bit 6 | `FAULT` | Utility AC voltage or frequency outside permitted window |
| `0x0602` | `FLT_GRD_PLL_UNLOCK` | Bit 9 | `FAULT` | Utility-tie PFC phase-locked loop lost lock |
| `0x0603` | `FLT_GRD_ANTI_ISLAND` | Bit 6 | `LOCKOUT` | Utility anti-islanding or ground-fault disconnect triggered |
| `0x0701` | `FLT_INV_SHORT_CIRCUIT` | Bit 0 | `LOCKOUT` | Instantaneous AC output short-circuit software breaker trip |
| `0x0702` | `FLT_INV_OVERLOAD_I2T` | Bit 8 | `LOCKOUT` | Inverse-time (`I²t`) thermal overload software breaker trip |
| `0x0703` | `FLT_INV_SYNC_LOSS` | Bit 9 | `FAULT` | Parallel Inverter lost hardware zero-cross synchronization |
| `0x0704` | `FLT_INV_NEUTRAL_IMBALANCE` | Bit 12 | `FAULT` | 240 V split-phase active neutral current exceeded safe limit |
| `0x0705` | `FLT_INV_GFCI_LEAKAGE` | Bit 6 | `LOCKOUT` | AC output ground-fault / residual current trip |
| `0x0801` | `FLT_CRD_REQ_NODE_MISSING` | — | `FAULT` | Commissioned `Required` node missing past discovery or runtime deadline |
| `0x0802` | `FLT_CRD_DUPLICATE_ID` | Bit 13 | `LOCKOUT` | Duplicate `GlobalNodeID` conflict detected on main CAN bus |
| `0x0803` | `FLT_CRD_STORAGE_FAIL` | — | `Warning` / `FAULT` | Coordinator persistent storage (NVS/MicroSD) read/write failure |
| `0x0804` | `FLT_CRD_CAN_BUS_OFF` | — | `FAULT` | Coordinator TWAI controller entered Bus-Off |

---

## 4. Open Items

- Confirm whether `Active_Fault_Summary` in `CMD_ACK` Byte 3 carries the low byte (`Bits 0–7`) of `Fault_Bitmask` across all node profiles.
- Define an optional diagnostic query/response frame (or slow fault-code telemetry field) if nodes need to transmit the full 16-bit detailed code over CAN in addition to the `EMERGENCY_FAULT` bitmask.
- Align Solar child local emergency identifiers and forwarded gateway fault payloads once Solar Gateway wire frames are specified.
