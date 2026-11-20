# STM32 Control Card Architecture

Status: Draft design baseline; component selection, pin assignments, electrical ratings, and mechanical details are not yet approved.

## Purpose

Define a reusable STM32 control daughter card for the accessible power-electronics modules in the microgrid. The card plugs into a module-specific host power board and provides local control, isolated CAN communications, programming access, and a directly mounted display and keypad.

The compact, inaccessible solar MPPT controller is outside the scope of this common physical card. The system coordinator remains a separate ESP32-S3 platform.

## Design Philosophy

- Reuse one control card across applicable power modules; keep power conversion, sensing, and gate-drive circuitry on the host board.
- Derive the card's operating supplies locally from the host-provided isolated 24 VDC supply.
- Keep CAN galvanically isolated from the controller and host control domain, using a card-mounted isolated DC/DC converter for CAN-side power.
- Mount the display securely to the control card, rather than relying on a cable.
- Use the STM32G474 Nucleo board as a layout and MCU-support-hardware reference, not as a circuit to duplicate. Review its placement and its use of clocks, reset, and decoupling against the selected MCU's requirements.
- Make usable MCU pins not reserved for the display, CAN, or ST-LINK/SWD available to host modules, subject to the selected package's electrical and alternate-function constraints.
- Keep fast control and protective action local; neither CAN nor the display is a safety dependency.
- Design the card and host interface together, including safe behavior when the card is absent, unpowered, or resetting.

## System Partition and Power

```text
Battery module
  30 V, 50 kHz auxiliary control lifeline
           |
           v
Host module: rectification, smoothing, regulation and isolation
           |
           | isolated 24 VDC
           v
STM32 control card
  input protection --> local conversion --> 3.3 V control/display
                   \-> isolated DC/DC --> isolated CAN-side supply

Host power board <--> PWM, sensing, faults and control <--> STM32
STM32 <--> SWD programming connector
STM32 <--> directly mounted SPI LCD and keypad
STM32 <--> isolated CAN transceiver <--> system CAN bus
```

The battery module generates the 30 V, 50 kHz control-power lifeline. The host module rectifies and smooths this source and supplies regulated, isolated 24 VDC to the control card. The card generates all voltages below 24 VDC required by its own circuitry. Its CAN-side power is separately isolated from the control domain by a DC/DC converter on the card.

The lifeline waveform, meaning of the 30 V rating, source limits, and host supply implementation require definition; do not infer rectifier or regulator ratings from the nominal value. The host must provide control power during the system's intended fault and black-start conditions. A local module fault must not shut down the common lifeline.

## Assumptions and Constraints

- The target controller is the system-standard STM32G474QET6, subject to pin-mux and resource verification across supported modules.
- The exact Nucleo reference board and its MCU package must be identified before comparing layouts or support circuits. Clock sources, decoupling values, and placement must be verified against the STM32G474QET6 documentation rather than copied blindly.
- The target control PCB is four layers, consistent with the [hardware standards](../../docs/Hardware_Standards_and_Control_Power.md).
- The host's nominal 24 VDC supply is isolated, but its tolerance, transient limits, isolation reference, capacity, and startup behavior are not yet specified.
- The 24 V return defines the card's control power domain. Its relationship to host sensing, gate-drive, chassis, and power-stage grounds must be specified per interface; an isolated supply alone does not establish isolation for connected signals.
- CAN uses Classical CAN, standard 11-bit identifiers, and 500 kbps. The current [communications specification](../../protocol/Microgrid_Communications_Specifications.md) calls for greater than 2.5 kVrms galvanic isolation at each node and termination at both physical bus ends. Required working voltage, insulation class, creepage, clearance, and applicable standards remain to be established.
- Existing module documents describe 1.3-inch OLEDs; this card instead targets the Waveshare LCD identified below. Those descriptions have not yet been reconciled.

## Requirements

### Controller and Host Interface

- Provide the HRTIM PWM outputs, analog inputs, hardware fault inputs, enable/control outputs, and other I/O needed by the supported host modules. Establish the common signal inventory and pin allocation before fixing the connector.
- Validate STM32 alternate functions, simultaneous ADC resources, timer channels, fault routing, and pin conflicts against every supported host, including the five-phase boost and inverter designs. No particular STM32 pin assignment is approved yet.
- Inventory the supported modules' required simultaneous functions before allocating pins. Bring remaining usable, unreserved GPIO out to the host interface; supply, ground, reset, oscillator, and other dedicated or required support pins are not general-purpose I/O.
- Define each connector signal's direction, voltage range, reference, current capability, protection, default state, and timing. Define analog accuracy and source-impedance requirements as part of the interface.
- Place gate drivers, contactor coil drivers, power-stage sensors, and any required isolation or signal conditioning on the host unless a later interface decision explicitly assigns them to the card. Do not connect power-stage voltages or 24 V coil loads directly to MCU pins.
- Provide host-side hardware interlocks and a de-energized safe state independent of firmware for loss of card power, reset, disconnection, and hardware trips. Specify and verify the actual gate-driver and contactor behavior for each host; a software command alone is not a protective function.
- Keep cycle-by-cycle fault protection local and independent of CAN, display operation, and software execution. Define fault latching, acknowledgement, and restart behavior per module.
- Identify host connector polarization, retention, ground/return allocation, shielding and routing needs, and card insertion/removal policy before selecting the connector. Do not assume hot-plug capability.

### Card Power

