# Glossary

## Module

A physical hardware assembly.

A Module may contain one or more internal subsystems.

A Module may contain one or more Nodes.

Examples:

- Battery Module
- Boost Converter Module
- Solar MPPT Module
- Inverter Module

## Node

A logical participant on the microgrid communication network.

A Node possesses a unique CAN address and exchanges information with other Nodes.

A Node may represent an entire Module or a subsystem within a Module.

Examples:

- Battery Node
- Boost Node
- Solar Node
- Inverter Node
- Coordinator Node

## Coordinator

The system-level decision-making Node responsible for resource allocation and system coordination.

The Coordinator does not own hardware safety functions.

## BMS

Battery Management System.

An internal subsystem responsible for monitoring and protecting a battery.

The BMS is not directly visible to the Coordinator.

## Energy Bus

The 96VDC system bus used for energy storage and energy transfer.

## Inverter Bus

The 380VDC bus supplying inverter modules.