# Grid Interface Node

## Purpose

The Grid Interface Node provides a controlled, power-factor-corrected interface between the external AC utility grid and the 96VDC Energy Bus, enabling grid-assisted battery charging and supplemental energy import.

## Responsibilities

- Synchronize with utility AC line voltage and frequency
- Convert utility AC power onto the 96VDC Energy Bus with active Power Factor Correction (PFC)
- Regulate imported power within limits commanded by the Coordinator
- Monitor utility grid voltage, frequency, and line quality
- Physically isolate from both the AC utility grid and the 96VDC Energy Bus upon faults or grid disturbances

## Not Responsible For

- Determining grid import schedules or battery fleet charging priorities
- Supplying standalone AC loads during a utility grid outage (owned by Inverter Nodes)
- Regulating the 380VDC Inverter Bus

## Interfaces

- External Utility AC Grid (`120VAC` / `240VAC`)
- 96VDC Energy Bus
- 30V 50kHz Common Auxiliary Power Bus
- Microgrid CAN Network

## Inputs

- AC electrical power from the utility grid
- Control power from the 30V 50kHz Common Auxiliary Power Bus
- Enable and import power limit commands from the Coordinator

## Outputs

- Regulated DC power to the 96VDC Energy Bus
- Grid quality metrics, imported power telemetry, and fault status to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Safety Responsibilities

- Immediately disconnect AC utility input and 96VDC bus output contactors upon utility undervoltage, overvoltage, frequency deviation, or anti-islanding trip conditions
- Protect internal power stages from overcurrent and overtemperature
- Autonomously enter a protective state without Coordinator approval when safety limits are exceeded

## Coordinator Interaction

- Transmits periodic heartbeats (`1 s`), utility availability status, and imported power telemetry (`500 ms`)
- Receives grid-import enable commands and current/wattage limits from the Coordinator
- Ceases power import and opens isolation contactors if Coordinator communications are lost (`>500 ms`)

## Fault Conditions

- Utility Grid Overvoltage / Undervoltage / Blackout
- Utility Grid Frequency Out of Range
- Phase-Locked Loop (PLL) Sync Loss
- 96VDC Bus Overvoltage / Undervoltage
- Input / Output Overcurrent
- Heatsink Overtemperature
- Contactor Feedback Fault
- Coordinator Communication Timeout

## Open Questions

- Whether bidirectional grid export support will be added in a future hardware revision
