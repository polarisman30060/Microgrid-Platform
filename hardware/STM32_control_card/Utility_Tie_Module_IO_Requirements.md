# Utility-Tie Module Control-Card I/O Requirements

Status: Draft functional inventory for the common control card. Signal counts, gate-driver details, connector positions, and STM32 pin assignments are not finalized.

## Purpose and Scope

Identify the interface between the common STM32 control card and the 10.8 kW two-phase interleaved active PFC Utility-Tie power board described in the [utility-tie hardware baseline](../Utility_Tie_Hardware.md) and [control-card architecture](./STM32_Control_Card_Architecture.md).

The module converts `120 VAC / 240 VAC` utility power onto the `96 VDC` energy bus using a unidirectional two-phase interleaved active PFC stage (`180°` out of phase) with `HRTIM1` control.

## Functional Signal Inventory

Directions are relative to the control card. Counts refer to logical signals, not necessarily dedicated MCU pins; the host power board supplies conditioned signals with specified safe voltage ranges and references.

| Function | Direction | Quantity | Status and purpose |
| --- | --- | --- | --- |
| Interleaved PFC PWM commands | Output | 2 | Two `180°` out-of-phase HRTIM PWM commands for the two interleaved unidirectional PFC switching legs. |
| Gate-driver enable | Output | 1 shared | Shared command to enable or disable the PFC gate drivers through the host board; must default to disabled on reset, power loss, or card removal. |
| AC input voltage | Input | 1 | Conditioned, isolated differential measurement (`TI AMC1311` or equivalent) of instantaneous AC line voltage for amplitude and phase/zero-crossing tracking. |
| AC input current | Input | 1 | Conditioned analog measurement of total AC input current. |
| PFC phase currents | Input | 2 | Conditioned analog measurements of the two interleaved PFC phase currents (`ACS37002` or equivalent) for current-loop regulation and cycle-by-cycle balancing. |
| 96 V bus voltage | Input | 1 | Conditioned analog measurement of the 96 VDC energy bus, including for pre-charge assessment and protection. |
| 96 V bus current | Input | 1 | Conditioned analog measurement of DC output current onto the 96 VDC bus. |
| Heatsink temperature | Input | 1 | Analog heatsink temperature sensor. Define sensor type, placement, and conditioning. |
| Hardware trip | Input | 1 common HRTIM fault input | Combines current-sensor overcurrent outputs and gate-driver faults into a common hardware trip to shut down both PFC PWM outputs. |
| AC input contactor command | Output | 1 | Command a host-board driver for the AC utility input isolation contactor. |
| 96 V pre-charge contactor command | Output | 1 | Command a host-board driver for the 96 V bus pre-charge contactor. |
| 96 V main isolation contactor command | Output | 1 | Command a host-board driver for the 96 V bus main isolation contactor. |
| Contactor feedback | Input | 3 | Separate feedback inputs to verify the AC input contactor, 96 V pre-charge contactor, and 96 V main isolation contactor states. |
| Cooling fan speed control | Output | 1 | PWM fan-speed command to a suitable host-side fan interface. |
| Cooling fan tachometer | Input | 1 | Fan tachometer feedback. Define conditioning, levels, and timing. |
| Other host control/status | Input / output | TBD | Identify any dedicated hardware zero-crossing digital comparator input (if not derived solely from ADC sampling) or additional overtemperature trips. |

In summary, the utility-tie module currently identifies **2 PWM outputs**, **7 analog measurements** (AC voltage, AC current, 2 phase currents, 96 V voltage, 96 V current, heatsink temperature), **1 common hardware trip**, **1 shared gate-driver enable**, **3 contactor outputs + 3 contactor feedbacks**, and **cooling fan PWM + tachometer**.

CAN, display/keypad, SWD, and card power are common card functions, not additional utility-tie host I/O.

## Hardware Boundaries

- The host supplies isolated gate drivers, AC and DC isolation/sensing, and 24 V contactor drivers. Never connect utility AC, 96 V bus, or inductive coil loads directly to MCU pins.
- Route current-sensor overcurrent outputs and gate-driver faults to the common HRTIM hardware trip so switching disables in hardware without software latency. Provide an independent host-side safe-disable state for card reset, loss of power, or card removal.

## Open Hardware Allocation Questions

- Is AC zero-crossing tracked purely from the isolated AC-voltage ADC channel, or should a dedicated digital zero-crossing comparator input also be reserved?
- Are any thermal snap discs or extra fault inputs needed on the PFC inductors or rectifiers?
