## Battery Node

Purpose:

The Battery Node stores and supplies energy to the microgrid through the 96VDC Energy Bus.

The Battery Node provides a standardized energy-storage interface to the Coordinator regardless of battery chemistry, storage technology, or internal implementation.

### Responsibilities

The Battery Node is responsible for:

- Safe storage of electrical energy
- Controlled charging and discharging
- State of Charge (SOC) estimation
- State of Health (SOH) estimation
- Thermal management and monitoring
- Local fault detection
- Local fault protection
- Reporting operating status to the Coordinator

### Internal Architecture

A Battery Node may contain:

- Energy Storage System
- Battery Management System (BMS)
- Bidirectional DC/DC Converter
- Local Controller
- CAN Communications Interface

The internal architecture of a Battery Node is implementation dependent.

The Coordinator shall not require knowledge of the internal Battery Node design.

### Coordinator Interface

The Coordinator communicates exclusively with the Battery Node.

The Battery Node shall present a standardized interface including:

- State of Charge
- State of Health
- Available Charge Power
- Available Discharge Power
- Voltage
- Current
- Availability Status
- Warning Status
- Fault Status

The Coordinator shall not directly communicate with the Battery Management System.

### Battery Chemistry Independence

The Battery Node architecture shall support multiple storage technologies including:

- Flooded Lead Acid
- AGM Lead Acid
- Gel Lead Acid
- Lithium Iron Phosphate (LiFePO4)
- Lithium-Ion
- Sodium-Ion
- Future Energy Storage Technologies

Battery chemistry-specific implementation details shall remain internal to the Battery Node.

### Safety Authority

The Battery Node possesses local safety authority.

The Battery Node may independently enter WARNING, FAULT, or LOCKOUT states without Coordinator approval.

Examples include:

- Overvoltage
- Undervoltage
- Overcurrent
- Overtemperature
- Communication Failure with Internal Subsystems

The Battery Node shall take protective action immediately when required to maintain safe operation.

### Energy Bus Interface

The Battery Node exchanges energy exclusively through the 96VDC Energy Bus.

The Battery Node shall not require direct knowledge of:

- Inverter Operation
- 380VDC Bus Operation
- AC Load Management
- Solar Generation Control

These functions remain the responsibility of other system nodes and the Coordinator.