# CAN Bus Message & Protocol Specification

Version: 1.1
Status: Working CAN ID registry and provisional timing schedule approved for implementation. Payloads for Solar gateway routing/aggregation, local Solar emergency reporting, provisioning discovery, and several application behaviors remain draft. Boost 0 has been reopened and is no longer frozen. This does not approve electrical behavior or production deployment.

## 1. Physical & Data-Link Layer

* **Protocol Standard:** Classical CAN 2.0B (`11-bit` standard identifiers used for all real-time control, telemetry, discovery, and fault traffic; `29-bit` extended frames reserved strictly for future offline firmware OTA/diagnostics).
* **Controller Compatibility:** All STM32G474QE `FDCAN` peripherals must operate in **Classic CAN mode** (`FDCAN_FRAME_CLASSIC`) to match the Coordinator's Waveshare ESP32-S3 `TWAI` hardware (`GPIO 19` RX, `GPIO 20` TX, enabled via `CH422G EXIO5 = HIGH`).
* **Nominal Bitrate:** `500 kbps` (galvanically isolated CAN transceivers required on all nodes; `120 Ohm` termination at both physical bus ends).
* **Byte Order:** Little-Endian (Intel format) for all multi-byte integers.
* **Maximum Payload:** `8 bytes` per frame.

---

## 2. Identifier Allocation Map (`0x000` – `0x7FF` Standard 11-bit)

| ID Range | Priority | Category | Direction | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `0x001` – `0x047` | Highest | **Main-bus Emergency / Hardware Faults** | Main-bus nodes -> Broadcast | ID is the sending node's `GlobalNodeID` |
| `0x048` – `0x0FF` | Highest | **Reserved emergency IDs** | Reserved | |
| `0x100` | Very High | **Coordinator Heartbeat & Global State** | Coordinator -> Broadcast | `100 ms` |
| `0x120` | High | **Broadcast Control** | Coordinator -> Broadcast | Payload/action use remains draft |
| `0x121` – `0x167` | High | **Addressed Main-bus Control** | Coordinator -> Node | `0x120 + GlobalNodeID` |
| `0x168` – `0x19F` | High | **Reserved control IDs** | Reserved | |
| `0x1A0` – `0x1AF` | High | **Solar gateway child-control routing** | Coordinator -> Gateway | Payload is not yet defined |
| `0x1B0` – `0x1BF` | High | **Solar gateway routed replies** | Gateway -> Coordinator | Reply/ACK payload is not yet defined |
| `0x1C0` – `0x1FF` | High | **Reserved control IDs** | Reserved | |
| `0x200` – `0x2FF` | Medium-High | **Battery / BMS telemetry** | Battery Nodes -> Broadcast | Fast `100 ms`, slow `1 s` |
| `0x300` – `0x3FF` | Medium | **Boost telemetry** | Boost Nodes -> Broadcast | Fast `50 ms`, slow `1 s` |
| `0x400` – `0x4FF` | Medium | **Source and Solar gateway telemetry** | Nodes -> Broadcast | Solar child IDs are local to isolated segments |
| `0x500` – `0x5FF` | Medium-Low | **Inverter telemetry** | Inverter Nodes -> Broadcast | `200 ms` |
| `0x600` – `0x6FF` | Low | **Node Heartbeats** | Nodes -> Broadcast | Type-specific period; see Section 4 |
| `0x700` – `0x7FF` | Lowest | **Identity, Provisioning & ACKs** | Bidirectional | Startup/on-request identity; ACK on state change |

The allocation table describes the main CAN bus unless explicitly marked as gateway-local. Solar child identifiers may numerically overlap main-bus identifiers because each Solar segment is physically isolated and raw child frames are not transparently bridged.

### Node Addressing & Type Codes

* **`Node_Type` Enum:** `0x01`=Battery, `0x02`=Boost, `0x03`=Solar_MPPT, `0x04`=Generator, `0x05`=Grid_Interface, `0x06`=AC_Inverter, `0x07`=Display, `0x08`=Coordinator, `0x09`=Solar_Segment_Gateway.
* **`Instance`:** Registry-local index, zero-based; it is not packed with `Node_Type` to form an address.
* **Main-bus `GlobalNodeID` allocations:**

