# Solar Node

## Purpose

The Solar Node provides a standardized renewable energy harvesting interface between individual photovoltaic panels (or strings) and the 96VDC Energy Bus, performing local Maximum Power Point Tracking (MPPT) without requiring local human interaction.

## Responsibilities

- Harvest photovoltaic energy and regulate current injection onto the 96VDC Energy Bus
- Execute local Maximum Power Point Tracking (MPPT)
- Block reverse current flow from the 96VDC Energy Bus into the photovoltaic array
- Report harvested power, panel electrical parameters, and operating status to the Microgrid CAN Network
- Accept remote provisioning, power-stage profile configuration, and firmware updates from the Coordinator

## Not Responsible For

- Regulating overall 96VDC Energy Bus voltage across the system
- Managing battery charging algorithms or energy dispatch priorities
- Providing local physical display or button interfaces

## Interfaces

- Photovoltaic Array Input
- 96VDC Energy Bus
- 30V 50kHz Common Auxiliary Power Bus
- Microgrid CAN Network

## Inputs

- DC power from the connected photovoltaic panel or string
- Control power from the 30V 50kHz Common Auxiliary Power Bus
- Enable, curtailment, and remote configuration commands from the Coordinator

## Outputs

- Regulated DC power to the 96VDC Energy Bus
- Harvest telemetry, serial identification, and fault status to the Microgrid CAN Network

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

- Transmits periodic heartbeats (`500 ms`), permanent serial number, and harvesting telemetry
- Receives remote Node ID assignment/configuration, power-curtailment limits, and future snow-melting commands via the Coordinator web interface
- Continues autonomous MPPT harvesting within safe bus voltage limits if Coordinator communications are lost (`>1500 ms`)

## Fault Conditions

- PV Input Overvoltage / Undervoltage
- 96VDC Bus Overvoltage / Undervoltage
- Output Overcurrent or Reverse-Current Fault
- Enclosure Overtemperature
- Swappable Power Stage ID Mismatch
- Coordinator Communication Timeout

## Open Questions

- Operational trigger conditions and power limits for the provisioned future snow-melting mode
