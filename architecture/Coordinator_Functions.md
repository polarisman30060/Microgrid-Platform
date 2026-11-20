# System Coordinator Functions

## 1. Role & Architectural Boundaries
The System Coordinator serves as the supervisory manager, human-machine interface (HMI), and telemetry gateway for the microgrid.

The CAN timings and frame references below follow the working registry and provisional schedule. Gateway payloads and physical Solar gateway implementation remain open. See [`../docs/Coordinator_CAN_Implementation.md`](../docs/Coordinator_CAN_Implementation.md) before implementing additional CAN application behavior.

**Strict Real-Time Boundary:** The Coordinator **never** executes fast cycle-by-cycle PWM regulation or primary hardware overcurrent protection. All high-speed voltage/current loops and microsecond-level fault shutdowns reside strictly inside the local node microcontrollers. If the Coordinator reboots or disconnects, the microgrid continues operating safely in `DEGRADED_AUTONOMOUS` mode.

---

## 2. Core Functional Responsibilities

### 2.1 Node Discovery, Registry & Health Monitoring
* Maintains a live registry of all active CAN nodes, tracking permanent serial numbers, firmware versions, node types, current states, and last-seen timestamps.
* Transmits the master `COORD_HEARTBEAT` (`0x100`) frame every `100 ms`.
* Flags a node as `OFFLINE` according to its type-specific deadline: Boost/Solar gateway `500 ms`, Battery/Inverter `1 s`, Generator/Grid/Display `3 s`; Solar children are monitored by their gateway at `500 ms`.

### 2.2 Startup Sequencing & Contactor Orchestration
* Enforces strict power-up ordering so downstream stages never draw load before upstream buses are precharged and stabilized:
  1. Verify `30V 50kHz` Auxiliary Lifeline health and absence of system faults.
  2. Precharge and close primary `96VDC` Energy Bus via Battery/Storage node(s) (`ΔV ≤ 1.5V`).
  3. Soft-start and stabilize `380VDC` HV DC inverter link via Boost Converter node(s).
  4. Enable AC Output module(s) sequentially to prevent simultaneous inrush surges.

### 2.3 Power Dispatch, Current Balancing & Bus Trim
* Aggregates real-time power generation, battery state-of-charge (SoC), and load demand across all modules.
* Sends current-balancing updates (`CMD_NODE_CONTROL`, `0x120+`) at `20 ms` only when active parallel Boost balancing requires them. It does not send routine `20 ms` Inverter commands; Inverter commands are on change or at most every `250 ms` when justified.
* Manages source priority rules (Solar MPPT -> Battery Storage -> Backup Generator / Grid Input) based on SoC thresholds and user configuration, while enforcing fleet-wide Battery mode exclusivity (`CHARGING`, `DISCHARGING`, `IDLE`).

### 2.4 Software Circuit Breaker & Fault Management
* Monitors trip statuses across all AC output modules and DC converters.
* Provides operator-initiated **Software Breaker Trip** and **Software Breaker Reset** commands over CAN (`0x120+`) with explicit main-bus `CMD_ACK` (`0x760 + GlobalNodeID - 1`) verification after confirming fault conditions have cleared.
* Logs timestamped pre- and post-fault telemetry snapshots to non-volatile MicroSD storage for root-cause diagnostics.

### 2.5 Local HMI & Web Diagnostics Interface
* **Local Display:** Renders real-time bus voltages, net power flows (kW), battery SoC, active alarms, and breaker controls on the local `7-inch` touchscreen UI.
* **Wi-Fi Web Portal:** Hosts an embedded web interface (HTTP/WebSocket) providing:
  * Live module-by-module power flow visualization.
  * Remote software circuit breaker trip/reset controls.
  * Remote Solar provisioning (`0x7C0`) through the appropriate gateway; child routing and provisioning payload details remain to be defined.
  * Configurable SoC reserve targets and generator auto-start thresholds.
  * Downloadable fault and event logs.
