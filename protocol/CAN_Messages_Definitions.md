# CAN Bus Message & Protocol Specification

Version: 1.0
Status: Draft overall. The configured Boost instance 0 communications profile in Section 3.6.1 is frozen for isolated simulator and compatible-firmware development only. Other profiles and project-wide protocol behavior remain draft; this does not approve electrical behavior or production deployment.

## 1. Physical & Data-Link Layer

* **Protocol Standard:** Classical CAN 2.0B (`11-bit` standard identifiers used for all real-time control, telemetry, discovery, and fault traffic; `29-bit` extended frames reserved strictly for future offline firmware OTA/diagnostics).
* **Controller Compatibility:** All STM32G474QE `FDCAN` peripherals must operate in **Classic CAN mode** (`FDCAN_FRAME_CLASSIC`) to match the Coordinator's Waveshare ESP32-S3 `TWAI` hardware (`GPIO 19` RX, `GPIO 20` TX, enabled via `CH422G EXIO5 = HIGH`).
* **Nominal Bitrate:** `500 kbps` (galvanically isolated CAN transceivers required on all nodes; `120 Ohm` termination at both physical bus ends).
* **Byte Order:** Little-Endian (Intel format) for all multi-byte integers.
* **Maximum Payload:** `8 bytes` per frame.

---

## 2. Identifier Allocation Map (`0x000` – `0x7FF` Standard 11-bit)

| ID Range | Priority | Category | Direction | Cycle Time |
| :--- | :--- | :--- | :--- | :--- |
| `0x001` – `0x01F` | Highest | **Emergency / Hardware Trips** | Any Node -> Broadcast | Event-driven (`< 2 ms`) |
| `0x100` – `0x11F` | Very High | **Coordinator Heartbeat & Global State** | Coordinator -> Broadcast | `100 ms` / Event |
| `0x120` – `0x1FF` | High | **Control Setpoints & Balancing Tokens** | Coordinator -> Specific/Group | `20 ms` (Boost/Inv) / `100 ms` |
| `0x200` – `0x2FF` | Medium-High | **Battery / BMS Node Telemetry** | Battery Nodes -> Broadcast | `50 ms` Fast / `250 ms` Slow |
| `0x300` – `0x3FF` | Medium | **HV Boost Converter Telemetry** | Boost Nodes -> Broadcast | `50 ms` Fast / `250 ms` Slow |
| `0x400` – `0x4FF` | Medium | **Solar MPPT, Generator & Grid Telemetry** | Source Nodes -> Broadcast | `50 ms` Fast / `250 ms` Slow |
| `0x500` – `0x5FF` | Medium-Low | **AC Output / Breaker Module Telemetry** | AC Nodes -> Broadcast | `50 ms` Fast / `250 ms` Slow |
| `0x600` – `0x6FF` | Low | **Node Heartbeats & Local State Status** | All Nodes -> Broadcast | `100 ms` |
| `0x700` – `0x7FF` | Lowest | **Node Identity, Provisioning & Command Acks** | Bidirectional | `1000 ms` / On-demand |

### Node Addressing & Type Codes

* **`Node_Type` Enum:** `0x01`=Battery, `0x02`=Boost, `0x03`=Solar_MPPT, `0x04`=Generator, `0x05`=Grid_Interface, `0x06`=AC_Inverter, `0x07`=Display, `0x08`=Coordinator.
* **`Instance` (`0x0`–`0xF`):** Local unit index within a functional class (e.g., Boost `0` = Primary Boost, Boost `1`–`3` = Secondary Boosts).
* **`GlobalNodeID` (`0x01`–`0x7F`):** Unique system-wide node offset (`(Node_Type << 4) | Instance`) used in `0x120+`, `0x600+`, `0x700+`, and `0x740+` frames so no two nodes ever share a TX identifier.

---

## 3. Core Frame Definitions

### 3.1 `0x001 + (Node_Type & 0x0F)` — `EMERGENCY_FAULT` (DLC: 8, Event-Driven)
Broadcast immediately upon any local hardware fault latch.

