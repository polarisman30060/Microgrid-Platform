# Boost Module Control-Card I/O Requirements

Status: Draft functional inventory for the common control card. Signal counts, protection architecture, connector positions, and STM32 pin assignments are not finalized.

## Purpose and Scope

Identify the interface between the common STM32 control card and the 10 kVA, five-phase boost power board. This inventory is derived from the [boost hardware baseline](../Boost_Converter_Hardware.md), [Boost Node definition](../../architecture/nodes/Boost_Node.md), and [control-card architecture](./STM32_Control_Card_Architecture.md). It is not a schematic, complete protection design, or approved connector pinout.

The boost module converts the 96 VDC energy bus to the 380 VDC inverter bus. Its baseline specifies five independently controlled boost phases, each switching at 40 kHz, with phase shedding and interleaving. The host power board contains the power switches, isolated gate drivers, sensors, power isolation, and contactor drivers.

## Functional Signal Inventory

Directions are relative to the control card. Counts refer to logical signals, not necessarily dedicated MCU pins; the final implementation may use conditioning or other interface circuitry on the host. All electrical ranges and references remain to be specified.

| Function | Direction | Quantity | Status and purpose |
| --- | --- | --- | --- |
| Phase PWM commands | Output | 5 | Baseline: one independently controllable PWM command per single-switch phase, with HRTIM interleaving and phase shedding. Verify timer outputs, pin mux, startup states, and timing for the selected MCU. |
| Phase current measurements | Input | 5 | Baseline: one conditioned analog measurement for each phase. Define sensor range, scaling, filtering, ADC timing and accuracy, including behavior when a phase is shed. |
| Phase overcurrent trips | Input | 5 sensor outputs, common trip | Any phase sensor's overcurrent output must trigger HRTIM hardware shutdown of all five PWM outputs without an interrupt or software decision. Individual phase identification is not required. The number of card inputs and fault routing are TBD. |
| 96 V bus voltage | Input | 1 | Baseline: conditioned voltage measurement for control, pre-charge decisions, and protection monitoring. |
| 380 V bus voltage | Input | 1 | Baseline: conditioned voltage measurement for regulation and protection monitoring. |
| Bus current measurements | Input | 2 | Measure total input and output bus current separately from the five phase-current sensors. Define sensor ranges, scaling, filtering, and ADC timing. |
| Heatsink temperature | Input | 1 | One analog heatsink temperature sensor. Define sensor type, placement, conditioning, and measurement range. |
| Inductor overtemperature | Input | TBD | Overtemperature snap discs are the likely protection method. Disc count, placement, wiring, trip integration, and reporting remain to be decided. |
| Pre-charge contactor command | Output | 1 | Baseline: control a host-board driver, not the 24 V coil directly. Confirm the pre-charge circuit and its safe de-energized state. |
| Main isolation contactor command | Output | 1 | Baseline: control a host-board driver, not the 24 V coil directly. Confirm fault and loss-of-power behavior. |
| Contactor feedback | Input | TBD | Contactor faults are listed, but auxiliary contacts or other feedback methods have not been defined. Distinguish commanded from verified contactor state. |
| Other hardware fault indication / trip | Input | TBD | Define protective trips beyond phase overcurrent and their reporting interfaces. Decide which faults are common or individually identifiable and how they are latched and reset. |
| Gate-driver status or enable | Input / output | TBD | Determine available driver status and any separate enable or interlock signals from the selected driver and host design; do not assume one per phase. |

Five PWM outputs, ten analog measurements, and five phase-sensor overcurrent outputs are identified. The analog measurements comprise five phase currents, two bus voltages, two bus currents, and one heatsink temperature. The number of MCU fault inputs depends on the selected fault routing; snap-disc, feedback, and other fault signal counts remain open. ADC pins, sampling resources, HRTIM fault inputs, and their routing must be verified; logical signals alone do not establish MCU resource compatibility.

CAN, display/keypad, SWD, and card power are common card functions, not additional boost host I/O. See the [control-card architecture](./STM32_Control_Card_Architecture.md). The existing Nucleo boost simulator does not implement power-stage PWM, ADC, or contactor control and is not a production pin map.

## Control and Protection Boundaries

