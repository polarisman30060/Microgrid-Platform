# CAN Bus Message & Protocol Specification

## 1. Physical & Data-Link Layer
* **Protocol Standard:** CAN 2.0B (11-bit standard identifiers used for primary real-time traffic; 29-bit extended frames reserved for firmware OTA/diagnostics).
* **Nominal Bitrate:** 500 kbps (isolated CAN transceivers required on all nodes, 120 Ohm termination at both physical bus ends).
* **Byte Order:** Little-Endian (Intel format) for all multi-byte integers.
* **Maximum Payload:** 8 bytes per frame.

---

## 2. Identifier Allocation Map (`0x000` – `0x7FF` Standard 11-bit)
*(Note: Standard 11-bit CAN IDs span `0x000`–`0x7FF`. Extended IDs `0x800+` utilize the 29-bit IDE flag).*

| ID Range | Priority | Category | Direction | Cycle Time |
| :--- | :--- | :--- | :--- | :--- |
| `0x001` – `0x01F` | Highest | **Emergency / Hardware Trips** | Any Node -> Broadcast | Event-driven (Immediate) |
| `0x100` – `0x11F` | Very High | **Coordinator Heartbeat & Global State** | Coordinator -> Broadcast | 100 ms / Event |
| `0x120` – `0x1FF` | High | **Control Setpoints & Trim Commands** | Coordinator -> Specific/Group | 20 ms – 100 ms |
| `0x200` – `0x2FF` | Medium-High | **Battery / BMS Node Telemetry** | Battery Nodes -> Broadcast | 50 ms / 250 ms |
| `0x300` – `0x3FF` | Medium | **HV Boost Converter Telemetry** | Boost Nodes -> Broadcast | 50 ms / 250 ms |
| `0x400` – `0x4FF` | Medium | **Solar MPPT & Generator Telemetry** | Source Nodes -> Broadcast | 100 ms / 500 ms |
| `0x500` – `0x5FF` | Medium-Low | **AC Output / Breaker Module Telemetry** | AC Nodes -> Broadcast | 50 ms / 250 ms |
| `0x600` – `0x6FF` | Low | **Node Heartbeats & Health Status** | All Nodes -> Broadcast | 100 ms |
| `0x700` – `0x7FF` | Lowest | **Configuration, Diagnostics & Reset Ack** | Bidirectional | On-demand |

**Node ID Encoding:** Within each functional block, the lower 4 bits (`0x..0` to `0x..F`) identify the physical `Node_Instance_ID` (`0`–`15`) configured via hardware address pins or non-volatile flash.

---

## 3. Core Frame Definitions

### 3.1 `0x001 + NodeID` — `EMERGENCY_FAULT` (DLC: 8, Event-Driven)
Broadcast immediately upon any hardware fault latch.
* **Byte 0:** `Node_Type` (`1`=Battery, `2`=Boost, `3`=MPPT, `4`=Gen, `5`=AC_Out)
* **Byte 1:** `Fault_State` (`0x06` = Latched Fault)
* **Bytes 2–3 (`uint16`):** `Fault_Bitmask`
  * Bit 0: Hardware Overcurrent (`HRTIM_FLT`)
  * Bit 1: Primary Bus Overvoltage
  * Bit 2: Primary Bus Undervoltage
  * Bit 3: Over-Temperature Shutdown
  * Bit 4: Contactor Weld / Feedback Mismatch
  * Bit 5: BMS Critical Cell Alarm
  * Bit 6: Isolation / Ground Fault
  * Bit 7: Watchdog / Internal MCU Fault
* **Bytes 4–5 (`uint16`):** `Snapshot_Voltage` (0.1 V/bit, unsigned)
* **Bytes 6–7 (`int16`):** `Snapshot_Current` (0.1 A/bit, signed)

---

### 3.2 `0x100` — `COORD_HEARTBEAT` (DLC: 8, 100 ms Periodic)
Master keep-alive and global system mode broadcast.
* **Byte 0:** `Rolling_Counter` (`0`–`255`, increments every frame to detect frozen transmitters)
* **Byte 1:** `Global_Target_State` (`0x00`=Init, `0x01`=Standby, `0x02`=Precharge, `0x03`=Normal_Run, `0x05`=Shutdown)
* **Byte 2:** `System_Flags` (Bit 0: Allow Charge, Bit 1: Allow Discharge, Bit 2: Allow AC Output, Bit 3: Clear Latched Faults)
* **Byte 3:** `Active_Node_Count`
* **Bytes 4–7 (`uint32`):** `System_Uptime_Seconds`

---

### 3.3 `0x120 + NodeID` — `CMD_NODE_CONTROL` (DLC: 8, 50 ms Periodic / Event)
Supervisory dispatch setpoints sent by Coordinator to a target node (`NodeID = 0x0` addresses all nodes of a group if needed, `0x1`–`0xF` addresses specific unit).
* **Byte 0:** `Command_Action` (`0`=No-op/Hold, `1`=Enable Power Stage, `2`=Disable Power Stage, `3`=Reset Breaker/Fault, `4`=Close Precharge)
* **Bytes 1–2 (`uint16`):** `Target_Voltage_Setpoint` (0.05 V/bit; e.g., `1920` = 96.00 V, `7600` = 380.00 V)
* **Bytes 3–4 (`int16`):** `Max_Charge_Or_Input_Current` (0.1 A/bit)
* **Bytes 5–6 (`int16`):** `Max_Discharge_Or_Output_Current` (0.1 A/bit)
* **Byte 7:** `CRC8` (SAE J1850 polynomial `0x1D` over Bytes 0–6 for command integrity)