* **Byte 0:** `GlobalNodeID` (Identifies exact faulting unit)
* **Byte 1:** `Fault_State` (`0x05` = Recoverable Fault, `0x06` = Latched Lockout)
* **Bytes 2–3 (`uint16`):** `Fault_Bitmask`
  * Bit 0: Hardware Overcurrent (`HRTIM_FLT`)
  * Bit 1: Bus Overvoltage (`96V` or `380V`)
  * Bit 2: Bus Undervoltage (`96V` or `380V`)
  * Bit 3: Over-Temperature Shutdown
  * Bit 4: Contactor Weld / Mirror Feedback Mismatch
  * Bit 5: BMS Critical Cell Alarm / Reverse Current
  * Bit 6: Isolation / Ground Fault / Utility Anti-Islanding Trip
  * Bit 7: Watchdog / Internal MCU / Engine Overspeed Fault
* **Bytes 4–5 (`uint16`):** `Snapshot_Voltage` (`0.1 V/bit`, unsigned)
* **Bytes 6–7 (`int16`):** `Snapshot_Current` (`0.1 A/bit`, signed)

---

### 3.2 `0x100` — `COORD_HEARTBEAT` (DLC: 8, `100 ms` Periodic)
Master keep-alive and global supervisory state broadcast.

* **Byte 0:** `Rolling_Counter` (`0`–`255`, increments every `100 ms`)
* **Byte 1:** `Global_Target_State` (`0x00`=Startup, `0x01`=Discovery, `0x02`=Ready, `0x03`=Bus_Precharge, `0x04`=Running, `0x05`=Degraded, `0x06`=Fault, `0x07`=Lockout)
* **Byte 2:** `System_Flags`
  * Bit 0: Fleet Allow Charge (`1`=Allowed)
  * Bit 1: Fleet Allow Discharge (`1`=Allowed; mutually exclusive with Bit 0 at fleet level)
  * Bit 2: Allow HV 380V Boost
  * Bit 3: Allow AC Inverter Outputs
  * Bit 4: Clear Recoverable Faults Pulse
* **Byte 3:** `Active_Node_Count`
* **Bytes 4–7 (`uint32`):** `System_Uptime_Seconds`

---

### 3.3 `0x120 + GlobalNodeID` — `CMD_NODE_CONTROL` (DLC: 8, `20 ms` Boost/Inv / `100 ms` Others)
Supervisory dispatch setpoints and current-balancing tokens sent by Coordinator to a specific node (`GlobalNodeID = 0x00` for global broadcast).

* **Byte 0:** `Command_Action`
  * `0x00` = No-op / Hold Current Mode
  * `0x01` = Precharge & Close Contactor (`Q_PRE` -> `Q_ISOL` once `ΔV <= 1.5V`)
  * `0x02` = Enable Power Stage / Normal Run
  * `0x03` = Disable Power Stage / Standby (Open `Q_ISOL`)
  * `0x04` = Remote Software Breaker Trip (Inverter / DC Out)
  * `0x05` = Reset Software Breaker / Latched Fault
  * `0x06` = Generator Crank / Start Sequence
  * `0x07` = Generator Cooldown & Stop Sequence
* **Bytes 1–2 (`uint16`):** `Target_Voltage_Setpoint` (`0.05 V/bit`; e.g., `1920` = `96.00 V`, `7600` = `380.00 V`)
* **Bytes 3–4 (`int16`):** `Current_Limit_Or_Balance_Token` (`0.1 A/bit`; for parallel Boost/Inverter nodes, carries the active current-sharing target token updated every `20 ms`; for Battery/Grid/Solar, carries max charge/import/curtailment current)
* **Bytes 5–6 (`int16`):** `Max_Discharge_Or_Secondary_Limit` (`0.1 A/bit`; max discharge current for Battery, or reactive/secondary trim)
* **Byte 7:** `CRC8` (SAE J1850 polynomial `0x1D` over Bytes `0–6`)

---

