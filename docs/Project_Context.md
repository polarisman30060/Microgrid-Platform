# Project Context
Version: 1.0
Last Updated: 2026-11-22

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

The system is built around two primary DC power buses and one un-interruptible auxiliary control lifeline.

## 96VDC Energy Bus

The `96VDC` bus (`91.2V–100.8V`, `±5%` normal regulation; `84.0V–108.0V` fault bounds) is the primary system energy bus. Its negative rail (`LV_BUS-`) is single-point referenced to Earth (`PE`) through a DC GFDI device so the bus does not float.

All energy sources and storage systems connect to this bus.

Examples:

- Battery Nodes (galvanically isolated bidirectional Dual Active Bridge)
- Solar MPPT Nodes
- Generator Nodes
- Utility-Tie PFC Nodes (two-stage galvanically isolated PFC + DC-DC)
- Future Energy Sources and Storage Technologies

System Philosophy:

96VDC Bus = Energy Management Bus

## 400VDC (±200VDC Split-Rail) Inverter Bus

The galvanically isolated `400VDC` (`+200VDC / 0V (HV_MID = N = PE) / -200VDC`; `380V–415V` normal, `360V–435V` trip bounds) split-capacitor bus exists solely as an inverter supply bus.

The `400VDC` bus is not intended for energy storage or direct energy source connection.

The `400VDC` bus is supplied exclusively through **10 kW 2-phase interleaved Full-Bridge LLC (FB-LLC)** Boost Converter Nodes, whose HF transformers isolate the bus so `HV_MID` can be earth-bonded as residential AC Neutral (`N`).

System Philosophy:

400VDC (±200V Split) Bus = Isolated Earth-Referenced Power Conversion Bus

## Split Diode-OR'd `24 VDC` Common Auxiliary Power Bus

The regulated, isolated `24.0 VDC` (`22.8–25.2 V`) auxiliary bus provides un-interruptible control and Black Start power from Battery Nodes (via Ideal-Diode OR'd isolated `48 V → 24 V` supplies with per-tap fuses/eFuses), partitioned into:
- **`AUX_LOGIC_24V`:** Always-on control lifeline feeding the Coordinator, STM32 control cards, and isolated CAN (never cut by E-Stop or local faults).
- **`AUX_COIL_24V`:** Routed through the hardwired latched E-Stop / `96 V` DC GFDI Safety Relay to feed all contactor coils, `NC` `400 V` discharge relays, module `HRTIM1_FLT3` (`ESTOP_OK`), and NEC 690.12 rooftop Solar Rapid Shutdown keep-alive.

System Philosophy:

AUX_LOGIC_24V = Un-interruptible Control & Black Start Lifeline | AUX_COIL_24V = Hardwired Safety-Interlocked Power Enable

# Coordinator & Safety Philosophy

The Waveshare ESP32-S3-Touch-LCD-7 Supervisor acts as the System Coordinator.

Coordinator responsibilities:

- Node Discovery & Startup Sequencing
- Power Allocation & Battery Fleet Coordination
- Multi-Module Load Balancing (`20 ms` current-balancing tokens)
- Local Touchscreen HMI & Wi-Fi Web Server Monitoring

The Coordinator does not own system safety. Local module controllers retain autonomous authority to disconnect and enter protective states without Coordinator approval.

System Philosophy:

Local Modules Own Safety

Coordinator Owns Resource Allocation

# Network & Communications Reference

The Classical CAN 2.0B contract uses `500 kbps` standard 11-bit identifiers (`0x000–0x7FF`), a `100 ms` Coordinator heartbeat, type-specific node heartbeat/missing-node deadlines, and `20 ms` current-sharing updates for active parallel Boost and Inverter modules. Non-paralleled Inverters do not require those frequent updates. Inverter sharing details and bus capacity remain to be validated. The working registry assigns main-bus GlobalNodeIDs and places Solar children on isolated gateway-local segments; gateway payloads and physical implementation remain open. See `protocol/Microgrid_Communications_Specifications.md`, `protocol/CAN_Messages_Definitions.md`, and `docs/Software_Design_Constraints.md` for current status.

# Battery Architecture & DALY BMS Strategy

The Battery Node is the public battery-facing interface to the microgrid, internally partitioning into a Battery Pack, a COTS DALY BMS, and a Bidirectional Charger/Discharger Converter.

- **Chemistry Independence:** Supports Flooded/AGM/Gel Lead-Acid, LiFePO4, Lithium-Ion, Sodium-Ion, and future chemistries without exposing internal details to the Coordinator.
- **DALY BMS Reference Implementation:** COTS DALY BMS units handle initial cell-level monitoring and protection, communicating strictly with the local Charger/Discharger controller using native DALY communications.
- **Abstraction Boundary:** The DALY protocol is internal only and is not the system standard. The local converter translates telemetry onto the unified Microgrid CAN Network.

# Storage Fleet Philosophy

The Coordinator controls storage at the fleet level (`CHARGING`, `DISCHARGING`, `IDLE`) and shall not intentionally command one Battery Node to charge while another Battery Node is discharging.

Energy flow priority:

1. Generation supplies loads.
2. Excess generation charges storage.
3. Storage supplies deficits.
4. Generator/Grid may support charging when desired.

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
- EasyEDA Pro

Target Supervisor Hardware:

- Waveshare ESP32-S3-Touch-LCD-7 Rev 1.2 (`CH422G EXIO5 = HIGH` for CAN transceiver routing)

Target Module Controller:

- STM32G474QE (and compact STM32G474CE/RE packages for per-panel Solar MPPT nodes)
