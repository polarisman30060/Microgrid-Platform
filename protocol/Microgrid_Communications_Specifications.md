# Microgrid Communications Specification

Version: 0.1
Status: Draft

## CAN Identifier Format

The Microgrid Communications Bus shall use:

- CAN 2.0B Physical Layer
- Standard 11-bit CAN Identifiers

Extended 29-bit identifiers are not used in Revision 0.1.

## Node Discovery

Nodes use fixed manually assigned addresses.

The Supervisor shall perform node discovery during startup and continuously during system operation.

The Supervisor shall not assign CAN addresses.

Nodes may be classified as:

- Required
- Optional
- Ignored

System startup shall require all Required nodes to be present and operational.

## Heartbeats

All nodes shall transmit a heartbeat message every 500 ms.

A node shall be considered offline after three consecutive missed heartbeats.

Offline timeout = 1500 ms.

## System Configuration

During initial commissioning, the Supervisor may discover and record active nodes.

After configuration is saved, the recorded node list becomes the expected system configuration.

Subsequent startups shall compare discovered nodes against the stored configuration.

The Supervisor shall not assign node addresses.

## Startup Validation

Missing Required nodes shall inhibit system startup.

Missing Optional nodes shall not inhibit startup.

The Supervisor shall report missing Optional nodes as warnings to the operator.

## Node Address Allocation

Addresses shall be grouped by node function.

0x100-0x1FF  Battery Nodes

0x200-0x2FF  Solar Nodes

0x300-0x3FF  Boost Nodes

0x400-0x4FF  Inverter Nodes

0x500-0x5FF  Grid Interface Nodes

0x600-0x6FF  Generator Nodes

0x700-0x7FF  Display Nodes

0x800-0x8FF  Service/Test Nodes

## Node Identification

All modules shall display their configured Node ID through the local user interface.

The Node ID shall be visible without the use of external diagnostic equipment.

The local interface should also provide access to:

- Node Type
- Firmware Version
- Communication Status
- Module Serial Number

## Duplicate Address Detection

Each module shall contain a unique serial number.

The Supervisor shall monitor for duplicate node addresses.

If multiple modules report the same Node ID, the Supervisor shall declare an address conflict.

Address conflicts shall be reported to the operator.

Startup shall be inhibited if a Required node is involved in an address conflict.

The Supervisor shall not automatically reassign node addresses.


## Module Serial Numbers

Each module shall contain a permanent unique serial number.

The serial number shall be assigned during manufacturing or initial firmware programming.

Serial numbers shall not be automatically regenerated.

Node IDs may change through configuration.

Serial numbers shall remain constant for the life of the module.


## Fault Authority

Each module shall possess local fault authority.

A module may enter FAULT or LOCKOUT state without Supervisor approval when a local safety condition is detected.

Examples include:

- Overvoltage
- Undervoltage
- Overcurrent
- Overtemperature
- Hardware Faults
- Sensor Failures

The Supervisor shall be informed through fault messages but shall not be required for protective action.

System Philosophy:

Local Modules Own Safety

Supervisor Owns Coordination

## Coordinator Function and Failure Behavior

The Supervisor functions as the System Coordinator.

The Coordinator is responsible for:

- Node Discovery
- Startup Sequencing
- Power Allocation
- Battery Charge / Discharge Management
- Multi-Module Load Balancing
- System Monitoring
- HMI and Operator Interaction

The Coordinator shall not be responsible for module-level safety protection.

Safety protection shall remain the responsibility of the individual module controllers.

Examples include:

- Overcurrent Protection
- Overvoltage Protection
- Undervoltage Protection
- Overtemperature Protection
- Hardware Fault Detection

System Design Philosophy:

Local Modules Own Safety

Coordinator Owns Resource Allocation

### Coordinator Loss

Loss of the Coordinator shall not create an unsafe operating condition.

Upon loss of Coordinator communications, local module controllers shall continue operating using their most recent valid operating parameters.

The system shall transition into DEGRADED MODE.

### Degraded Mode

In DEGRADED MODE:

- Existing power conversion modules may continue operation.
- Local safety functions shall remain fully active.
- System optimization and load balancing functions may be reduced or unavailable.
- Available system capacity may be reduced.
- Non-critical loads may be shed if required.

### Multiple Boost Converter Systems

In systems utilizing multiple boost converter modules, coordinated load sharing is normally managed by the Coordinator.

If Coordinator communications are lost:

- A designated Primary Boost Converter may continue operation.
- Secondary Boost Converters may enter Standby or Shutdown mode.
- The 380VDC bus shall remain operational if sufficient capacity exists within the remaining active boost converters.

This behavior allows the system to preserve operation of critical loads while eliminating dependence on the Coordinator for safe operation.

The loss of optimization shall not immediately result in loss of power.

