# Inverter Node

## Purpose

The Inverter Node converts DC power from the earth-referenced `400VDC` (`±200VDC` split-rail) Inverter Bus into regulated `60 Hz` residential AC output (`120VAC` single-phase or `120/240VAC` split-phase) referenced to the Earth-bonded split-capacitor midpoint (`HV_MID = N = PE`), supporting parallel current-mode operation and software-resettable circuit breaker protection.

## Responsibilities

- Convert energy from the `400VDC` (`±200VDC` split-rail) Inverter Bus into regulated `60 Hz` AC power using half-bridge output legs referenced to `HV_MID` (`N/PE`)
- Synchronize output waveform phase and frequency with parallel Inverter Nodes
- Balance active and reactive current sharing across parallel output modules
- Receive planned `20 ms` current-sharing updates when participating in active parallel operation; non-paralleled operation does not require that cadence. Sharing payload, measurement freshness, and timeout behavior remain to be defined and validated.
- Enforce local inverse-time (`I²t`) overload, sub-microsecond short-circuit breaker protection, and AC ground-fault (GFCI/RCD) monitoring
- Report AC output telemetry, breaker state, and fault status to the Microgrid CAN Network

## Not Responsible For

- Regulating the `400VDC` Inverter Bus or `96VDC` Energy Bus
- Managing upstream power generation or battery storage dispatch
- Coordinating system-wide load shedding across independent circuits

## Interfaces

- `400VDC` (`±200VDC` Split-Rail) Inverter Bus (`HV+`, `HV_MID`, `HV-`)
- Residential AC Load Distribution Panel (`120VAC` single-phase or `120/240VAC` split-phase, with Neutral bonded to `HV_MID / PE`)
- Hardware Zero-Crossing Synchronization Line
- Split `24VDC` Common Auxiliary Power Bus (`AUX_LOGIC_24V` / `AUX_COIL_24V`)
- Microgrid CAN Network

## Inputs

- High-voltage split DC power (`+200VDC` / `HV_MID` / `-200VDC`) from the `400VDC` Inverter Bus
- Hardware synchronization pulses from the designated Time Master
- Control power from `AUX_LOGIC_24V` and safety-interlocked coil power from `AUX_COIL_24V`
- Output enable, operating mode, and remote breaker reset commands from the Coordinator

## Outputs

- Regulated `60 Hz` AC power to residential branch or main circuits
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

- Immediately open physical AC line air-gap contactors (`L1` for 120V; `L1` and `L2` for 240V, maintaining continuous Neutral-Ground bonding) and 2-pole DC disconnect (`HV+` and `HV-`) upon overload (`I²t`), short-circuit, ground-fault leakage, DC-bus fault, or overtemperature events
- Autonomously enter a protective state without Coordinator approval when safe operating limits are exceeded
- Prevent back-feeding or unsynchronized parallel connection onto live AC lines

## Coordinator Interaction

- Transmits periodic heartbeats (`250 ms`), real-time AC load telemetry (`200 ms`), and circuit-breaker trip states
- Receives output enable commands and operator-initiated software circuit-breaker resets from the Coordinator HMI/web interface
- Maintains autonomous AC output regulation and local breaker protection during Coordinator communication loss (`> 500 ms`)

## Fault Conditions

- AC Output Short-Circuit / Instantaneous Overcurrent
- Inverse-Time (`I²t`) Thermal Overload Trip
- AC Ground-Fault / Residual Current Leakage (`GFCI/RCD`) Trip
- `400VDC` (`±200VDC`) Bus Undervoltage / Overvoltage / Split-Rail Imbalance
- Phase Synchronization Loss
- Heatsink Overtemperature
- Output Contactor or DC Disconnect Weld / Feedback Fault
- Coordinator Communication Timeout

## Open Questions

- Master-election handoff behavior if the designated Hardware Time Master inverter trips offline
