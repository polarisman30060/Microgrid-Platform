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

The Coordinator firmware now transmits the normal coordinator heartbeat and parses the proposed Battery 0 identity, heartbeat, and telemetry frames. The configured Boost 0 CAN profile is frozen for isolated Nucleo simulator and compatible-firmware development; broader CAN behavior remains draft. The standalone Nucleo Boost 0 firmware project is in [firmware/nucleo_boost_simulator/](firmware/nucleo_boost_simulator/) and its PlatformIO build passes; board flashing and CAN-bench tests are pending. Coordinator command transmission and Boost frame support remain future work. See the [Coordinator CAN implementation guide](docs/Coordinator_CAN_Implementation.md).

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