| Node type | Instances | GlobalNodeID range |
| --- | ---: | --- |
| Battery | 0–4 | `0x01`–`0x05` |
| Boost | 0–4 | `0x06`–`0x0A` |
| Generator | 0–1 | `0x0B`–`0x0C` |
| Grid Interface | 0–1 | `0x0D`–`0x0E` |
| Inverter | 0–39 | `0x0F`–`0x36` |
| Display | 0 | `0x37` |
| Solar Segment Gateway | 0–15 | `0x38`–`0x47` |

`0x48`–`0x7F` remain reserved. Solar MPPT children do not have a main-bus `GlobalNodeID`: each uses a gateway instance and local slot `0`–`31`, with local CAN address `slot + 1`. Sixteen gateways provide 512 local slots for the 500-Solar-node target. Battery counts above five require a future integration layer and are not allocated here. The Coordinator retains type `0x08`.

---

## 3. Core Frame Definitions

### 3.1 Main-bus `GlobalNodeID` — `EMERGENCY_FAULT` (DLC: 8, Event-Driven)
On the main bus, the identifier equals the sending node's `GlobalNodeID` (`0x001`–`0x047`) and is broadcast immediately upon a local hardware fault latch. Gateway-local Solar emergency IDs have not yet been assigned. If a gateway forwards a child fault upstream, its own unique emergency ID identifies the gateway; the exact child-slot payload encoding remains open.

* **Byte 0:** `GlobalNodeID` (Identifies exact faulting unit)
* **Byte 1:** `Fault_State` (`0x05` = Recoverable Fault, `0x06` = Latched Lockout)
* **Bytes 2–3 (`uint16`):** `Fault_Bitmask`
  * Bit 0: Hardware Overcurrent (`HRTIM_FLT`)
  * Bit 1: Bus Overvoltage (`96V` or `400V`)
  * Bit 2: Bus Undervoltage (`96V` or `400V`)
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
  * Bit 2: Allow HV 400V Boost
  * Bit 3: Allow AC Inverter Outputs
  * Bit 4: Clear Recoverable Faults Pulse
* **Byte 3:** `Active_Node_Count`
* **Bytes 4–7 (`uint32`):** `System_Uptime_Seconds`

---

### 3.3 Main-bus `0x120 + GlobalNodeID` — `CMD_NODE_CONTROL` (DLC: 8)
Supervisory dispatch setpoints sent by the Coordinator to one main-bus node. `0x120` is reserved for broadcast control; addressed node IDs are `0x121`–`0x167`. Solar children use `0x120 + LocalAddress` only on their isolated gateway segment. Solar child-control routing through gateway IDs `0x1A0`–`0x1AF` is reserved; its compact payload and paired reply behavior are not defined yet.

The provisional command schedule is event/on-change based for ordinary nodes. Active parallel Boost and Inverter modules require current-sharing updates at `20 ms`. Non-paralleled Inverter commands are sent on change, or at most every `250 ms` when justified; they do not require routine `20 ms` updates. The Inverter sharing profile, telemetry freshness, and total bus capacity remain to be defined and validated.

* **Byte 0:** `Command_Action`
  * `0x00` = No-op / Hold Current Mode
  * `0x01` = Precharge & Close Contactor (`Q_PRE` -> `Q_ISOL` once `ΔV <= 1.5V`)
  * `0x02` = Enable Power Stage / Normal Run
  * `0x03` = Disable Power Stage / Standby (Open `Q_ISOL`)
  * `0x04` = Remote Software Breaker Trip (Inverter / DC Out)
  * `0x05` = Reset Software Breaker / Latched Fault
  * `0x06` = Generator Crank / Start Sequence
  * `0x07` = Generator Cooldown & Stop Sequence
