# Solar Node

## Purpose

The Solar Node provides a standardized renewable energy harvesting interface between individual photovoltaic panels (or strings) and the 96VDC Energy Bus, performing local Maximum Power Point Tracking (MPPT) without requiring local human interaction.

## Responsibilities

- Harvest photovoltaic energy and regulate current injection onto the 96VDC Energy Bus
- Execute local Maximum Power Point Tracking (MPPT)
- Block reverse current flow from the 96VDC Energy Bus into the photovoltaic array
- Report harvested power, panel electrical parameters, and operating status to the local Solar gateway segment
- Accept provisioning and control through the local gateway; gateway routing/provisioning payloads remain undefined

## Not Responsible For

- Regulating overall 96VDC Energy Bus voltage across the system
- Managing battery charging algorithms or energy dispatch priorities
- Providing local physical display or button interfaces

## Interfaces

- Photovoltaic Array Input
- 96VDC Energy Bus
- 30V 50kHz Common Auxiliary Power Bus
- Isolated Solar gateway-local CAN segment

## Inputs

- DC power from the connected photovoltaic panel or string
- Control power from the 30V 50kHz Common Auxiliary Power Bus
- Enable, curtailment, and configuration commands routed by the local gateway

## Outputs

- Regulated DC power to the 96VDC Energy Bus
- Harvest telemetry, serial identification, and fault status to the local gateway segment

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Safety Responsibilities

- Protect internal conversion stages from input overvoltage, output overvoltage, overcurrent, and overtemperature
- Prevent reverse current back-feeding during night or shading events
- Autonomously disconnect from the 96VDC Energy Bus without Coordinator approval when safety limits are exceeded

## Coordinator Interaction

- Transmits local-segment heartbeats (`100 ms`), identity at startup/on request, and harvesting telemetry
- Uses a gateway-local slot (`0`–`31`); it does not have a main-bus `GlobalNodeID`
- Receives remote Node ID assignment/configuration, power-curtailment limits, and future snow-melting commands through a gateway; routing and payload formats are not yet defined
- Is monitored by its gateway with a provisional `500 ms` missing-child deadline
- Continues autonomous MPPT harvesting within safe bus voltage limits if Coordinator communications are lost (`>500 ms`)
- See the [Solar Gateway Node design](./Solar_Gateway_Node.md) for the gateway's preliminary scope and communication boundary

## Fault Conditions

- PV Input Overvoltage / Undervoltage
- 96VDC Bus Overvoltage / Undervoltage
- Output Overcurrent or Reverse-Current Fault
- Enclosure Overtemperature
- Swappable Power Stage ID Mismatch
- Coordinator Communication Timeout

## Open Questions

- Operational trigger conditions and power limits for the provisioned future snow-melting mode
