# Battery Charger/Discharger Module Control-Card I/O Requirements

Status: Draft functional inventory for the common control card. Signal counts, BMS transceiver details, connector positions, and STM32 pin assignments are not finalized.

## Purpose and Scope

Identify the interface between the common STM32 control card and the 2.5 kW bidirectional Battery Charger/Discharger power board described in the [battery hardware baseline](../Battery_and_BMS_Hardware.md) and [control-card architecture](./STM32_Control_Card_Architecture.md).

The module bridges a nominal 48 V battery pack (managed by a COTS DALY Smart BMS) to the 96 VDC energy bus using a 4-switch synchronous buck-boost converter switching at 50 kHz.

## Functional Signal Inventory

Directions are relative to the control card. Counts refer to logical signals, not necessarily dedicated MCU pins; the host power board supplies conditioned signals with specified safe voltage ranges and references.

| Function | Direction | Quantity | Status and purpose |
| --- | --- | --- | --- |
| Buck-boost bridge PWM | Output | 4 (2 high/low pairs) | Separate high- and low-side PWM commands for the 4-switch synchronous buck-boost stage (`Q1–Q4` via dual isolated gate drivers). |
| Gate-driver enable | Output | 1 shared | Shared command to enable or disable the bridge gate drivers through the host board; must default to disabled on reset, power loss, or card removal. |
| Inductor current | Input | 1 | Conditioned analog measurement of main inductor current (`ACS37002` / `ACS372` or equivalent). |
| Battery & 96 V bus currents | Input | 2 | Separate conditioned analog measurements of battery-side DC current and 96 V bus-side DC current. |
| Battery pack voltage | Input | 1 | Conditioned analog measurement of battery-side voltage. |
| 96 V bus voltage | Input | 1 | Conditioned analog measurement of the 96 VDC energy bus, including for pre-charge assessment and protection. |
| Heatsink temperature | Input | 1 | Analog heatsink temperature sensor. Define sensor type, placement, and conditioning. |
| Hardware trip | Input | 1 common HRTIM fault input | Combines current-sensor overcurrent outputs and gate-driver faults into a common hardware trip to shut down all 4 PWM outputs. |
| Pre-charge contactor command | Output | 1 | Command a host-board driver for the 96 V bus pre-charge contactor (do not drive the 24 V coil directly). |
| Main isolation contactor command | Output | 1 | Command a host-board driver for the 96 V bus main isolation contactor. |
| Contactor feedback | Input | 2 | Separate feedback inputs to verify Pre-Charge and Main Isolation contactor states. |
| Cooling fan speed control | Output | 1 | PWM fan-speed command to a suitable host-side fan interface. |
| Cooling fan tachometer | Input | 1 | Fan tachometer feedback. Define conditioning, levels, and timing. |
| DALY BMS communication link | Input / output | 2–3 (UART TX/RX + optional RS485 DE/RE) | Confirmed primary interface: UART (or RS485 via a host-side isolated transceiver) for maximum compatibility with standard COTS DALY Smart BMS units. |

In summary, the battery module currently identifies **4 PWM outputs**, **6 analog measurements** (inductor current, battery current, 96 V bus current, battery voltage, 96 V bus voltage, heatsink temperature), **1 common hardware trip**, **1 shared gate-driver enable**, **2 contactor outputs + 2 contactor feedbacks**, **cooling fan PWM + tachometer**, and **2–3 BMS communication signals**.

CAN (system bus), display/keypad, SWD, and card power are common card functions, not additional battery host I/O.

## Hardware Boundaries

- The host supplies isolated gate drivers, power-stage sensing and isolation, BMS bus isolation, and 24 V contactor drivers. Never connect battery pack voltage, 96 V bus, or inductive coil loads directly to MCU pins.
- Keep the DALY BMS isolated from the control card's logic domain and strictly separate from the System Coordinator CAN bus; the STM32 acts as the local gateway.
- Route current-sensor overcurrent outputs and gate-driver faults to the common HRTIM hardware trip so switching disables in hardware without software latency. Provide an independent host-side safe-disable state for card reset, loss of power, or card removal.

## Open Hardware Allocation Questions

- Confirm whether the battery-side and 96 V-side current sensors also feed the common hardware overcurrent trip.
