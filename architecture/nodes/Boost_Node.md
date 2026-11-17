# Boost Node

## Purpose

The Boost Node transfers energy from the 96VDC Energy Bus to the 380VDC Inverter Bus, maintaining a regulated high-voltage DC supply for downstream AC output inverter modules.

## Responsibilities

- Step up energy from the 96VDC Energy Bus to the 380VDC Inverter Bus
- Regulate the 380VDC Inverter Bus within safe operating limits
- Monitor 96VDC input and 380VDC output voltages, phase currents, and thermal limits
- Execute multi-phase interleaving and dynamic light-load phase shedding
- Participate in coordinated load balancing when multiple Boost Nodes operate in parallel
- Report operating status, capacity, and faults to the Microgrid CAN Network

## Not Responsible For

- Managing battery charging or discharging
- Controlling solar harvesting, generator operation, or utility-tie power flow
- Controlling downstream AC inverter waveforms or residential circuit breakers
- Direct cell-level battery monitoring

## Interfaces

- 96VDC Energy Bus
- 380VDC Inverter Bus
- 30V 50kHz Common Auxiliary Power Bus
- Microgrid CAN Network

## Inputs

- Energy from the 96VDC Energy Bus
- Control power from the 30V 50kHz Common Auxiliary Power Bus
- Enable, voltage setpoint, and load-sharing commands from the Coordinator

## Outputs

- Regulated DC power to the 380VDC Inverter Bus
- Status, telemetry, and fault reports to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Safety Responsibilities

- Protect internal power stages from overcurrent, overvoltage, undervoltage, and overtemperature conditions
- Autonomously enter a protective state without Coordinator approval when safety limits are exceeded
- Physically isolate from the 96VDC Energy Bus via main isolation contactors when safe operation cannot be maintained

## Coordinator Interaction

- Transmits periodic heartbeats (`500 ms`) and bus telemetry
- Receives startup, standby, and parallel current-sharing setpoints from the Coordinator
- Transitions to deterministic Primary/Secondary fallback behavior if Coordinator communications are lost (`>1500 ms`)

## Fault Conditions

- 96VDC Bus Undervoltage / Overvoltage
- 380VDC Bus Overvoltage / Undervoltage / Short-Circuit
- Phase Overcurrent / Hardware Overcurrent Trip
- Heatsink or Inductor Overtemperature
- Pre-Charge Timeout or Contactor Fault
- Coordinator Communication Timeout

## Open Questions

- Exact Primary/Secondary bus voltage droop curve during Coordinator-loss degraded mode
