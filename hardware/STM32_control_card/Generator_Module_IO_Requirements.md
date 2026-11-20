# Generator Module Control-Card I/O Requirements

Status: Draft functional inventory for the common control card. Signal counts, gate-driver architecture, engine interface details, connector positions, and STM32 pin assignments are not finalized.

## Purpose and Scope

Identify the interface between the common STM32 control card and the Integrated Starter-Generator power and engine-control board described in the [generator hardware baseline](../Generator_Module_Hardware.md) and [control-card architecture](./STM32_Control_Card_Architecture.md).

The generator module interfaces a petrol-driven 24 V, 200 A alternator core with the 96 VDC energy bus using a 3-phase active rectifier/bridge, a 4-switch bidirectional synchronous buck-boost converter, and rotor field excitation control. It supports cranking/motor mode (`96 VDC -> 24 V`) and generation mode (`24 V -> 96 VDC`).

## Functional Signal Inventory

Directions are relative to the control card. Counts refer to logical signals, not necessarily dedicated MCU pins; the host power board supplies conditioned signals with specified safe voltage ranges and references.

| Function | Direction | Quantity | Status and purpose |
| --- | --- | --- | --- |
| 3-phase active rectifier PWM | Output | 6 (3 high/low pairs) | Separate high- and low-side PWM commands for the 3-phase bridge used in active rectification and cranking. Define gate-driver interface, dead time, and default-off behavior. |
| Bidirectional buck-boost PWM | Output | 4 (2 high/low pairs) | Separate high- and low-side PWM commands for the 4-switch synchronous buck-boost stage bridging the 24 V rail and 96 VDC bus. |
| Rotor field excitation PWM | Output | 1 | PWM command to modulate alternator rotor field excitation. Define driver interface, frequency, current limiting, and default-off behavior. |
| 24 V rail voltage | Input | 1 | Conditioned analog measurement of the 24 V alternator/low-side converter rail. |
| 96 V bus voltage | Input | 1 | Conditioned analog measurement of the 96 VDC energy bus (high side of the buck-boost stage), including for pre-charge assessment and protection. |
| 3-phase alternator AC voltages | Input | 3 | Conditioned analog measurements of the three alternator phase voltages for active rectification and cranking synchronization. |
| 3-phase alternator currents | Input | 3 | Conditioned analog measurements of the three alternator phase currents. |
| Buck-boost inductor / DC current | Input | 1 | Conditioned analog measurement of buck-boost stage current for bidirectional current control and protection. |
| Rotor field excitation current | Input | 1 | Conditioned analog measurement of rotor field excitation current. |
| Heatsink temperature | Input | 1 | Analog heatsink temperature sensor. Define sensor type, placement, and conditioning. |
| Engine coolant temperature | Input | 1 | Analog engine coolant temperature measurement. Define sensor type, scaling, and conditioning. |
| Engine oil pressure | Input | 1 | Analog oil pressure measurement (0–3.3 V conditioned transducer). |
| Hardware trip | Input | 1 common HRTIM fault input | Combines current-sensor overcurrent outputs and gate-driver faults into a common hardware trip to shut down switching outputs. Define fault sources, host logic, latching, and recovery. |
| Gate-driver enable | Output | 1 shared | Shared command to enable or disable all bridge gate drivers through the host board; must default to disabled on reset, power loss, or card removal. |
| Pre-charge contactor command | Output | 1 | Command a host-board driver for the 96 V bus pre-charge contactor (do not drive the 24 V coil directly). |
| Main isolation contactor command | Output | 1 | Command a host-board driver for the 96 V bus main isolation contactor. |
| Contactor feedback | Input | 2 | Separate feedback inputs to verify Pre-Charge and Main Isolation contactor states. |
| Cooling fan speed control | Output | 1 | PWM fan-speed command to a suitable host-side fan interface. |
| Cooling fan tachometer | Input | 1 | Fan tachometer feedback. Define conditioning, levels, and timing. |
| Engine control & status I/O | Input / output | TBD (maximize operational capture) | Reserve I/O to capture all practical operational engine data and control: e.g., engine RPM/crank speed input, ignition/fuel shutoff output, throttle/choke actuator PWM, starter/crank interlock, and additional engine sensors (such as fuel level or exhaust/cylinder temp) as the engine harness is defined. |
| Other host control/status | Input / output | TBD | Identify any additional overtemperature trips or auxiliary status signals required by the host board. |

In summary, the generator module currently identifies **11 PWM outputs** (10 high-speed bridge channels plus 1 field excitation PWM), **13 analog measurements** (including analog oil pressure), **1 common hardware trip**, **1 shared gate-driver enable**, **2 contactor outputs + 2 contactor feedbacks**, **cooling fan PWM + tachometer**, and **TBD engine control/status I/O** to maximize operational telemetry.

CAN, display/keypad, SWD, and card power are common card functions, not additional generator host I/O.

## Preliminary Resource Observations

- **HRTIM capacity:** The 10 bridge PWM outputs (3-phase bridge + 4-switch buck-boost) require 5 HRTIM1 timer units (10 of the 12 `HRTIM1_CH[A..F][1..2]` outputs on STM32G474QET6). Field excitation PWM can use a general-purpose timer or the remaining HRTIM1 unit (`CHF`), subject to pin-mux and timing verification against the [STM32G474 datasheet](../../docs/datasheet/stm32g474qe.pdf).
- **Analog channel count:** 13 analog inputs exceed the 10-channel boost example and must be checked alongside ADC peripheral assignments, simultaneous sampling needs, and pin conflicts with display, CAN, SWD, and HRTIM signals.

## Hardware Boundaries

- The host supplies gate drivers, power-stage sensing and isolation, field-driver stage, engine signal conditioning, and 24 V contactor drivers. Never connect 96 V bus, alternator phase voltages, or inductive coil loads directly to MCU pins.
- Route current-sensor overcurrent outputs and gate-driver faults to the common HRTIM hardware trip so switching disables in hardware without software latency. Provide an independent host-side safe-disable state for card reset, loss of power, or card removal.
- Ensure engine fuel/ignition and cranking outputs default to a safe de-energized state on reset, loss of control power, or card removal.

## Open Hardware Allocation Questions

- What exact engine control and status signals are required (RPM/crank sensor, ignition/fuel shutoff, throttle/choke actuator, starter/crank interlock, fuel level, additional temperatures)?
- What is the electrical interface and timing for the 3-phase voltage/current sensing and buck-boost current sensing?
