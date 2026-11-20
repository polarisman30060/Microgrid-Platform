# STM32 Control Card: EasyEDA Pro Walkthrough

Status: **Learning and preliminary placement only. Not a complete schematic, verified PCB, or authorization to order or energize a board.**

We will build this in stages: schematic first, then a mechanical placement mock-up, then electrical routing after the open design decisions are resolved. EasyEDA Pro's menus can vary by version; the names below follow its [official editor FAQ](https://prodocs.easyeda.com/en/faq/editor/). This guide uses the **Pro** edition, not EasyEDA Standard.

## What goes on this card

```text
  Display/keypad HAT, mounted above the card with standoffs
  +-------------------------------------------------------+
  |  CAN connector       isolated CAN area                |
  |                  isolation boundary                   |
  |  24 V input   5 V / 3.3 V supplies       SWD + SWO   |
  |              MCU + clocks + decoupling                |
  |  J1: power / analog         J2: PWM / fault / sync   |
  +-------------------------------------------------------+
         J1 and J2 mate downward to the host board
```

This sketch shows functional neighborhoods, not a dimensioned or approved layout. The HAT connector and its mounting holes must share the measured HAT geometry. The actual locations of J1, J2, J3, CAN and J5 depend on the host board, enclosure, component heights and accessibility.

## Before starting

Keep these open:

1. [Architecture](./STM32_Control_Card_Architecture.md), [draft pinout](./STM32_Control_Card_Pinout.md), [power and mechanics](./STM32_Control_Card_Mechanical_and_Power.md), and [draft BOM](./STM32_Control_Card_BOM.md).
2. The [STM32 datasheet](../../docs/datasheet/stm32g474qe.pdf), component datasheets and the [Waveshare HAT schematic](../../docs/datasheet/Waveshare_1_3inch_LCD_HAT_Schematic.pdf).
3. If available, the actual display HAT, mating headers, debugger and calipers. Measure the HAT hole centers, connector position and mating height instead of relying on the draft dimensions.

The pinout, footprints, BOM prices/links, debugger pinout and mating connectors are **candidates**, not checked manufacturing instructions. The Nucleo's exact clock sources and the equivalence of the proposed crystals have not been established.

## Session 1: create a project and learn the editor

