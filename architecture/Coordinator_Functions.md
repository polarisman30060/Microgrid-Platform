# System Coordinator Functions

## 1. Role & Architectural Boundaries
The System Coordinator serves as the supervisory manager, human-machine interface (HMI), and telemetry gateway for the microgrid.

**Strict Real-Time Boundary:** The Coordinator **never** executes fast cycle-by-cycle PWM regulation or primary hardware overcurrent protection. All high-speed voltage/current loops and microsecond-level fault shutdowns reside strictly inside the local node microcontrollers. If the Coordinator reboots or disconnects, the microgrid continues operating safely in `DEGRADED_AUTONOMOUS` mode.

---

## 2. Core Functional Responsibilities

### 2.1 Node Discovery, Registry & Health Monitoring
* Maintains a live registry of all active CAN nodes, tracking firmware versions, node types, current states, and last-seen timestamps.
* Transmits the master `COORD_HEARTBEAT` frame every 100 ms.
* Flags a node as `OFFLINE` if its `NODE_HEARTBEAT` is missed for > 300 ms, adjusting system power budgets accordingly.

### 2.2 Startup Sequencing & Contactor Orchestration
* Enforces strict power-up ordering so downstream stages never draw load before upstream buses are precharged and stabilized:
  1. Verify Auxiliary Lifeline health and absence of system faults.
  2. Precharge and close primary DC energy bus via Battery/Storage node(s).
  3. Soft-start and stabilize HV DC inverter link via Boost Converter node(s).
  4. Enable AC Output module(s) sequentially to prevent simultaneous inrush surges.

### 2.3 Power Dispatch, Current Balancing & Bus Trim
* Aggregates real-time power generation, battery state-of-charge (SoC), and load demand across all modules.
* Broadcasts slow-loop voltage trim offsets and current-sharing setpoints (at 10 Hz – 50 Hz) to eliminate steady-state drift on the DC buses and balance thermal/current loading across parallel phases or modules.
* Manages source priority rules (e.g., Solar MPPT -> Battery Storage -> Backup Generator / Grid Input) based on SoC thresholds and user configuration.

### 2.4 Software Circuit Breaker & Fault Management
* Monitors trip statuses across all AC output modules and DC converters.
* Provides operator-initiated **Software Breaker Reset** commands over CAN after verifying that fault conditions have cleared.
* Logs timestamped pre- and post-fault telemetry snapshots to non-volatile storage for root-cause diagnostics.

### 2.5 Local HMI & Web Diagnostics Interface
* **Local Display:** Renders real-time bus voltages, net power flows (kW), battery SoC, active alarms, and breaker controls on the local touchscreen UI.
* **Wi-Fi Web Portal:** Hosts an embedded web interface (HTTP/WebSocket) providing:
  * Live module-by-module power flow visualization.
  * Remote software circuit breaker trip/reset controls.
  * Configurable SoC reserve targets and generator auto-start thresholds.
  * Downloadable fault and event logs.