### 3.4 `0x200 + Instance` — `TELEM_BATTERY_FAST` (DLC: 8, `50 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `Bus_Side_Voltage` (`0.05 V/bit`)
* **Bytes 2–3 (`uint16`):** `Pack_Side_Voltage` (`0.05 V/bit`)
* **Bytes 4–5 (`int16`):** `Pack_Current` (`0.1 A/bit`; positive = discharging to `96V` bus, negative = charging pack)
* **Byte 6 (`uint8`):** `Pack_SoC_Percent` (`0.5 %/bit`; `0`–`200` = `0.0%`–`100.0%`)
* **Byte 7:** `Battery_Mode_And_Contactor_Status`
  * Bits `0–1`: Operating Mode (`0`=Idle, `1`=Charging, `2`=Discharging, `3`=Fault)
  * Bit 2: Main Contactor `Q_ISOL` Closed
  * Bit 3: Precharge `Q_PRE` Closed
  * Bit 4: BMS Cell Balancing Active
  * Bit 5: BMS Charge Allowed
  * Bit 6: BMS Discharge Allowed
  * Bit 7: `30V 50kHz` Auxiliary Lifeline Healthy

---

### 3.5 `0x210 + Instance` — `TELEM_BATTERY_SLOW` (DLC: 8, `250 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `Min_Cell_Voltage_mV` (`1 mV/bit`)
* **Bytes 2–3 (`uint16`):** `Max_Cell_Voltage_mV` (`1 mV/bit`)
* **Byte 4 (`int8`):** `Max_Pack_Temp_C` (`1 °C/bit`, signed `-40` to `+125`)
* **Byte 5 (`int8`):** `Converter_MOSFET_Temp_C` (`1 °C/bit`, signed)
* **Bytes 6–7 (`uint16`):** `Available_Capacity_Ah` (`0.1 Ah/bit`)

---

### 3.6 `0x300 + Instance` — `TELEM_BOOST_FAST` (DLC: 8, `50 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `LV_Input_Voltage` (`0.05 V/bit`)
* **Bytes 2–3 (`uint16`):** `HV_Output_Voltage` (`0.1 V/bit`; `3800` = `380.0 V`)
* **Bytes 4–5 (`int16`):** `Total_Input_Current` (`0.1 A/bit`)
* **Byte 6 (`int8`):** `Max_Stage_Temp_C` (`1 °C/bit`, signed)
* **Byte 7:** `Boost_Phase_And_Role_Status`
  * Bits `0–4`: Active Interleaved Phases (`Stages 1–5`)
  * Bit 5: Main Contactor `Q_ISOL` Closed
  * Bit 6: Role (`1`=Designated Primary Boost, `0`=Secondary Boost)
  * Bit 7: Autonomous Droop Mode Active

### 3.6.1 Frozen Boost Instance 0 Communications Profile

This configured-node profile is frozen for the Nucleo-G474RE communications simulator and compatible Coordinator development. It freezes the wire contract for **Boost instance 0 only**; it does not define or validate power-stage control, protection, precharge physics, or production commissioning. The simulator shall not energize or connect to power-stage hardware.

Profile revision: `0` (frozen 2026-11-19; revision is documentation-only and is not transmitted on CAN).

Boost instance 0 has `GlobalNodeID = (0x02 << 4) | 0 = 0x20` and uses only ordinary microgrid frames:

| Frame | CAN ID | Direction | Period / handling |
| --- | --- | --- | --- |
| `COORD_HEARTBEAT` | `0x100` | Coordinator -> bus | Every `100 ms` |
| `CMD_NODE_CONTROL` | `0x140` | Coordinator -> Boost 0 | Coordinator request; `20 ms` setpoint cadence when active |
| `TELEM_BOOST_FAST` | `0x300` | Boost 0 -> bus | Every `50 ms` |
| `NODE_HEARTBEAT` | `0x620` | Boost 0 -> bus | Every `100 ms` |
| `NODE_IDENTITY` | `0x720` | Boost 0 -> bus | At startup and every `1000 ms` |
| `CMD_ACK` | `0x760` | Boost 0 -> bus | Event-driven for non-no-op control actions |

