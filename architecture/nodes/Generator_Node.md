# Generator Node

## Purpose

The Generator Node interfaces a petrol-driven starter-generator prime mover with the 96VDC Energy Bus, providing bidirectional power transfer to crank the engine during startup and inject regulated DC charging power during generation.

## Responsibilities

- Draw energy from the 96VDC Energy Bus to crank and start the internal combustion engine
- Transition smoothly from motoring (cranking) mode to power generation mode once engine ignition and operating RPM are achieved
- Regulate generator field excitation and power conversion onto the 96VDC Energy Bus
- Monitor engine RPM, electrical output, and thermal limits
- Report engine state, generation telemetry, and fault conditions to the Microgrid CAN Network

## Not Responsible For

- Determining when generator dispatch is economically or energetically required across the microgrid
- Managing battery state-of-charge targets or fleet charging allocation
- Regulating the 380VDC Inverter Bus or AC loads

## Interfaces

- Petrol Engine & Starter-Generator Assembly
- 96VDC Energy Bus
- 30V 50kHz Common Auxiliary Power Bus
- Microgrid CAN Network

## Inputs

- Energy from the 96VDC Energy Bus (during engine cranking)
- Mechanical power from the petrol engine (during generation)
- Control power from the 30V 50kHz Common Auxiliary Power Bus
- Start, stop, and power-output setpoint commands from the Coordinator

## Outputs

- Regulated DC power to the 96VDC Energy Bus
- Engine status, RPM, electrical output, and fault reports to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Operating Modes

- STANDBY
- CRANKING
- GENERATING
- COOLDOWN

## Safety Responsibilities

- Enforce cranking duration limits, over-speed shutdown, and reverse-power protection
- Protect conversion electronics from overcurrent, overvoltage, and overtemperature
- Autonomously shut down fuel/ignition and isolate from the 96VDC Energy Bus without Coordinator approval when safety limits are exceeded

## Coordinator Interaction

- Transmits periodic heartbeats (`1 s`), operating mode, RPM, and power telemetry (`500 ms`)
- Receives engine start/stop dispatch requests and target generation power limits from the Coordinator
- Executes a controlled cooldown and shutdown if Coordinator communications are lost (`>500 ms`) while generating

## Fault Conditions

- Engine Cranking Timeout / Failure to Start
- Engine Over-Speed / Under-Speed Stall
- 96VDC Bus Overvoltage / Undervoltage
- Converter or Alternator Overtemperature
- Phase / Bus Overcurrent
- Pre-Charge or Main Contactor Fault
- Coordinator Communication Timeout

## Open Questions

- Maximum cranking retry count and cooldown lockout timer before requiring operator reset