- The control card generates the five phase commands; host-board isolated gate drivers drive the power switches. Define the off state at each interface and ensure disabled phases remain off during startup, reset, loss of control power, or card removal.
- Route the five phase-current sensors' overcurrent outputs to a common HRTIM fault path, directly or through suitable host-board fault logic. Any one phase trip must force all five PWM outputs to the gate-driver off state in hardware; individual phase identification is not required. The fault must latch, and the isolation contactor must open. The latch implementation, reset method, and contactor trip path remain to be designed; do not rely on contactor opening for the initial fast PWM shutdown. A phase overcurrent may indicate a hard short in the power electronics. The proposed [ACS37002 family](https://www.allegromicro.com/en/products/sense/current-sensor-ics/integrated-current-sensors/acs37002) supports adjustable overcurrent fault protection; verify the exact part, threshold and output characteristics before specifying the circuit.
- Hardware overcurrent and other critical trips must inhibit the appropriate switching paths without relying on firmware, CAN, or the display. Determine the response to other faults, how trips interact with the contactors, and what feedback reaches the controller.
- HRTIM fault shutdown depends on the MCU being powered and correctly configured. Provide an independent host-side safe-disable mechanism for card removal, unpowered or resetting MCU, and other conditions in which the timer cannot enforce protection. Define how sensor power loss and fault-line disconnection are detected or made safe.
- Establish trip thresholds and the worst-case end-to-end delay, including sensor, fault logic, HRTIM, gate driver, and power switch, against the power stage's safe operating limits. Validate the complete path on hardware; no specific response time is established here.
- Inductor thermal protection will most likely use snap discs rather than analog inductor-temperature measurements. Their placement, trip and reset characteristics, fail-safe wiring, and integration into the protective trip path must be designed and verified; an MCU indication alone is not the independent protective path.
- The host supplies appropriately conditioned, protected signals referenced to the card's control domain. Define each sensor's isolation boundary, voltage range, accuracy, and failure behavior; do not connect 96 V or 380 V directly to the control card.
- The two contactor command signals drive host circuitry. Their physical topology, voltage-monitoring points, coil suppression, feedback, and interlock behavior need separate design and verification.
- Loss of coordinator communications is not equivalent to a local hardware trip: the boost node's specified Primary/Secondary degraded behavior applies while local protection remains active.

## Pin-Mapping Prerequisites

### Preliminary STM32G474QET6 Resource Check

The supplied [STM32G474 datasheet](../../docs/datasheet/stm32g474qe.pdf) (DS12288 Rev 6, Table 12, pages 59-69) identifies the following non-overlapping candidate functions in the 128-pin LQFP package. These are an initial boost feasibility check, **not an approved common-card pinout**.

| Candidate function | MCU pin | LQFP128 pin | Peripheral function |
| --- | --- | ---: | --- |
| Phase PWM A | PA8 | 89 | HRTIM1_CHA1 |
| Phase PWM B | PA10 | 91 | HRTIM1_CHB1 |
| Phase PWM C | PB12 | 66 | HRTIM1_CHC1 |
| Phase PWM D | PB14 | 68 | HRTIM1_CHD1 |
| Phase PWM E | PC8 | 87 | HRTIM1_CHE1 |
| Common overcurrent trip | PA12 | 93 | HRTIM1_FLT1 |
| CAN receive / transmit | PD0 / PD1 | 108 / 109 | FDCAN1_RX / FDCAN1_TX |
| Display clock / data | PA5 / PA7 | 34 / 36 | SPI1_SCK / SPI1_MOSI |
| Debug | PA13 / PA14 | 96 / 98 | SWDIO / SWCLK |

PA12 also supports FDCAN1_TX, so CAN cannot use PA12 when it is assigned to HRTIM1_FLT1. PD0/PD1 are one alternative. The remaining display controls and eight keypad inputs, the two contactor outputs, other host signals, clocks, and dedicated support pins have not yet been allocated.

Ten distinct ADC-capable candidates remain available without overlapping the functions above: PC0-PC3, PA0-PA3, PB0, and PB1 (datasheet Table 12, pages 59-61). They are examples, not signal assignments. Pin availability does not establish that the sampling schedule, analog performance, or simultaneous conversion requirements are met.

The [STM32G4 reference manual](../../docs/datasheet/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) (RM0440 Rev 9, section 28.3.17, pages 963-967) states that fault channels can be associated with multiple timing units and configured to force outputs to a defined fault state. The unfiltered fault path is asynchronous. The fault state is maintained until software re-enables the outputs; a persistent fault prevents re-enabling. This supports investigating a common trip for all five phases, but the complete latching, reset, host interlock, and contactor implementation remains unverified.

### Remaining Work

1. Define the remaining signal counts and fault behavior with the boost power-board design.
2. Verify five independent HRTIM phase outputs and their 40 kHz timing, phase shifts, and shedding behavior against the STM32G474QET6 datasheet and reference manual.
3. Allocate ADC channels and trigger timing for the phase-current and bus measurements, including latency, concurrent sampling needs, and noise constraints.
4. Reserve common display, CAN, SWD, clock, and MCU support pins, then resolve alternate-function conflicts and map usable remaining GPIO to the host connector.
5. Check the same candidate mapping against the other intended power modules before freezing the common card and connector.

## Open Decisions

- What heatsink sensor type, placement, and conditioning should be used?
- How many inductor snap discs are required, where are they placed, and how are their trips wired, reported, latched, and reset?
- How should the common overcurrent trip be latched and reset, and how should it cause the isolation contactor to open?
- How should the five sensor outputs be routed to HRTIM fault inputs, and what independent host-side disable protects startup, reset, card removal, sensor power loss, and broken fault lines?
- Are auxiliary-contact feedback or other verification signals required for each contactor?
- Does the host need separate gate-enable, driver status, fan control, or other signals?
- What are the ADC accuracy, measurement bandwidth, control-loop timing, isolation, and host connector requirements?

## Verification

Before releasing a pinout, trace every required function to a compatible MCU peripheral and connector position, and validate the mapping with all supported module inventories. Before operating the power stage, verify the host's default-off and hardware-trip behavior, sensor scaling and fault handling, contactor sequencing and feedback, and phase-shedding timing using appropriate isolated test equipment and a controlled test procedure.