- Accept the host's isolated nominal 24 VDC and regulate all lower-voltage card rails locally, including 3.3 V for the MCU and display. Additional rails are subject to component selection.
- Size conversion, input filtering and protection, thermal design, power sequencing, and brownout behavior from the specified input range, transients, and worst-case load budget. None of these ratings is finalized.
- Prevent uncontrolled back-powering through external I/O, the display, CAN, or a connected programmer when a rail is off. Define startup and shutdown behavior for each power domain.
- Keep analog supply/reference quality and switching noise within the eventual ADC accuracy and EMC requirements.

### Isolated CAN

- Provide an isolated CAN transceiver or equivalent isolated interface, plus a card-mounted isolated DC/DC converter supplying its bus side. Select compatible logic-side and bus-side voltage levels; the bus-side converter must not bridge the isolation barrier through its return or other connections.
- Design the complete isolation barrier, including the transceiver, converter, PCB layout, connectors, and any protection components, to the required working voltage and insulation rating. The component withstand rating alone is not sufficient.
- Provide a way to terminate the bus only when the node is at a physical trunk end. The network has exactly two end terminations; intermediate nodes do not add termination. Define any CAN reference, shield, surge protection, and stub requirements with the network design.
- CAN is supervisory. Loss of communication follows the applicable module's autonomous or degraded behavior and must not disable independent local protection. Application messages and production profiles remain subject to the working protocol specifications.

### Programming and Debug

- Provide a keyed SWD interface with SWDIO, SWCLK, SWO, NRST, control ground, and target-voltage reference. Select the connector and pinout during schematic design.
- Define debugger supply and isolation arrangements and prevent unintended power injection. The debugger ground connects to the control domain; its attachment must not bypass any isolation boundary or expose the operator to hazardous potentials.

### Display and Keypad

The selected [Waveshare 1.3-inch LCD HAT](https://www.waveshare.com/wiki/1.3inch_LCD_HAT) is a 3.3 V, 240 x 240 SPI IPS LCD using the ST7789 controller family. It has three pushbuttons and a five-position joystick. It is an LCD, not an OLED.

| Group | Signals required |
| --- | --- |
| Display | SPI clock, MOSI, chip select, data/command, reset, backlight control |
| Keypad | KEY1, KEY2, KEY3, joystick up, down, left, right, and press |
| Supply | 3.3 V and control ground |

The display interface is write-only SPI according to the vendor documentation; a display MISO connection is not required. The keypad inputs need suitable pull-ups and debouncing, with their electrical behavior checked against the vendor schematic and actual hardware.

The HAT has a Raspberry Pi 40-pin connector. Provide a compatible physical mating interface or a rigid board-mounted adapter, with the required HAT pin positions mapped to the STM32 signals. The vendor's BCM names are Raspberry Pi signal identifiers, not STM32 pins. Confirm the full HAT schematic, connector orientation, 3.3 V and ground pin positions, unused pins, backlight drive, current consumption, and any 5 V connections before wiring; never apply 24 V to the HAT.

Mount the HAT solidly to the card using a verified connector and mechanical supports or standoffs so button operation does not stress the connector. Confirm board dimensions, clearances, access, serviceability, and enclosure alignment against the actual hardware. The display and buttons must not be required for protective action.

## Verification Before Release

- Review the complete host interface and STM32 pin/peripheral allocation against every supported module and the selected component datasheets.
- Measure input range, transients, startup, brownout, power consumption, rail ripple, and temperatures over the specified operating conditions.
- Verify CAN communication, end termination, isolation, and EMC with representative wiring and system equipment. Perform isolation testing to the selected requirements without exceeding component test ratings.
- Verify safe host behavior for power loss, reset, disconnected card, asserted fault, and debugger attachment. Validate local protection independently of firmware and CAN.
- Test display initialization, all eight controls, mounting rigidity, enclosure fit, and operation during relevant electrical noise and thermal conditions.

## References and Parts

- The [control-card master pinout](./STM32_Control_Card_Pinout.md) defines the draft 128-pin `STM32G474QET6` allocation, Waveshare HAT mapping, and `2.54 mm` host headers (`J1`, `J2`, `J3`).
- The [power, isolation, and mechanical baseline](./STM32_Control_Card_Mechanical_and_Power.md) defines the 24 V-to-5 V/3.3 V power tree, isolated CAN circuit, `2.54 mm` connector hardware, and hand-soldering notes.
- The [control-card BOM](./STM32_Control_Card_BOM.md) records selected parts, DigiKey supplier links, and dated prices.
- The [EasyEDA Pro walkthrough](./EasyEDA_Pro_Layout_Walkthrough.md) explains the draft schematic and placement workflow and the checks required before fabrication.
- Local component datasheet copies are stored in the shared [datasheet directory](../../docs/datasheet/); PDFs are not included in Git. See its [README](../../docs/datasheet/README.md) for sources. Verify revisions and errata before final design decisions.
- [STM32G474 datasheet](../../docs/datasheet/stm32g474qe.pdf), DS12288 Rev 6, and [STM32G4 reference manual](../../docs/datasheet/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), RM0440 Rev 9, cover the MCU pinout and peripherals.

## Open Questions

1. What are the 24 V supply minimum and maximum, ripple, transients, current budget, hold-up, and isolation specifications?
2. Which host modules and revisions must the first card support, and what are their exact simultaneous PWM, ADC, fault, and communication requirements?
3. What safety and EMC standards, environmental conditions, working voltages, and isolation ratings apply to the card and each interface?
4. What are the host connector, card dimensions, mounting geometry, display orientation, enclosure clearances, and service access requirements?
5. What are the required CAN transceiver, isolated supply, protection, reference/shield connection, and termination implementation?
6. What are the host fault-latch, reset, and restart requirements, and how are card removal and debugger use controlled?
7. Should optional interfaces or component population variants be supported for modules that do not use every card function?