---

### 3.4 `0x200 + Instance` — `TELEM_BATTERY_FAST` (DLC: 8, 50 ms Periodic)
* **Bytes 0–1 (`uint16`):** `Bus_Side_Voltage` (0.05 V/bit)
* **Bytes 2–3 (`uint16`):** `Pack_Side_Voltage` (0.05 V/bit)
* **Bytes 4–5 (`int16`):** `Pack_Current` (0.1 A/bit; positive = discharging to bus, negative = charging pack)
* **Byte 6 (`uint8`):** `Pack_SoC_Percent` (0.5 %/bit; `0`–`200` = 0.0%–100.0%)
* **Byte 7:** `Contactor_And_BMS_Status` (Bit 0: Main Contactor Closed, Bit 1: Precharge Closed, Bit 2: Balancing Active, Bit 3: Charge Allowed, Bit 4: Discharge Allowed)

### 3.5 `0x210 + Instance` — `TELEM_BATTERY_SLOW` (DLC: 8, 250 ms Periodic)
* **Bytes 0–1 (`uint16`):** `Min_Cell_Voltage_mV` (1 mV/bit)
* **Bytes 2–3 (`uint16`):** `Max_Cell_Voltage_mV` (1 mV/bit)
* **Byte 4 (`int8`):** `Max_Pack_Temp_C` (1 °C/bit, signed `-40` to `+125`)
* **Byte 5 (`int8`):** `Converter_MOSFET_Temp_C` (1 °C/bit, signed)
* **Bytes 6–7 (`uint16`):** `Available_Capacity_Ah` (0.1 Ah/bit)

---

### 3.6 `0x300 + Instance` — `TELEM_BOOST_FAST` (DLC: 8, 50 ms Periodic)
* **Bytes 0–1 (`uint16`):** `LV_Input_Voltage` (0.05 V/bit)
* **Bytes 2–3 (`uint16`):** `HV_Output_Voltage` (0.1 V/bit; `3800` = 380.0 V)
* **Bytes 4–5 (`int16`):** `Total_Input_Current` (0.1 A/bit)
* **Byte 6 (`int8`):** `Max_Stage_Temp_C` (1 °C/bit, signed)
* **Byte 7 (`uint8`):** `Active_Interleaved_Phases_Bitmask` (Bits 0–4 represent Stages 1–5 active status)

---

### 3.7 `0x500 + Instance` — `TELEM_AC_OUTPUT_FAST` (DLC: 8, 50 ms Periodic)
* **Bytes 0–1 (`uint16`):** `AC_RMS_Voltage` (0.1 V/bit; `1200` = 120.0 V, `2400` = 240.0 V)
* **Bytes 2–3 (`uint16`):** `AC_RMS_Current` (0.05 A/bit; `400` = 20.00 A)
* **Bytes 4–5 (`int16`):** `Active_Power_Watts` (1 W/bit, signed)
* **Byte 6 (`uint8`):** `AC_Frequency_Hz` (0.1 Hz/bit offset from 50 Hz; e.g., `100` = 60.0 Hz)
* **Byte 7 (`uint8`):** `Breaker_Status` (`0`=Open, `1`=Closed/Normal, `2`=Tripped_Overload, `3`=Tripped_ShortCircuit, `4`=Locked_Out)

---

### 3.8 `0x600 + GlobalNodeID` — `NODE_HEARTBEAT` (DLC: 8, 100 ms Periodic)
* **Byte 0:** `Rolling_Counter` (`0`–`255`)
* **Byte 1:** `Node_Type`
* **Byte 2:** `Current_Local_State` (`0x00`–`0x06`)
* **Byte 3:** `Active_Warning_Flags` (Thermal derating, CAN error passive, sensor drift)
* **Bytes 4–5 (`uint16`):** `Aux_Supply_Voltage_mV` (1 mV/bit; monitors local regulated rail health)
* **Bytes 6–7 (`uint16`):** `Firmware_Build_ID`

---

## 4. Timeout & Bus-Off Recovery Rules
1. **Heartbeat Watchdog:** Missing `COORD_HEARTBEAT` for > 500 ms triggers immediate local entry into `DEGRADED_AUTONOMOUS` mode.
2. **Command Integrity:** Any `CMD_NODE_CONTROL` frame failing its Byte 7 `CRC8` check is silently dropped and increments the local `CAN_RX_CRC_Error` counter.
3. **CAN Bus-Off Recovery:** Nodes must enable automatic hardware/software Bus-Off recovery with a 100 ms backoff retry timer so transient EMI spikes do not permanently deafen a power module.
