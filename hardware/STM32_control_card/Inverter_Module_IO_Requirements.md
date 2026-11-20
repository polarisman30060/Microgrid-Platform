# Inverter Module Control-Card I/O Requirements

Status: Draft functional inventory. Signal counts, gate-driver architecture, connector positions, and STM32 pin assignments are not finalized.

## Purpose and Scope

Identify control-card interfaces for the two inverter variants described in the [inverter hardware baseline](../Inverter_Modules_Hardware.md) and [Inverter Node definition](../../architecture/nodes/Inverter_Node.md). The common card is described in the [control-card architecture](./STM32_Control_Card_Architecture.md).

Variant A uses a two-leg H-bridge for 120 VAC. Variant B uses a three-leg bridge for 240 VAC split phase with an active neutral. The module converts the 380 VDC bus to AC and supports synchronized parallel operation. This document inventories signals; it does not define the complete inverter protection or gate-drive design.

## Functional Signal Inventory

Directions are relative to the control card. A logical measurement is not necessarily a dedicated MCU pin. The host power board must supply conditioned signals with specified safe voltage ranges and references.

| Function | Direction | Quantity | Status and purpose |
| --- | --- | --- | --- |
| Bridge gate commands | Output | 4 for 120 V; 6 for 240 V | The STM32 supplies separate high- and low-side PWM commands for each bridge leg: two legs for 120 VAC, three for 240 VAC. Define gate-driver interface, dead time, timing, and default-off behavior. |
| Gate-driver enable | Output | 1 shared for each variant | A shared command enables or disables all bridge gate drivers through the host board. Define the interface and ensure a disabled default on reset, loss of card power, or card removal. Independent hardware fault shutdown is still required. |
| DC input voltages | Input | 2 for each variant | Measure the upstream 380 VDC bus and the inverter input-capacitor voltage on the load side of the DC disconnect, including for precharge assessment. Define sensing, scaling, isolation, and ADC timing. |
| AC output voltage | Input | 1 for 120 V; 2 for 240 V | Measure output voltage on the 120 V variant. For 240 V, measure L1-N and L2-N. Define sensing, scaling, isolation, and ADC timing. |
| AC output current | Input | 1 for 120 V; 2 for 240 V | Measure output current on the 120 V variant. The 240 V variant measures 240 V output current and neutral current for split-phase imbalance monitoring. No dedicated sensor overcurrent trip output is currently specified; define sensor placement and verify short-circuit protection. |
| Heatsink temperature | Input | 1 for each variant | One analog heatsink temperature sensor. Choose sensor type, placement, and conditioning. |
| Synchronization input | Input | 1 | A slave receives the master's zero-crossing reference. The card must make a hardware-timer-compatible input available through the host interface. |
| Synchronization output | Output | 1 | A master provides the zero-crossing reference to parallel inverters. Make a timer-associated output available through the host interface; define the line driver and isolation. |
| Hardware trip | Input | 1 common HRTIM fault input | The host combines applicable hardware faults, including gate-driver faults, into a common trip to shut down all bridge PWM outputs. Define other fault sources, host logic, reporting, and electrical characteristics. |
| AC output contactor command | Output | At least 1 | Each variant specifies an output contactor; drive the host-board coil driver, not the coil directly. Confirm coil and pole arrangement. |
| DC-side disconnect command | Output | 1 for each variant | Command a per-inverter DC-side disconnect through a suitable host-board driver. Define the disconnect device, electrical interface, and safe state. |
| DC input precharge command | Output | 1 provisional for each variant | Reserve a command for a host-side precharge path. Confirm the need, circuit, electrical interface, and safe state as the power stage is designed. |
| AC contactor feedback | Input | 1 for each variant | One feedback signal per inverter. Define the contact or other sensing method; a single signal does not necessarily verify each pole of the 240 V contactor. |
| DC disconnect feedback | Input | 1 for each variant | Separate feedback for the DC-side disconnect. Define the sensing method and what it can verify. |
| Cooling fan speed control | Output | 1 for each variant | PWM fan-speed command to a suitable host-side fan interface. Define levels, frequency, electrical drive, and default behavior. |
| Cooling fan tachometer | Input | 1 for each variant | Fan tachometer feedback. Define signal conditioning, voltage levels, and timing requirements. |
| Other host control/status | Input / output | TBD | Identify any additional gate-driver status and sensing required by the host design. |

CAN, display/keypad, SWD, and card power are common functions, not additional inverter host I/O. Synchronization and power-stage signals must be assessed alongside them when allocating pins.

The identified analog measurements total five for the 120 V variant and seven for the 240 V variant, including both DC input voltages and the heatsink. Further sensing may be needed as the host design develops.

## Preliminary Resource Comparison

