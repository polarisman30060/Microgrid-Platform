# Boost Node

## Purpose

The Boost Node is a **5-phase interleaved boost converter** with **active phase shedding** and **active per-phase current balancing**. It transfers energy from the 96VDC Energy Bus to the 380VDC Inverter Bus, maintaining a regulated high-voltage DC supply for downstream AC output inverter modules.

## Responsibilities

- Step up energy from the 96VDC Energy Bus to the 380VDC Inverter Bus across five interleaved switching phases
- Regulate the 380VDC Inverter Bus within safe operating limits
- Monitor 96VDC input and 380VDC output voltages, per-phase currents, and thermal limits
- Execute multi-phase interleaving, **active per-phase current balancing**, and **active light-load phase shedding** (dynamically enabling/disabling phases and re-indexing phase angles based on load)
- Participate in coordinated load balancing when multiple Boost Nodes operate in parallel
- Report operating status, capacity, slow diagnostic telemetry (including **active phase count**), and faults to the Microgrid CAN Network

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

- Transmits periodic heartbeats (`100 ms`), fast bus telemetry (`50 ms`), and planned **slow telemetry** (e.g., `1 s` cadence) reporting **how many phases are currently active** (1–5) along with phase-shedding/balancing status and thermal/health diagnostics
- Receives startup, standby, and parallel current-sharing setpoints from the Coordinator
- Transitions to deterministic Primary/Secondary fallback behavior if Coordinator communications are lost (`>500 ms`)

## Fault Conditions

- 96VDC Bus Undervoltage / Overvoltage
- 380VDC Bus Overvoltage / Undervoltage / Short-Circuit
- Phase Overcurrent / Hardware Overcurrent Trip
- Heatsink or Inductor Overtemperature
- Pre-Charge Timeout or Contactor Fault
- Coordinator Communication Timeout

## Open Questions

- Exact Primary/Secondary bus voltage droop curve during Coordinator-loss degraded mode