All frames are standard 11-bit Classical CAN data frames at `500 kbit/s`, with exactly 8 data bytes and little-endian multi-byte fields. Payload layouts and scaling are those in Sections 3.2, 3.3, 3.6, 3.11, 3.12, and 3.13. This profile uses `CMD_ACK = 0x740 + GlobalNodeID`; for Boost 0 this is `0x760` and does not overlap the fixed provisioning identifier `0x780`.

The following command and acknowledgement rules apply to this profile:

* `CMD_NODE_CONTROL` is accepted only at `0x140` as a standard data frame with DLC 8 and a valid CRC-8/SAE-J1850 over bytes 0–6 (`poly=0x1D`, `init=0xFF`, `refin=false`, `refout=false`, `xorout=0xFF`). A bad-CRC frame is dropped with no ACK. The receiver increments its local CRC-error counter.
* Action `0x00` is a no-op/hold-current-mode setpoint update and does not generate an ACK. Actions `0x01`, `0x02`, `0x03`, and `0x05` are supported Boost actions. Actions `0x04`, `0x06`, and `0x07` are not applicable to Boost and receive `Rejected_Invalid_State`.
* Each supported state-changing action produces one `CMD_ACK` with the echoed action, the defined ACK status, current local state, fault summary, and local uptime. The Coordinator allows only one state-changing command in flight to this node and correlates the response by source ID and echoed action. This profile does not specify automatic application-level retries; a missing ACK is an unknown command outcome, not success.
* The simulator may model acceptance, rejection, and resulting telemetry/state for these actions, but must never drive contactors, PWM, or other power-stage controls. Its simulated measurements and state transitions are not evidence of electrical operation or safety.
* Boost 0 is statically configured as `GlobalNodeID 0x20`. Remote provisioning and unconfigured-node discovery are not part of this profile. The simulator does not transmit `EMERGENCY_FAULT`; unique system-wide emergency identifiers remain unresolved.
* For the simulator identity frame, `Power_Stage_Profile_ID = 0x00` means no physical power stage is attached. Capability flags are zero. The serial number must be a configured, unique 32-bit test serial; it must not be copied to another simulated node.

This profile does not freeze other Boost instances, the global emergency-ID allocation, remote provisioning, project-wide retry/version rules, or electrical state-machine behavior. Changes to these require corresponding updates to this specification, the Coordinator, simulator firmware, and conformance tests.

---

### 3.7 `0x400 + Instance` — `TELEM_SOLAR_MPPT` (DLC: 8, `100 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `PV_Input_Voltage` (`0.05 V/bit`)
* **Bytes 2–3 (`uint16`):** `PV_Input_Current` (`0.05 A/bit`)
* **Bytes 4–5 (`uint16`):** `Bus_Output_Current` (`0.05 A/bit` injected onto `96V` bus)
* **Byte 6 (`int8`):** `Enclosure_Temp_C` (`1 °C/bit`, signed)
* **Byte 7:** `MPPT_Status_Flags`
  * Bits `0–1`: Mode (`0`=Off/Night, `1`=MPPT_Tracking, `2`=CV_Bus_Curtailment, `3`=Snow_Melt_Reserved)
  * Bit 2: `96V` Output Contactor/FET Connected
  * Bit 3: Curtailment Active

---

### 3.8 `0x410 + Instance` — `TELEM_GENERATOR` (DLC: 8, `50 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `Bus_Output_Voltage` (`0.05 V/bit`)
* **Bytes 2–3 (`int16`):** `Bus_Current` (`0.1 A/bit`; positive = generating to `96V` bus, negative = cranking draw)
* **Bytes 4–5 (`uint16`):** `Engine_RPM` (`1 RPM/bit`)
* **Byte 6 (`int8`):** `Alternator_Converter_Temp_C` (`1 °C/bit`, signed)
* **Byte 7:** `Generator_Mode_And_Alarms`
  * Bits `0–2`: Operating Mode (`0`=Standby, `1`=Cranking, `2`=Warmup, `3`=Generating, `4`=Cooldown, `5`=Fault)
  * Bit 3: Main Contactor `Q_ISOL` Closed
  * Bit 4: Low Oil Pressure Alarm
  * Bit 5: Low Fuel Warning
  * Bit 6: Crank Timeout / Start Fail

