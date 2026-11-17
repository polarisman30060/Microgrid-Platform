# Coordinator Node

## Purpose

The Coordinator Node serves as the central supervisory intelligence of the microgrid, orchestrating node discovery, startup sequencing, fleet energy allocation, load balancing, and fault logging across the Microgrid CAN Network.

## Responsibilities

- Discover active nodes on the Microgrid CAN Network and validate against stored configuration
- Detect and report missing nodes or duplicate Node ID conflicts
- Sequence system startup across storage, conversion, and output nodes
- Manage fleet-level battery charging and discharging without conflicting node commands
- Coordinate multi-module load balancing across parallel Boost and Inverter nodes
- Dispatch renewable, grid, and generator power sources according to system energy priority
- Log system events, telemetry, and fault reports to persistent storage

## Not Responsible For

- Real-time sub-millisecond switching control or PWM generation
- Direct cell-level Battery Management System (BMS) monitoring or balancing
- Autonomous module-level hardware safety protection or emergency contactor trips
- Automatic reassignment of conflicting CAN Node IDs

## Interfaces

- Microgrid CAN Network (via galvanic CAN isolator)
- Display Node / Local HMI & Wi-Fi Web Server (co-hosted on the supervisor platform)
- Persistent Non-Volatile Storage
- 30V 50kHz Common Auxiliary Power Bus

## Inputs

- Node heartbeats, serial numbers, telemetry, and fault messages from the Microgrid CAN Network
- Operator configuration, mode commands, and breaker reset requests from the local HMI or web interface
- Un-interruptible control power from the 30V 50kHz Common Auxiliary Power Bus

## Outputs

- Startup, enable, power allocation, and load-sharing commands to the Microgrid CAN Network
- Aggregated system telemetry, topology status, and active alarms to the Display/Web HMI
- Persistent configuration files and timestamped fault logs

## Operating States

- OFF
- STARTUP
- DISCOVERY
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Safety Responsibilities

- Inhibit system startup if any Required node is missing or involved in a duplicate Node ID conflict
- Prevent conflicting fleet commands (such as commanding simultaneous charging and discharging across Battery Nodes)
- Ensure system architecture never relies on Coordinator availability for immediate module-level hardware protection

## Coordinator Interaction

- Acts as the supervisory master polling and orchestrating all functional nodes (`0x100–0x8FF`)
- Monitors `500 ms` node heartbeats and flags nodes offline after `1500 ms` (3 missed heartbeats)

## Fault Conditions

- Required Node Missing at Startup or Runtime
- Duplicate Node ID Conflict Detected
- System-Wide Energy Deficit / Critical Low State of Charge
- CAN Bus Error / Transceiver Fault
- Persistent Storage Read/Write Failure

## Open Questions

- Exact arbitration priority and load-sharing algorithms when multiple heterogeneous energy sources operate simultaneously
