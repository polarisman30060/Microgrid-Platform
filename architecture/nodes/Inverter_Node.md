# Inverter Node

## Purpose

The Inverter Node converts DC power from the 380VDC Inverter Bus into regulated 60Hz residential AC output (`120VAC` single-phase or `240VAC` split-phase), supporting parallel current-mode operation and software-resettable circuit breaker protection.

## Responsibilities

- Convert energy from the 380VDC Inverter Bus into regulated 60Hz AC power
- Synchronize output waveform phase and frequency with parallel Inverter Nodes
- Balance active and reactive current sharing across parallel output modules
- Receive planned `20 ms` current-sharing updates when participating in active parallel operation; non-paralleled operation does not require that cadence. Sharing payload, measurement freshness, and timeout behavior remain to be defined and validated.
- Enforce local inverse-time (`I²t`) overload and sub-microsecond short-circuit breaker protection
- Report AC output telemetry, breaker state, and fault status to the Microgrid CAN Network

## Not Responsible For

- Regulating the 380VDC Inverter Bus or 96VDC Energy Bus
- Managing upstream power generation or battery storage dispatch
- Coordinating system-wide load shedding across independent circuits

## Interfaces

- 380VDC Inverter Bus
- Residential AC Load Distribution Panel (`120VAC` or `240VAC` split-phase)
- Hardware Zero-Crossing Synchronization Line
- 30V 50kHz Common Auxiliary Power Bus
- Microgrid CAN Network

## Inputs

- High-voltage DC power from the 380VDC Inverter Bus
- Hardware synchronization pulses from the designated Time Master
- Control power from the 30V 50kHz Common Auxiliary Power Bus
- Output enable, operating mode, and remote breaker reset commands from the Coordinator

## Outputs

- Regulated 60Hz AC power to residential branch or main circuits
- AC voltage, current, power factor, breaker state, and fault reports to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Safety Responsibilities

- Immediately open physical AC output air-gap contactors upon overload (`I²t`), short-circuit, DC-bus fault, or overtemperature events
- Autonomously enter a protective state without Coordinator approval when safe operating limits are exceeded
- Prevent back-feeding or unsynchronized parallel connection onto live AC lines

## Coordinator Interaction

- Transmits periodic heartbeats (`250 ms`), real-time AC load telemetry (`200 ms`), and circuit-breaker trip states
- Receives output enable commands and operator-initiated software circuit-breaker resets from the Coordinator HMI/web interface
- Maintains autonomous AC output regulation and local breaker protection during Coordinator communication loss (`>500 ms`)

## Fault Conditions

- AC Output Short-Circuit / Instantaneous Overcurrent
- Inverse-Time (`I²t`) Thermal Overload Trip
- 380VDC Bus Undervoltage / Overvoltage
- Phase Synchronization Loss
- Heatsink Overtemperature
- Output Contactor Weld / Feedback Fault
- Coordinator Communication Timeout

## Open Questions

- Master-election handoff behavior if the designated Hardware Time Master inverter trips offline