---

### 3.9 `0x420 + Instance` — `TELEM_GRID_INTERFACE` (DLC: 8, `50 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `Grid_AC_RMS_Voltage` (`0.1 V/bit`)
* **Bytes 2–3 (`int16`):** `DC_Bus_Output_Current` (`0.1 A/bit` onto `96V` bus)
* **Byte 4 (`uint8`):** `Grid_Frequency_Hz` (`0.1 Hz/bit` offset from `50 Hz`; `100` = `60.0 Hz`)
* **Byte 5 (`int8`):** `Heatsink_Temp_C` (`1 °C/bit`, signed)
* **Byte 6:** `Grid_Status_Flags`
  * Bit 0: Utility Grid Present & Within Window
  * Bit 1: PLL Phase Locked
  * Bit 2: AC Input Contactor Closed
  * Bit 3: `96V` Output Contactor Closed
  * Bit 4: Anti-Islanding / Grid Fault Trip
* **Byte 7 (`uint8`):** `Power_Factor_Percent` (`0`–`100`%)

---

### 3.10 `0x500 + Instance` — `TELEM_AC_OUTPUT_FAST` (DLC: 8, `50 ms` Periodic)

* **Bytes 0–1 (`uint16`):** `AC_RMS_Voltage` (`0.1 V/bit`; `1200` = `120.0 V`, `2400` = `240.0 V`)
* **Bytes 2–3 (`uint16`):** `AC_RMS_Current` (`0.05 A/bit`; `400` = `20.00 A`)
* **Bytes 4–5 (`int16`):** `Active_Power_Watts` (`1 W/bit`, signed)
* **Byte 6 (`uint8`):** `AC_Frequency_Hz` (`0.1 Hz/bit` offset from `50 Hz`; e.g., `100` = `60.0 Hz`)
* **Byte 7:** `Breaker_And_Sync_Status`
  * Bits `0–2`: `Breaker_Status` (`0`=Open, `1`=Closed/Normal, `2`=Tripped_Overload_I2t, `3`=Tripped_ShortCircuit, `4`=Remote_Tripped, `5`=Locked_Out)
  * Bit 3: Split-Phase `240V` Mode (`0`=`120V` Single-Phase, `1`=`240V` Split-Phase)
  * Bit 4: Hardware Zero-Cross Sync Locked
  * Bit 5: Designated Sync Time Master

---

### 3.11 `0x600 + GlobalNodeID` — `NODE_HEARTBEAT` (DLC: 8, `100 ms` Periodic)

* **Byte 0:** `Rolling_Counter` (`0`–`255`)
* **Byte 1:** `Node_Type`
* **Byte 2:** `Current_Local_State` (`0x00`=Off, `0x01`=Startup, `0x02`=Ready, `0x03`=Running, `0x04`=Degraded_Autonomous, `0x05`=Fault, `0x06`=Lockout)
* **Byte 3:** `Active_Warning_Flags` (Bit 0: Thermal Derating, Bit 1: CAN Error Passive, Bit 2: Sensor/Token Drift, Bit 3: Coordinator Timeout)
* **Bytes 4–5 (`uint16`):** `Aux_Supply_Voltage_mV` (`1 mV/bit`; monitors local isolated rail health)
* **Bytes 6–7 (`uint16`):** `Firmware_Build_ID`

---

### 3.12 `0x700 + GlobalNodeID` — `NODE_IDENTITY` (DLC: 8, `1000 ms` Periodic & On-Demand)
Used for commissioning discovery, duplicate `GlobalNodeID` conflict detection, and remote Solar MPPT identification.

* **Bytes 0–3 (`uint32`):** `Permanent_Serial_Number` (Unique hardware serial number assigned at manufacture/flash)
* **Byte 4:** `Node_Type`
* **Byte 5:** `Configured_Instance_ID` (`0x0`–`0xF`)
* **Byte 6:** `Power_Stage_Profile_ID` (Identifies swappable power stage hardware variant, e.g., Solar MPPT panel profile or Inverter 120V/240V variant)
* **Byte 7:** `Capability_Flags` (Bit 0: Has Local OLED UI, Bit 1: Primary Role Capable, Bit 2: Remote ID Provisionable)

