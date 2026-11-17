# Project Context
Version: 0.1
Last Updated: 2026-11-16

# Project Overview

The project is a modular residential microgrid platform designed around distributed intelligence, modular power conversion, modular energy storage, and fault-tolerant operation.

Primary goals:

- Modularity
- Scalability
- Serviceability
- Fault Tolerance
- Chemistry Independence
- Vendor Independence

# Core System Architecture

The system is built around two primary DC buses.

## 96VDC Energy Bus

The 96VDC bus is the primary energy bus.

All energy sources and storage systems connect to this bus.

Examples:

- Battery Nodes
- Solar MPPT Nodes
- Generator Nodes
- Future Energy Sources
- Future Energy Storage Technologies

System Philosophy:

96VDC Bus = Energy Management Bus

## 380VDC Inverter Bus

The 380VDC bus exists solely as an inverter supply bus.

The 380VDC bus is not intended for energy storage.

The 380VDC bus is supplied exclusively through Boost Converter Nodes.

System Philosophy:

380VDC Bus = Power Conversion Bus

# Coordinator Philosophy

The ESP32-S3 Supervisor acts as the System Coordinator.

Coordinator responsibilities:

- Node Discovery
- Startup Sequencing
- Power Allocation
- Battery Fleet Coordination
- Load Balancing
- HMI Functions
- System Monitoring

The Coordinator does not own system safety.

System Philosophy:

Local Modules Own Safety

Coordinator Owns Resource Allocation

# Node Discovery

Node addresses are manually assigned.

The Coordinator never assigns addresses.

Coordinator responsibilities:

- Discover Active Nodes
- Verify Expected Nodes
- Detect Missing Nodes
- Detect Duplicate Node IDs

Node Classification:

- Required
- Optional
- Ignored

Missing Required Nodes:

- Startup inhibited

Missing Optional Nodes:

- Startup allowed
- Warning displayed

# CAN Communications

Bus Type:

- CAN 2.0B
- Standard 11-bit IDs

Dynamic addressing is not used.

Heartbeat Interval:

- 500 ms

Offline Detection:

- 3 missed heartbeats
- 1500 ms timeout

# Node Address Allocation

0x100-0x1FF  Battery Nodes

0x200-0x2FF  Solar Nodes

0x300-0x3FF  Boost Nodes

0x400-0x4FF  Inverter Nodes

0x500-0x5FF  Grid Interface Nodes

0x600-0x6FF  Generator Nodes

0x700-0x7FF  Display Nodes

0x800-0x8FF  Service/Test Nodes

# Serial Numbers

Every module contains:

- Permanent Serial Number
- Configurable Node Address

Node Address:

- Network Location

Serial Number:

- Hardware Identity

Serial Numbers never change.

Node IDs may be reconfigured.

# Duplicate Address Handling

Duplicate node addresses are not permitted.

The Coordinator shall:

- Detect duplicates
- Report duplicates
- Identify affected serial numbers

The Coordinator shall never automatically reassign addresses.

# Battery Architecture

The Battery Node is the public battery-facing interface to the microgrid.

A Battery Node may contain:

- Energy Storage
- Battery Management System (BMS)
- Bidirectional DC/DC Converter
- Local Controller
- CAN Interface

The Coordinator communicates with the Battery Node.

The BMS is considered an internal subsystem.

# Battery Chemistry Independence

Supported battery technologies may include:

- Flooded Lead Acid
- AGM
- Gel
- LiFePO4
- Lithium-Ion
- Sodium-Ion
- Future Technologies

The Coordinator shall not require knowledge of battery chemistry.

# Battery Interface Philosophy

Every Battery Node should expose:

- State of Charge (SOC)
- State of Health (SOH)
- Pack Voltage
- Pack Current
- Available Charge Power
- Available Discharge Power
- Availability State
- Warning State
- Fault State

The Battery Node abstracts battery chemistry and BMS implementation.

# Storage Fleet Philosophy

The Coordinator controls storage at the fleet level.

Battery fleet states:

- CHARGING
- DISCHARGING
- IDLE

The Coordinator should not intentionally command one Battery Node to charge while another Battery Node is discharging.

Energy flow priority:

1. Generation supplies loads.
2. Excess generation charges storage.
3. Storage supplies deficits.
4. Generator/Grid may support charging when desired.

# DALY BMS Strategy

DALY BMS devices are the initial reference implementation.

The DALY protocol is not the system standard.

The system standard is the Battery Node Interface.

Future battery technologies shall be integrated through the Battery Node abstraction layer.

# Open Items

- Complete Battery Node Definition
- Define Boost Node
- Define Solar Node
- Define Inverter Node
- Define Generator Node
- Define Grid Interface Node
- Define Coordinator State Machine
- Define Load Sharing Protocol
- Define Battery Fleet Management
- Define Startup Sequence
- Define Degraded Mode Behavior
- Define Fault Message Formats

# Development Environment

Repository:

Microgrid-Platform

Development Tools:

- GitHub
- GitHub Desktop
- Visual Studio Code
- PlatformIO
- ESP-IDF
- STM32CubeIDE

Target Supervisor Hardware:

- Waveshare ESP32-S3-TOUCH-LCD-7 Rev 1.2

Target Module Controller:

- STM32G474QE
