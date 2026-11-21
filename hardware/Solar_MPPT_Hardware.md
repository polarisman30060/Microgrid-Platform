# Solar MPPT Node Hardware Baseline

## Module Scope & Physical Deployment
- **Granularity:** **Per-panel / per-string dedicated MPPT module** mounted directly behind or adjacent to the photovoltaic array.
- **Mounting Environment:** Sealed enclosure mounted in an **inaccessible outdoor location** near the panels; designed for zero physical human interaction after installation.
- **No Local UI:** **No local display, buttons, or keypad.** All provisioning, node address setup, power-stage profile selection, diagnostics, and firmware updates are executed remotely over **CANbus** via the module's isolated **Solar Segment Gateway** ([Solar_Gateway_Node.md](../architecture/nodes/Solar_Gateway_Node.md); gateway routing/provisioning payloads remain TBD) and the ESP32-S3 System Coordinator web interface.
- **Operational Role:** Unidirectional current source harvesting solar power across widely varying panel `Vmp` characteristics and regulating injection onto the `96VDC Energy Bus` (`91.2V–100.8V`, `±5%` normal regulation).

## Modular Two-Board Architecture
To accommodate vastly different solar panel Maximum Power Point (`MPP`) voltages and wattage ratings without redesigning the digital electronics, the Solar MPPT Node splits into two standardized boards:

1. **Common Control Daughter Card (Universal Across All Variants):**
   - **Microcontroller:** Compact **STM32G474** package variant—such as the **STM32G474CE** (`LQFP-48`) or **STM32G474RE** (`LQFP-64`)—maintaining 100% firmware and `HRTIM` peripheral compatibility with the system-standard **STM32G474QE** (`LQFP-128`) while minimizing PCB footprint.
   - **Standardized Interconnect:** Pin-compatible board-to-board header carrying isolated `HRTIM` PWM gate signals, isolated analog telemetry (`ADC`), hardware fault trips, and power-stage identification/configuration lines.
   - **Auxiliary Power:** Powered from the **30V 50kHz** common auxiliary power source so the node remains addressable by the Coordinator even after sundown.
   - **Form Factor:** Compact **surface-mount device (SMD)** PCB using commercial off-the-shelf (**COTS**) components.

2. **Swappable Power Converter Cards (Panel-Specific Revisions):**
   - **Role:** Houses the application-specific power switches, **Infineon** isolated gate drivers, magnetics, bulk capacitors, and bus isolation hardware sized for a specific solar panel voltage/current class.
   - **Variant A — Low-Voltage Single-Panel Boost Card:** Optimized for standard `60-cell / 72-cell` residential panels (`Vmp ~30V–45V`, `300W–600W` class), stepping up directly to the `96VDC Energy Bus`.
   - **Variant B — Wide-Range / High-Voltage Buck-Boost Card:** 4-switch synchronous buck-boost topology for high-voltage thin-film panels, bifacial high-`Voc` modules, or short series strings (`24V` to `200V` input window) that can sit either below or above the `96VDC` bus.
   - **Variant C — High-Power Array / Multi-Panel Card:** Scaled power stage with heavier copper busbars and higher-current magnetics for large-format commercial panels (`600W+`) or parallel panel pairs.

## Galvanic Isolation, CANbus & Bus Interface
- **Coordinator Communication:** Galvanically isolated **CANbus** (`STM32G474` `FDCAN` interface) connected to an isolated **Solar Segment Gateway** local CAN segment (`slot 0–31`, `500 kbps` Classic CAN per [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md); gateway summary/routing payloads remain TBD).
- **Isolated Telemetry:**
  - **Current Sensing:** Isolated **Allegro ACS37002** Hall-effect current sensors on both the PV input and `96VDC` bus output.
  - **Voltage Sensing:** Isolated **TI AMC1311** differential amplifiers measuring PV input voltage and `96VDC` bus rail voltage.
- **Bus Interface & Protection:**
  - Reverse-current blocking stage (ideal diode / synchronous FET cutoff) to prevent back-feeding the panel at night or during array shading.
  - Solid-state or miniature `24VDC` coil bus isolation relay/contactor with pre-charge limiting for hot-plugging onto the live `96VDC` bus.

## Future Snow-Melting Feature (Provisioned)
- **Common Control Card (Rev A):** I/O pins, isolated gate-drive signal headers, temperature sensor inputs, and firmware state hooks are **fully provisioned** on the `STM32G474` daughter card to support active snow-melting.
- **Swappable Power Cards:** High-current reverse-bias power switching hardware required to drive heating current back into the solar panel cells is **omitted from initial Rev A power cards** and deferred to future snow-melting power card revisions.