---

### 3.13 `0x740 + GlobalNodeID` — `CMD_ACK` (DLC: 8, Event-Driven Response)
Explicit node response to state-changing `CMD_NODE_CONTROL` or `CMD_REMOTE_PROVISION` commands.

* **Byte 0:** `Echoed_Command_Action`
* **Byte 1:** `Ack_Status` (`0x00`=Accepted, `0x01`=Rejected_Invalid_State, `0x02`=Rejected_Local_Fault, `0x03`=Rejected_Precharge_DeltaV, `0x04`=Rejected_CRC_Error)
* **Byte 2:** `Current_Local_State`
* **Byte 3:** `Active_Fault_Summary`
* **Bytes 4–7 (`uint32`):** `Local_Uptime_ms`

---

### 3.14 `0x780` — `CMD_REMOTE_PROVISION` (DLC: 8, On-Demand)
Sent by Coordinator during commissioning to assign a new `Instance_ID` to an unconfigured or inaccessible module (such as a roof-mounted Solar MPPT) matched by its `Permanent_Serial_Number`.

* **Bytes 0–3 (`uint32`):** `Target_Permanent_Serial_Number`
* **Byte 4:** `New_Instance_ID` (`0x0`–`0xF`)
* **Byte 5:** `New_Stage_Profile_ID` (`0xFF` = No change)
* **Byte 6:** `Provision_Action` (`0x01` = Commit to Non-Volatile Flash & Reboot CAN ID)
* **Byte 7:** `CRC8` (SAE J1850 polynomial `0x1D` over Bytes `0–6`)

---

## 4. Timeout, Safety & Bus-Off Recovery Rules

1. **Heartbeat Watchdog (`500 ms`):** Missing `COORD_HEARTBEAT` (`0x100`) for `> 500 ms` (5 consecutive missed beats) triggers immediate local transition into `DEGRADED_AUTONOMOUS` mode.
2. **Balancing Token Timeout (`100 ms`):** If an active Boost or Inverter node misses `CMD_NODE_CONTROL` balancing tokens for `> 100 ms` while `COORD_HEARTBEAT` is still active, it holds its last valid current setpoint and raises Bit 2 (`Sensor/Token Drift`) in `NODE_HEARTBEAT` Byte 3.
3. **Command Integrity (`CRC8`):** Any `CMD_NODE_CONTROL` (`0x120+`) or `CMD_REMOTE_PROVISION` (`0x780`) frame failing its Byte 7 `CRC8` check is silently dropped, increments the local `CAN_RX_CRC_Error` counter, and returns `Ack_Status = 0x04` on `CMD_ACK` (`0x740+`).
4. **CAN Bus-Off Recovery:** All nodes must enable automatic hardware/software Bus-Off recovery with a `100 ms` backoff retry timer so transient EMI spikes never permanently deafen a power module.

## 5. Unresolved Items Before Implementation

- Review and freeze a one-Battery Node profile for PC-to-Coordinator simulation using ordinary node frames; do not introduce simulator-only CAN identifiers.
- `EMERGENCY_FAULT` currently uses `0x001 + Node_Type`; multiple nodes of the same type therefore share an arbitration identifier. Define unique IDs that preserve the required fault priority.
- `CMD_ACK` currently uses `0x740 + GlobalNodeID`, which overlaps fixed provisioning ID `0x780` for some node IDs.
- Define the identity/discovery identifier and payload for a node that has not yet been assigned a `GlobalNodeID`, and confirm the valid ID range for each node type.
- The CRC-error rule says to silently drop an invalid command and also return an error acknowledgement; choose one behavior and define how it can be addressed safely.
- Define ACK correlation, retries, duplicate-command handling, and protocol version compatibility.

The Boost 0 profile above locally resolves CRC-error handling and ACK correlation for that configured node. The listed system-wide issues remain open and must be resolved before generalizing the profile or enabling broader production application behavior.

Do not treat unresolved frame assignments or illustrative examples as approved command behavior.
