# Microgrid Platform

Modular residential microgrid architecture based on:

- STM32G474QE module controllers
- ESP32-S3 supervisory controller
- Distributed fault-tolerant architecture
- Isolated CAN communications
- Modular power conversion stages
- Modular energy storage architecture

## Project Status

Architecture Development

The Coordinator firmware transmits its normal heartbeat and parses Battery 0 identity, heartbeat, and telemetry using the working CAN registry. Boost 0 has been reopened; the registry assigns unique main-bus GlobalNodeIDs and places Solar children on isolated gateway-local segments. Solar gateway payloads and physical gateway implementation remain open. All Nucleo simulator projects are grouped under [firmware/nucleo_simulators/](firmware/nucleo_simulators/), with a separate directory for each module. Board flashing and CAN-bench tests are pending. Coordinator support for additional node types remains future work. See the [Coordinator CAN implementation guide](docs/Coordinator_CAN_Implementation.md).

## Major Components

- Supervisor HMI
- Battery Management Modules
- Solar MPPT Modules
- Boost Converter Modules
- Inverter Modules
- Grid Interface Modules

## Documentation

- [Software design constraints](docs/Software_Design_Constraints.md)
- [Coordinator CAN implementation guide](docs/Coordinator_CAN_Implementation.md)
- [PC microgrid simulator requirements](docs/Microgrid_Simulator_Requirements.md)
- [Architecture](architecture/)
- [Protocol](protocol/)
- [Hardware](hardware/)
- [Firmware](firmware/)
