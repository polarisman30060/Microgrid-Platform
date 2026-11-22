# Battery Node

## Purpose

The Battery Node provides a standardized, **galvanically isolated** energy storage interface between a battery system (`48V` nominal pack + internal DALY BMS) and the negative-grounded `96VDC Energy Bus`. Internally, it coordinates energy storage, cell-level protection, and isolated bidirectional power conversion (`Dual Active Bridge`) as a single unified network node.

## Responsibilities

- Exchange energy with the 96VDC Energy Bus
- Supply un-interruptible control power to the Split Diode-OR'd `24VDC` Auxiliary Power Bus (`AUX_LOGIC_24V` / `AUX_COIL_24V`)
- Protect the connected battery system
- Control battery charging and discharging
- Determine and report battery operating limits
- Estimate and report battery state

## Not Responsible For

- Determine fleet charging or discharging strategy
- Manage or control other Battery Nodes
- Manage generation sources or system loads

## Interfaces

- 96VDC Energy Bus
- Split `24VDC` Common Auxiliary Power Bus (`AUX_LOGIC_24V` / `AUX_COIL_24V`)
- Microgrid CAN Network

## Inputs

- Power requests from the Coordinator
- Energy from the 96VDC Energy Bus

## Outputs

- Energy to the 96VDC Energy Bus
- Control power to the Split `24VDC` Common Auxiliary Power Bus (`48 V → 24 V` isolated + Ideal-Diode OR)
- Battery operating limits to the Microgrid CAN Network
- Battery state information to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Operating Modes

- IDLE
- CHARGING
- DISCHARGING

## Safety Responsibilities

- Protect the connected battery system from operating outside safe limits
- Enter a protective state without Coordinator approval
- Isolate the battery system from the microgrid when safe operation cannot be maintained

## Coordinator Interaction

- Transmits periodic heartbeats (`250 ms`) and fast (`100 ms`) / slow (`1 s`) battery telemetry
- Receives startup, standby, fleet mode, and current-limit setpoints from the Coordinator
- Transitions to autonomous `96VDC` V-I droop regulation (holding `AUX_LOGIC_24V` active) if Coordinator communications are lost (`>500 ms`)
## Fault Handling Behavior

- The Battery Node shall autonomously enter a protective state when a safety limit is exceeded.