* **Bytes 1–2 (`uint16`):** `Target_Voltage_Setpoint` (`0.05 V/bit`; e.g., `1920` = `96.00 V`, `8000` = `400.00 V`)
* **Bytes 3–4 (`int16`):** `Current_Limit_Or_Balance_Token` (`0.1 A/bit`; proposed for parallel Boost/Inverter current-sharing targets at `20 ms`; for Battery/Grid/Solar, carries max charge/import/curtailment current). Inverter target meaning and payload semantics remain to be validated.
* **Bytes 5–6 (`int16`):** `Max_Discharge_Or_Secondary_Limit` (`0.1 A/bit`; max discharge current for Battery, or reactive/secondary trim)
* **Byte 7:** `CRC8` (SAE J1850 polynomial `0x1D` over Bytes `0–6`)

---

### 3.4 `0x200 + Instance` — `TELEM_BATTERY_FAST` (DLC: 8, `100 ms` Periodic)

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
  * Bit 7: `24 VDC` Auxiliary Lifeline (`AUX_LOGIC_24V`) Healthy

---

### 3.5 `0x210 + Instance` — `TELEM_BATTERY_SLOW` (DLC: 8, `1 s` Periodic)

* **Bytes 0–1 (`uint16`):** `Min_Cell_Voltage_mV` (`1 mV/bit`)
* **Bytes 2–3 (`uint16`):** `Max_Cell_Voltage_mV` (`1 mV/bit`)
* **Byte 4 (`int8`):** `Max_Pack_Temp_C` (`1 °C/bit`, signed `-40` to `+125`)
* **Byte 5 (`int8`):** `Converter_MOSFET_Temp_C` (`1 °C/bit`, signed)
* **Bytes 6–7 (`uint16`):** `Available_Capacity_Ah` (`0.1 Ah/bit`)

---

### 3.6 `0x300 + Instance` — `TELEM_BOOST_FAST` (DLC: 8, `50 ms` Periodic)

Each Boost Node is a **10 kW 2-phase (`2 × 5 kW`) interleaved Full-Bridge LLC (FB-LLC) resonant converter** stepping `96 VDC` up to an isolated, earth-referenced **`400 VDC` (`±200 VDC` split-rail)** inverter bus. Bytes `0–6` and Byte `7` Bits `5–7` preserve layout compatibility while Byte `7` Bits `0–4` report 2-phase FB-LLC status:

* **Bytes 0–1 (`uint16`):** `LV_Input_Voltage` (`0.05 V/bit`; e.g., `1920` = `96.00 V`)
* **Bytes 2–3 (`uint16`):** `HV_Output_Voltage` (`0.1 V/bit`; total `HV+` to `HV-`, e.g., `4000` = `400.0 V`)
* **Bytes 4–5 (`int16`):** `Total_Input_Current` (`0.1 A/bit`)
* **Byte 6 (`int8`):** `Max_Stage_Temp_C` (`1 °C/bit`, signed; max of primary MOSFET heatsink / magnetics / rectifier)
* **Byte 7:** `Boost_LLC_Status`
  * Bits `0–1`: Active LLC Phase Mask (`Bit 0` = Phase 1 Active, `Bit 1` = Phase 2 Active; `0x00`=Off, `0x01`=Ph1 Only, `0x02`=Ph2 Only, `0x03`=Both `90°` Interleaved)
  * Bits `2–3`: LLC Operating Mode (`0`=Standby/Off, `1`=SoftStart_PSM, `2`=Normal_PFM, `3`=Burst_LightLoad)
  * Bit 4: Split-Bus Rail Balance OK (`1`=Balanced within `±10 V`, `0`=Imbalance Warning)
  * Bit 5: Main Contactor `Q_ISOL` Closed
  * Bit 6: Role (`1`=Designated Primary Boost, `0`=Secondary Boost)
  * Bit 7: Autonomous Droop Mode Active

### 3.6.0 `0x310 + Instance` — `TELEM_BOOST_SLOW` (DLC: 8, `1 s` Periodic)