1. Open [EasyEDA Pro](https://pro.easyeda.com/editor) or its desktop client. Create a new project named `STM32_Control_Card`. Create/open a board and its schematic. If needed, the Pro FAQ describes **New > Board**, which creates a board, schematic and PCB.
2. Save your work. For a local project, choose a location under `hardware/STM32_control_card` when the client permits. In the online editor, export a native project backup into that folder: cloud saves alone are not files in this repository. EasyEDA distinguishes offline `.eprj` projects from export/import formats; see its FAQ for your chosen mode. Do not commit generated fabrication files as if they were a verified design.
3. In the schematic, open the device library with **Shift+F**. Search `STM32G474QET6`. Place one candidate device on the drawing. Pro uses a *device* combining a schematic symbol, footprint and properties.
4. Select it and inspect its properties and footprint. Compare the full part number, LQFP128 package drawing, pin 1 orientation, pad numbers and symbol-to-pad mapping to the STM32 datasheet. A similar-looking LQFP128 footprint is not enough. If you cannot verify a library item, leave it marked unverified or create/correct a device; do not treat a library listing as a guarantee.
5. Add a candidate 1x6, 2.54 mm debug connector and label it `J5`. Check its footprint and pin numbering. The intended signals are target 3.3 V reference, SWCLK, control ground, SWDIO, NRST and SWO. The draft pin order is in the pinout document, but compare **each pin** with your specific ST-LINK/debugger and cable, or design a clearly labeled adapter. Pin 1 is a voltage reference, not permission to power the card from the debugger.
6. Practice drawing a wire from one symbol pin. To name a net, draw a wire first, then use **N** or edit the wire's name property. Identically named nets connect electrically: use this deliberately and check spelling. Wires must actually terminate on pin endpoints; a line that merely looks close is not connected.
7. Save. At this point the MCU and connector are *practice placements*, not a working circuit. Do not wire up the draft pin table wholesale until it is independently checked.

### How to check a footprint

For each real part: compare pad count and numbers to the manufacturer's drawing, pin-1 mark and orientation, pitch, body outline, pad and drill sizes, and courtyard/clearance. For polarized parts check polarity. For through-hole headers compare row numbering and mating-side orientation, not just 2.54 mm pitch. Confirm that the footprint is suitable for the intended side of the PCB. For fine-pitch MCU pads, check the manufacturer-recommended land pattern and your hand-soldering plan.

## Session 2: capture the complete schematic

Work in these functional groups, using separate schematic sheets if useful. Keep the sheet interfaces and net names consistent, and verify how your Pro version connects named nets across sheets.

| Group | What to capture and check |
| --- | --- |
| 24 V input and local power | Host input, protection, 24 V-to-5 V converter, 3.3 V regulator and their required supporting components. Derive ratings and component values from the actual host range, transient specification, load and device datasheets. |
| MCU support | Every supply/ground pin, VDDA/VREF, decoupling, reset, boot, clocks and status LED. Verify each physical pin and crystal circuit; choose clock loading and test startup rather than assuming the Nucleo uses the same crystals. |
| Debug | J5 with SWDIO, SWCLK, SWO, NRST, ground and target-voltage reference; check debugger compatibility and avoid back-powering. |
| Display and keypad | J4's HAT power, grounds, SPI/control and eight switch signals, matching the actual HAT schematic and physical pin numbering. Verify unused/5 V connections, backlight control and input conditioning. |
| Isolated CAN | Logic-side supply and signals, isolated DC/DC, CAN-side supply/return, transceiver, protection, connector and selectable end termination. Keep the two ground nets distinct; specify the entire barrier, not just the chip. |
| Host and expansion | J1, J2 and J3: verify every MCU pin, AF, connector position, net direction, voltage, return and safe default against each module. Define fault and gate-disable hardware on the host independent of firmware. |

Use the draft BOM only to find candidates and local datasheets. Add or correct parts and quantities from the **completed schematic**, then verify supplier links and dated prices before purchase. Identify deliberately unused pins using the editor's no-connect mechanism, rather than leaving ambiguous dangling connections.

**Stop points before treating the schematic as complete:**

- The proposed TVS has a listed maximum clamp of 53.3 V while the buck has a 40 V operating ceiling. The host transient envelope and a protection design that actually meets the regulator limits are unresolved.
- A DC isolation test rating does not prove a specific AC rating, reinforced insulation, or working-voltage suitability. Select and validate the CAN converter, transceiver, PCB, connector and protection as one barrier against the applicable requirements.
- The draft MEJ1S0505SC CAN converter has a 20 mA minimum load for its specifications and can rise well above 5 V at lighter load. The ISO1042 draws much less than 20 mA in its specified recessive condition. Do not connect the two directly without a validated solution for the full input, load, and startup range; see the [power draft](./STM32_Control_Card_Mechanical_and_Power.md#2-isolated-can-bus-interface).
- Confirm independent host-safe gate disable, trip/latch behavior and off states during reset, loss of power and card removal. An MCU output or software alone is not the safety system.
- Independently verify the pin table, actual display and host header mapping, debugger wiring, all power connections, crystal choices, component ratings and connector mating. The draft has not had a complete independent review.

Run the schematic electrical rule check (ERC) in your Pro version; resolve errors and review intentional exceptions. Review the resulting netlist and every connector pin manually. ERC cannot prove the circuit is safe, that a pin's alternate function is right, or that a footprint matches its real component.

### Saved schematic progress, 2026-11-20

The saved EasyEDA project contains three sheets: MCU and headers, decoupling and filtering, and CAN. A limited inspection of the saved records found the expected labels on the ISO1042, UCC33421, CAN terminal and NUP2105L pins; the NUP2105L symbol assigns pin 3 to the isolated return and pins 1 and 2 to CANH and CANL. The saved split-termination wiring places the jumper in series with the two resistors and the capacitor between their midpoint and isolated return. The saved MCU display clock and data assignments PG2/PG4 support SPI1 SCK/MOSI according to the MCU datasheet.

This is not a full netlist or ERC review. The 24 V input, 5 V and 3.3 V supplies, clocks, and host/expansion headers still need schematic capture. Check all cross-sheet connectivity, connector mating, symbol-to-footprint assignments, CAN surge protection and isolation, effective converter capacitance, and the complete power and safety design before PCB release or power-up. The CAN termination jumper is fitted only at a physical bus end.

## Session 3: make a mechanical placement mock-up

This is useful before the circuit is finished, but it is **not** a production PCB. In the PCB document, use verified or clearly marked provisional footprints. When the schematic is ready, use the editor's schematic-to-PCB/update function to synchronize devices and nets; do not independently create duplicate production components in the PCB. Check the update preview and unresolved items.

1. Set the units to millimeters. The draft proposes a **four-layer, approximately 95 x 70 mm** card. Draw a closed board outline at that size only as a starting envelope; it is not an approved fit. Confirm the final board size and origin with the host and enclosure.
2. Start with **J4 and the four HAT mounting holes**. Measure the HAT and transfer its hole centers and connector location as a single mechanical pattern. Check the pad/drill sizes, screw head and standoff clearance, connector engagement, underside component heights and button access. Holes may need copper clearances appropriate to the fastener; don't assume all standoffs are insulating.
3. Position the candidate J1 and J2 on the **bottom** for mating to the host. A bottom-side view can appear mirrored relative to a top view: compare pad 1 and every row against the connector drawing **and the actual mated host socket**, not a screen image alone. Verify that the specified connector pair actually mates, including shrouds, pin length, polarization, retention and board spacing. Ensure the two headers can be plugged in together without mechanical stress.
4. Place J3, the CAN connector and J5 so they are usable in the enclosure and do not collide with the HAT, standoffs or host. The unkeyed J5 draft needs clear pin-1/signal markings and a checked cable or adapter.
5. Place the MCU, its decoupling capacitors and clocks as a group. Keep each supply capacitor close to its relevant MCU supply/return pins with a short return path. Place crystal components close to their oscillator pins and keep noisy switching traces away, following the MCU and oscillator layout guidance.
6. Reserve a compact area for the 24 V input and buck, away from clocks, analog inputs and the display. Follow the actual regulator datasheet's recommended placement and switching-current loops once the power design is validated. Place the analog interface away from the buck and PWM paths.
7. Reserve a CAN-side region beside the CAN connector. Keep control-side and isolated-side copper, traces and components in their proper domains on **every layer**; make the isolation components bridge the boundary only as their verified footprints permit. Determine all creepage and clearance requirements before setting keep-outs; do not use a generic gap as proof of compliance.
8. Use the 2D and 3D views, if available, to inspect top and bottom; 3D models may be missing or wrong. Print the board outline and footprints **at 1:1**, check the print scale with a ruler, and compare the real HAT, connectors and hardware. Also check the assembled heights and host-side mating in a mechanical drawing or model.

Keep J1 analog returns and J2 fast signals close to their intended return paths. Plan host-side routing as well as the card; physical separation alone does not guarantee low noise. Do not place circuitry in the HAT/standoff clearance envelope without checking real dimensions.

## Session 4: rules, routing and review

Only begin final routing once the schematic, interfaces, isolation requirements, stackup and mechanical fit are reviewed.

1. In PCB, open **Design > Design Rules**. Enter the chosen manufacturer's capabilities and calculated project requirements for width, clearances, drills and board edge. Set stricter net/region rules where needed, especially across the isolation boundary. Manufacturer minimums are not safety requirements.
2. Define the actual four-layer stackup with the PCB manufacturer. The draft proposes top signals, an inner control-ground reference, an inner power/ground layer and bottom signals. Review return paths and isolation on every layer; avoid routing signals over plane gaps. Keep control and isolated CAN grounds separate unless a separately engineered connection is required.
3. Place and route the power supply's critical loops per its datasheet; route clock, analog, fault and PWM/return paths carefully; route display/debug and remaining I/O. Review trace widths for current and temperature rise, not just whether they fit. Ensure the fault/safe-disable design remains independent of software and communication.
4. Add appropriate copper pours, clearances and thermal connections; refill all pours. Inspect every layer at the isolation barrier, including vias, through-hole pads, planes, silkscreen and nearby mounting hardware. PCB clearances alone do not measure all creepage paths through or around physical components.
5. Run PCB design rule check (DRC), resolve violations and inspect unconnected nets, shorts, clearances, drill sizes and the outline. Check schematic-to-PCB consistency, pin-1 and polarity marks, 1:1 print and actual connector mating. DRC checks configured rules, not whether those rules or the design are safe.
6. Have the schematic, layout, host interfaces and safety assumptions independently reviewed. Only after approval, export fabrication files via **Export > PCB Fabrication File Gerber** and inspect them in an independent Gerber/drill viewer. Confirm layer count, outline, drills, units, copper and isolation keep-outs with the board manufacturer.

## First practical milestone

Create the project, place the candidate MCU and J5, and verify their symbols and footprints against the datasheets. Save a native backup in this project folder. Then share a screenshot of the schematic and the MCU device/footprint properties so we can work through the pin and support circuit checks together before drawing the rest.

## References

- [EasyEDA Pro editor FAQ](https://prodocs.easyeda.com/en/faq/editor/) and [Pro introduction](https://prodocs.easyeda.com/en/introduction/introduction/).
- [Control-card architecture](./STM32_Control_Card_Architecture.md), [pinout](./STM32_Control_Card_Pinout.md), [power and mechanical draft](./STM32_Control_Card_Mechanical_and_Power.md), and [BOM](./STM32_Control_Card_BOM.md).
