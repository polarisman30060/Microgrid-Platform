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

The Coordinator firmware now transmits the normal coordinator heartbeat and parses the proposed Battery 0 identity, heartbeat, and telemetry frames. The initial V0 targets one Battery Node emulated by a PC program connected through the Waveshare USB-CAN-A; the Coordinator uses normal node messages and needs no simulator-specific mode. The PC simulator is in [simulator/](simulator/); Battery command transmission in the Coordinator firmware remains future work. See the [Coordinator CAN implementation guide](docs/Coordinator_CAN_Implementation.md).

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