Mirrors Battery's `0x200`/`0x210` fast/slow split (`0x310–0x314` for Boost instances `0–4`), carrying slower FB-LLC resonant-tank, split-rail balance, and thermal diagnostics:

* **Bytes 0–1 (`uint16`):** `Switching_Freq_10Hz` (`10 Hz/bit`; e.g., `4500` = `45.00 kHz` nominal resonant frequency `f_r`, `0` when standby/off)
* **Bytes 2–3 (`int16`):** `Split_Rail_Imbalance_0p1V` (`0.1 V/bit`, signed: `V_HV_POS - |V_HV_NEG|`; e.g., `+15` = `+1.5 V`)
* **Byte 4 (`uint8`):** `Phase1_Tank_Current_RMS` (`0.5 A/bit`; `0`–`127.5 A` primary resonant tank RMS current)
* **Byte 5 (`uint8`):** `Phase2_Tank_Current_RMS` (`0.5 A/bit`; `0`–`127.5 A` primary resonant tank RMS current)
* **Byte 6 (`int8`):** `Magnetics_Or_Rectifier_Temp_C` (`1 °C/bit`, signed; `-128` = sensor not fitted / unavailable)
* **Byte 7:** `LLC_Diagnostic_Flags`
  * Bits `0–1`: Active Phase Count (`0`, `1`, or `2`)
  * Bit 2: Active PSM Phase-Balance Trim Engaged (`1`=Active)
  * Bit 3: Magnetics Overtemperature Snap-Disc Alarm (`1`=Tripped/Warning)
  * Bit 4: Thermal Derating Active
  * Bit 5: Capacitive-Mode / ZVS Margin Warning (`f_sw` near lower clamp)
  * Bit 6: Phase Current Imbalance Warning (`|I_ph1 - I_ph2|` exceeded threshold)
  * Bit 7: Reserved (`0`)

### 3.6.1 Reopened Boost Instance 0 Communications Profile

Boost instance 0 was previously treated as frozen; that decision has been reopened to apply the system-wide registry and the 2-phase FB-LLC telemetry update (`400 VDC` split-rail). This section records its current working IDs and draft behavior; it is not a frozen wire contract and does not define or validate power-stage control, protection, precharge physics, or production commissioning.

Profile revision: `2` (working registry + 2-phase FB-LLC telemetry; documentation-only, not transmitted on CAN).

Boost instance 0 has registry-assigned `GlobalNodeID = 0x06` and uses ordinary microgrid frames:

| Frame | CAN ID | Direction | Period / handling |
| --- | --- | --- | --- |
| `COORD_HEARTBEAT` | `0x100` | Coordinator -> bus | Every `100 ms` |
| `CMD_NODE_CONTROL` | `0x126` | Coordinator -> Boost 0 | `20 ms` only during active parallel balancing |
| `TELEM_BOOST_FAST` | `0x300` | Boost 0 -> bus | Every `50 ms` |
| `TELEM_BOOST_SLOW` | `0x310` | Boost 0 -> bus | Every `1 s` |
| `NODE_HEARTBEAT` | `0x606` | Boost 0 -> bus | Every `100 ms` |
| `NODE_IDENTITY` | `0x706` | Boost 0 -> bus | At startup/on request |
| `CMD_ACK` | `0x765` | Boost 0 -> bus | Event-driven for non-no-op control actions |

All frames are standard 11-bit Classical CAN data frames at `500 kbit/s`, with exactly 8 data bytes and little-endian multi-byte fields. Payload layouts and scaling are those in Sections 3.2, 3.3, 3.6, 3.11, 3.12, and 3.13. Main-bus ACK IDs use `0x760 + GlobalNodeID - 1`.

The following command and acknowledgement rules apply to this profile:

* `CMD_NODE_CONTROL` is accepted only at `0x126` as a standard data frame with DLC 8 and a valid CRC-8/SAE-J1850 over bytes 0–6 (`poly=0x1D`, `init=0xFF`, `refin=false`, `refout=false`, `xorout=0xFF`). A bad-CRC frame is dropped with no ACK. The receiver increments its local CRC-error counter.
* Action `0x00` is a no-op/hold-current-mode setpoint update and does not generate an ACK. Actions `0x01`, `0x02`, `0x03`, and `0x05` are supported Boost actions. Actions `0x04`, `0x06`, and `0x07` are not applicable to Boost and receive `Rejected_Invalid_State`.
* Each supported state-changing action produces one `CMD_ACK` with the echoed action, the defined ACK status, current local state, fault summary, and local uptime. The Coordinator allows only one state-changing command in flight to this node and correlates the response by source ID and echoed action. This profile does not specify automatic application-level retries; a missing ACK is an unknown command outcome, not success.
* The simulator may model acceptance, rejection, and resulting telemetry/state for these actions, but must never drive contactors, PWM, or other power-stage controls. Its simulated measurements and state transitions are not evidence of electrical operation or safety.
* Boost 0 is statically configured as `GlobalNodeID 0x06`. Remote provisioning and unconfigured-node discovery are not part of this profile. The simulator does not transmit `EMERGENCY_FAULT`.
* For the simulator identity frame, `Power_Stage_Profile_ID = 0x00` means no physical power stage is attached. Capability flags are zero. The serial number must be a configured, unique 32-bit test serial; it must not be copied to another simulated node.

This working profile does not freeze other Boost instances, project-wide retry/version rules, or electrical state-machine behavior. Changes require corresponding updates to this specification, the Coordinator, simulator firmware, and conformance tests.

---

### 3.7 Gateway-local `0x400 + SolarLocalSlot` — `TELEM_SOLAR_MPPT` (DLC: 8, `100 ms` Periodic)

This identifier is used only on the Solar gateway's isolated child segment. `SolarLocalSlot` is `0`–`31`; on that segment it may numerically overlap a main-bus identifier. A gateway must aggregate/filter child traffic and must not transparently bridge raw Solar frames to the main bus.

* **Bytes 0–1 (`uint16`):** `PV_Input_Voltage` (`0.05 V/bit`)
* **Bytes 2–3 (`uint16`):** `PV_Input_Current` (`0.05 A/bit`)
* **Bytes 4–5 (`uint16`):** `Bus_Output_Current` (`0.05 A/bit` injected onto `96V` bus)
* **Byte 6 (`int8`):** `Enclosure_Temp_C` (`1 °C/bit`, signed)
* **Byte 7:** `MPPT_Status_Flags`
  * Bits `0–1`: Mode (`0`=Off/Night, `1`=MPPT_Tracking, `2`=CV_Bus_Curtailment, `3`=Snow_Melt_Reserved)
  * Bit 2: `96V` Output Contactor/FET Connected
  * Bit 3: Curtailment Active

---

### 3.8 `0x410 + Instance` — `TELEM_GENERATOR` (DLC: 8, `500 ms` Periodic)

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

### 3.9 `0x420 + Instance` — `TELEM_GRID_INTERFACE` (DLC: 8, `500 ms` Periodic)

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

### 3.10 `0x500 + Instance` — `TELEM_AC_OUTPUT_FAST` (DLC: 8, `200 ms` Periodic)

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

  ### 3.10.1 Solar Segment Gateway Summary and Detail IDs (Payloads TBD)

  For gateway instance `g` (`0`–`15`), reserve these main-bus identifiers. Both summary frames are sent in each `500 ms` summary interval:

  | Frame | CAN ID | Provisional cadence |
  | --- | --- | --- |
  | Primary gateway summary | `0x430 + g` | One frame per `500 ms` summary interval |
  | Secondary gateway summary | `0x440 + g` | One frame per `500 ms` summary interval |
  | On-demand Solar child detail | `0x450 + g` | On request or paced rotation |

  These are reserved assignments only. Summary fields, scaling, child-detail encoding, freshness semantics, and gateway command/reply payloads still require design. Child faults should be forwarded promptly using the gateway's unique main-bus emergency ID; the child-slot payload is not yet specified.

  ---

  ### 3.11 `0x600 + GlobalNodeID` — `NODE_HEARTBEAT` (DLC: 8, Type-Specific Periodic)

  Main-bus IDs are `0x601`–`0x647`. Solar children use `0x600 + LocalAddress` (`0x601`–`0x620`) on their isolated segment.