The STM32G474QET6 datasheet Table 12 shows the candidate HRTIM pairs PA8/PA9 (A), PA10/PA11 (B), and PB12/PB13 (C) for the separate high- and low-side commands. Two pairs could serve the two-leg variant and all three pairs could serve the three-leg variant. These pins do not overlap the current boost candidate for CAN, SPI clock/data, SWD, or its common fault input. This is a pin-capability check, not a validated gate-drive, timer, or common-card configuration.

The datasheet also lists `HRTIM1_SCIN` on `PB2` (LQFP128 pin 41) and `PB6` (pin 121), and `HRTIM1_SCOUT` on `PB1` (pin 40) and `PB3` (pin 118). `PB2` is ADC-capable, and `PB3` is `JTDO-TRACESWO` (`SWO`); the draft [control-card master pinout](./STM32_Control_Card_Pinout.md) assigns `PB1` (`AF13`) to `HRTIM1_SCOUT` and `PB6` (`AF12`) to `HRTIM1_SCIN` so `PB3` remains dedicated to `SWO` on the debug header.

The [STM32G4 reference manual](../../docs/datasheet/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), RM0440 Rev 9, section 28.3.19, pages 970-972, describes an HRTIM-generated sync output and an external rising-edge sync input that can start or reset enabled HRTIM counters. The SCOUT pulse is 16 HRTIM clock cycles, not an automatically generated 10 microseconds as in the existing inverter baseline. Its available event sources and pulse width, the 60 Hz waveform reference, and the receiving circuitry must be evaluated together; a different timer or pulse conditioning may be needed. Timer synchronization alone does not establish zero-cross alignment of the AC waveform.

Select the synchronization method for the best achievable master-to-slave waveform alignment, rather than preserving the existing 10 microsecond pulse assumption. Define and measure the allowed phase error, jitter, and propagation-delay variation under realistic operating conditions before selecting the timer, pulse format, isolation, and distribution circuitry.

The [STM32G474 datasheet](../../docs/datasheet/stm32g474qe.pdf), DS12288 Rev 6, Table 12, pages 61-67 and 71, is the source for these pin capabilities. The complete display, keypad, sensing, fault, and synchronization allocation still needs verification across both inverter variants and the other modules.

## Hardware Boundaries

- The host supplies gate drivers, power-stage sensing and isolation, and the output contactor driver. Never connect AC output or the 380 V bus directly to MCU pins.
- The STM32 provides both PWM commands for each bridge leg, allowing HRTIM dead-time control. The host gate-drive design must also provide appropriate independent shoot-through protection and safe defaults; specific devices and timing are not selected.
- Neutral current indicates the imbalance between the split-phase line currents, subject to waveform and current-path conditions. The 240 V output-current measurement and neutral measurement do not automatically provide independent L1 and L2 readings; define the sensor placement and verify that sensing and independent protection are adequate for each conductor under mixed loads.
- The intended current-mode control is expected to limit current during a hard short, so a dedicated inverter sensor overcurrent trip is not currently specified. This protection is not yet demonstrated: evaluate response time, peak switch and inductor currents, sensor and control failure modes, and the need for independent hardware protection before finalizing the design. The existing inverter baseline's sub-microsecond protection claim remains unverified.
- Provide hardware protection to disable switching on critical trips without relying on software or network communications. Independently define safe behavior for card reset, loss of power, and removal. A contactor is not a substitute for fast gate shutdown.
- Reserve one common HRTIM fault input for host-board fault logic to disable every bridge PWM output in hardware. Define the individual trip sources, safe output states, fault-line failure behavior, and recovery separately.
- The sync connection must maintain the required isolation and be validated for signal integrity, pulse width, propagation delay, startup, and loss of synchronization. The existing sub-10 ns claim is not a verified end-to-end requirement or measurement. Define how the reference relates to waveform phase and how synchronization affects PWM counters without introducing unsafe output transitions.
- No complete pinout, sampling schedule, isolation rating, or protection circuit is approved by this inventory.

## Open Hardware Allocation Questions

- What gate-driver interface, dead time, and independent shoot-through protection are required for the selected switches?
- Can the current-mode control safely limit a hard short under worst-case conditions, and what independent protection is needed for control or sensing failures?
- Define the 120 V voltage and current measurement locations and the 240 V output-current sensing method.
- What heatsink sensor type, placement, and conditioning should be used? How will the single contactor feedback signal be generated and what can it verify?
- What DC-side disconnect device, driver, and feedback sensing method will be used?
- Is a controlled DC input precharge path required, and how should its provisionally reserved output interface with it?
- How are the master zero-crossing event and slave waveform phase generated and aligned with PWM timing? Specify pulse polarity, width, frequency, isolation, distribution, and master/slave role selection.
- Are any additional gate-driver status or other host signals needed, and which other host fault sources feed the common HRTIM input?
- What fan control and tachometer electrical interface and timing are required?

## Verification

After defining signal counts, check the candidate functions and alternate-function conflicts against the datasheet, HRTIM/ADC timing against the reference manual, and the common connector against every supported host. Validate isolation, synchronization, reset and hardware-trip behavior on representative hardware before operating the power stage.