* **Byte 0:** `Rolling_Counter` (`0`–`255`)
* **Byte 1:** `Node_Type`
* **Byte 2:** `Current_Local_State` (`0x00`=Off, `0x01`=Startup, `0x02`=Ready, `0x03`=Running, `0x04`=Degraded_Autonomous, `0x05`=Fault, `0x06`=Lockout)
* **Byte 3:** `Active_Warning_Flags` (Bit 0: Thermal Derating, Bit 1: CAN Error Passive, Bit 2: Sensor/Token Drift, Bit 3: Coordinator Timeout)
* **Bytes 4–5 (`uint16`):** `Aux_Supply_Voltage_mV` (`1 mV/bit`; monitors local isolated rail health)
* **Bytes 6–7 (`uint16`):** `Firmware_Build_ID`

---

### 3.12 `0x700 + Address` — `NODE_IDENTITY` (DLC: 8, Startup/On-Request)
Main-bus nodes use `0x700 + GlobalNodeID` (`0x701`–`0x747`). Solar children use `0x700 + LocalAddress` (`0x701`–`0x720`) only on their isolated segment. Identity is sent at startup and on request; the request mechanism remains to be specified.

* **Bytes 0–3 (`uint32`):** `Permanent_Serial_Number` (Unique hardware serial number assigned at manufacture/flash)
* **Byte 4:** `Node_Type`
* **Byte 5:** `Configured_Instance_ID` (node-type-specific; Solar local slot `0`–`31`)
* **Byte 6:** `Power_Stage_Profile_ID` (Identifies swappable power stage hardware variant, e.g., Solar MPPT panel profile or Inverter 120V/240V variant)
* **Byte 7:** `Capability_Flags` (Bit 0: Has Local OLED UI, Bit 1: Primary Role Capable, Bit 2: Remote ID Provisionable)

---

### 3.13 Main-bus `0x760 + GlobalNodeID - 1` — `CMD_ACK` (DLC: 8, Event-Driven Response)
Main-bus ACK IDs are `0x760`–`0x7A6` for state-changing addressed commands. Solar child ACKs use `0x740 + LocalAddress` (`0x741`–`0x760`) only on the local segment. Gateway-routed replies use reserved IDs `0x1B0`–`0x1BF`; their payload and relationship to a child ACK are not yet specified. Provisioning acknowledgement/confirmation behavior is also not defined.

* **Byte 0:** `Echoed_Command_Action`
* **Byte 1:** `Ack_Status` (`0x00`=Accepted, `0x01`=Rejected_Invalid_State, `0x02`=Rejected_Local_Fault, `0x03`=Rejected_Precharge_DeltaV, `0x04`=Rejected_CRC_Error)
* **Byte 2:** `Current_Local_State`
* **Byte 3:** `Active_Fault_Summary`
* **Bytes 4–7 (`uint32`):** `Local_Uptime_ms`

---

### 3.14 `0x7C0` — `CMD_REMOTE_PROVISION` (DLC: 8, On-Demand; Payload Draft)
The identifier remains reserved for Coordinator provisioning. The intended Solar workflow is for a gateway to match a child by serial number and assign a local slot; routing/discovery behavior for an unconfigured child is not yet defined.

* **Bytes 0–3 (`uint32`):** `Target_Permanent_Serial_Number`
* **Byte 4:** `New_Instance_ID` (node-type-specific; Solar local slot `0`–`31`)
* **Byte 5:** `New_Stage_Profile_ID` (`0xFF` = No change)
* **Byte 6:** `Provision_Action` (`0x01` = Commit to Non-Volatile Flash & Reboot CAN ID)
* **Byte 7:** `CRC8` (SAE J1850 polynomial `0x1D` over Bytes `0–6`)

---

## 4. Timeout, Safety & Bus-Off Recovery Rules

### Provisional traffic schedule

| Node/message | Cadence |
| --- | --- |
| Coordinator heartbeat | `100 ms` |
| Boost heartbeat / fast + slow telemetry | `100 ms` / `50 ms` + `1 s`; `20 ms` commands only when active parallel balancing requires them |
| Battery heartbeat / fast + slow telemetry | `250 ms` / `100 ms` + `1 s` |
| Inverter heartbeat / telemetry | `250 ms` / `200 ms`; `20 ms` current-sharing updates during active parallel operation; otherwise commands on change or at most every `250 ms` when justified |
| Generator and Grid heartbeat / telemetry | `1 s` / `500 ms`; important state/fault events immediate |
| Display heartbeat | `1 s` |
| Solar child heartbeat / telemetry | `100 ms` on the local gateway segment |
| Solar gateway heartbeat / summary | `100 ms` / two summary frames every `500 ms`; child faults promptly; child details on request or paced rotation |
| Identity / ACK | Identity at startup/on request; ACK only for state-changing commands |

### Watchdogs and bus behavior

1. **Coordinator watchdog (`500 ms`):** Missing `COORD_HEARTBEAT` (`0x100`) for more than `500 ms` triggers the node's locally defined degraded behavior.
2. **Missing-node deadlines:** Boost and Solar gateways `500 ms` (Solar children are monitored by their gateway at `500 ms`); Battery and Inverter `1 s`; Generator, Grid Interface, and Display `3 s`.
3. **Balancing-token timeout (`100 ms`, provisional for Boost):** If an active Boost misses a required balancing update while the Coordinator heartbeat remains active, it holds its last valid local setpoint and raises the applicable warning. This timeout still needs validation. The Inverter balancing-update timeout and safe behavior on stale or missing updates remain undefined.
4. **Command integrity (`CRC8`):** Invalid commands are not applied. ACK behavior for malformed/CRC-invalid commands remains profile-specific and must not contradict a module's safety behavior.
5. **CAN Bus-Off Recovery:** Nodes should use a bounded recovery strategy; the current Nucleo simulators use a `100 ms` retry backoff.
6. **Bus utilization target:** Aim for approximately `40%` sustained main-bus utilization and allow short peaks around `60%`. Validate with actual payloads, gateway summary rates, and worst-case events before treating this as a verified capacity.

## 5. Unresolved Items Before Implementation

- Define the compact 8-byte gateway child-control payload, routed reply/ACK semantics, and command correlation.
- Define the Solar gateway summary and child-detail payload fields, scaling, freshness, and update behavior.
- Assign local Solar child emergency identifiers and define how the gateway identifies the originating child in a forwarded main-bus fault.
- Define unconfigured Solar child discovery and identity-request behavior, including the provisioning route to the correct gateway.
- Select the Solar gateway hardware/interface and verify local-segment isolation and capacity; no physical gateway firmware target is specified yet.
- Validate main-bus utilization against the traffic schedule, full 500-child Solar target, and worst-case fault/command events.
- Define and validate the Inverter current-sharing payload, feedback freshness and cadence, missed-update behavior, and parallel-group scaling. Check whether the existing `200 ms` Inverter telemetry can support the `20 ms` update requirement and include concurrent parallel groups in the bus-load analysis.
- Define protocol version compatibility, retry behavior, duplicate-command handling, and per-profile malformed-command ACK behavior.
- Confirm Battery-to-DALY BMS model/interface/protocol separately; the DALY link is local to the Battery module and is not specified by this system CAN registry.

The main-bus CAN ID registry and node quotas above are approved working assignments. Reserved gateway IDs do not imply that their payloads or physical gateway implementation are complete. Do not treat unresolved payload assignments or illustrative electrical examples as approved command behavior.

The preliminary Solar Gateway scope, two-segment routing boundary, hardware direction, and failure behavior are documented in the [Solar Gateway Node design](../architecture/nodes/Solar_Gateway_Node.md). That design does not define gateway payload bytes or authorize transparent forwarding.
