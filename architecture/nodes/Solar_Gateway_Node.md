# Solar Gateway Node

## Status

Rough architecture proposal; gateway software has not been started. The registry assignments and traffic periods referenced here are working allocations. NUCLEO-G0B1RE is the preferred evaluation board and STM32G0B1RE is the leading cost-down MCU candidate, not a final production hardware approval. Gateway payloads and the electrical interface remain open.

## Purpose

The Solar Gateway connects one isolated local CAN segment containing Solar MPPT nodes to the microgrid main CAN bus. It represents that group to the Coordinator, routes explicitly addressed commands, supervises child-node communications, and reduces local traffic to bounded upstream summaries and requested details.

The registry reserves up to 16 gateways, each serving up to 32 local Solar slots. This provides 512 slots for the 500-node Solar target. A Solar child has no main-bus GlobalNodeID; its local address is `slot + 1` (`1`–`32`).

## Responsibilities

- Maintain independent CAN interfaces for the main bus and the gateway-local Solar segment.
- Advertise the gateway's assigned main-bus identity and heartbeat.
- Discover and supervise configured Solar children on its local segment.
- Route valid, explicitly addressed Coordinator commands to the correct child and relay the corresponding child response.
- Aggregate child telemetry into the reserved gateway summary traffic; send child detail only when requested or paced.
- Report child faults promptly using the gateway's main-bus emergency identifier, while retaining the originating local slot in a future, not-yet-defined fault payload.
- Keep each bus's receive, transmit, timeout, and recovery handling independent so a fault on one segment does not stall the other.

## Not Responsible For

- Performing MPPT, regulating the 96 VDC bus, or replacing a Solar node's local control and protection.
- Transparently repeating or tunneling arbitrary CAN frames between segments.
- Making fast electrical protection decisions that depend on the Coordinator, gateway, or CAN availability.
- Defining final command, reply, summary, provisioning, or child-fault payload bytes in this rough design.

## Topology and Addressing

```text
Coordinator and main-bus nodes
             |
       Main CAN bus
             |
    Solar Gateway (GID 0x38-0x47)
       / independent CAN boundary
  Gateway-local Solar CAN segment
       |       |       |          |
     slot 0  slot 1  slot 2 ... slot 31
```

Each physical CAN segment is separately terminated at its two ends and uses its own transceiver path. Solar-local CAN identifiers may numerically overlap main-bus identifiers; the segments are electrically separate, and the gateway must not forward raw child frames unchanged onto the main bus.

The gateway accepts traffic only for its configured gateway identity and known child slots. It validates destination, message class, and freshness before routing; broadcast child commands and arbitrary frame forwarding remain disabled unless a later protocol revision explicitly defines them.

## Hardware Evaluation and Recommendation

### MCU Candidates

| Candidate | CAN capability | Trade-off and status |
| --- | --- | --- |
| STM32G0B1RE | Two independent FDCAN controllers; each can be configured for Classical CAN at `500 kbps` | Leading cost-down candidate. Cortex-M0+ at up to `64 MHz`, so it has less compute headroom than the G474RE. Validate the complete 32-child traffic workload before production selection. |
| STM32G474RE | FDCAN operation in Classical CAN mode | Higher-performance Cortex-M4 at up to `170 MHz`; aligns with the module MCU family and remains a lower-risk fallback if G0B1 processing or memory headroom is insufficient. |
| STM32G0C1RE | Potential G0-family alternative | Exact RE-package FDCAN count and pin availability were not verified; do not select without checking the orderable-part datasheet and pin table. |

The lower-cost MCU direction matches the gateway's supervisory workload in principle, but the reduced processing headroom is not yet demonstrated. Finalize the orderable MCU/package only after confirming two independent FDCAN instances, alternate-function pins, clock configuration, memory needs, and worst-case traffic behavior against the applicable ST datasheet.

### Preferred Evaluation Board

Use the **NUCLEO-G0B1RE** for initial gateway firmware work. It carries an STM32G0B1RET6 and provides onboard ST-LINK debugging/programming, USB, clocks, buttons/LED, and Arduino/Morpho headers. Both FDCAN signal pairs are available on headers:

| Controller | RX | TX | Notes |
| --- | --- | --- | --- |
| FDCAN1 | PB8 | PB9 | Also mapped to Arduino D15/D14 |
| FDCAN2 | PB5 | PB6 | Also mapped to Arduino D4/D10 |

Reserve these pins for CAN when laying out a shield/carrier. Alternate pin mappings may overlap USB or other board functions; check the board schematic and MCU alternate-function table before wiring.

The NUCLEO board is **not** a CAN bus interface board: it exposes MCU logic-level TX/RX, not CANH/CANL. It has no CAN transceivers, bus termination, or galvanic isolation. The prototype therefore needs a carrier with two separate Classical CAN transceiver channels, plus the isolated power/barriers required by the system's bus-isolation requirements. Select and verify the carrier components against the project's `>2.5 kVrms` isolation requirement; do not infer compliance from a generic isolation claim.

A DigiKey listing showed the NUCLEO-G0B1RE at approximately `US$20.13` and in stock during research. This is a market snapshot, not a quote or guaranteed availability.

### Compact Alternative

The [WeAct STM32G0B1 Core Board](https://github.com/WeActStudio/WeActStudio.STM32G0B1CoreBoard) uses an STM32G0B1CBT6 and provides a compact USB-C core board with basic clocks, regulator, and headers. It has no onboard debugger, CAN transceivers, or galvanic isolation; add an ST-LINK-compatible probe and the same two-channel isolated CAN carrier. It is a possible lower-cost bench option, but the Nucleo is preferred for first bring-up because its debugger and ST board documentation reduce setup friction.

### Production Direction

For a production candidate, carry the MCU selected after prototype measurements onto a dedicated gateway PCB with:

- Separate CAN transceiver channels for the main bus and Solar-local bus.
- Galvanic isolation appropriate to each bus boundary, including the isolated power required by the chosen transceiver implementation.
- A protected low-voltage auxiliary input compatible with the system's control-power design.
- Per-segment termination configurable only when the gateway is physically at a segment end.
- Hardware provisions for reset, watchdog, debug, and clear bus/service indicators.

Exact transceiver part numbers, isolation ratings, connector pinouts, enclosure, environmental range, and auxiliary-power conversion are schematic-level decisions. They require component datasheet review and electrical validation; this document is not a safety or compliance approval.

### Commercial Gateway Screening

The following purchasable products may help with bench evaluation, but none is currently qualified as a drop-in Solar Gateway. In particular, prove application-level child aggregation and verify the complete isolation path before considering any product for connection to the microgrid buses.

| Product | Useful published capabilities | Fit gaps to resolve |
| --- | --- | --- |
| [Ixxat CAN@net NT 420](https://www.hms-networks.com/p/1-01-0332-42000-ixxat-can-net-nt-420) | Four CAN channels; `5 kbit/s`–`1 Mbit/s`; configurable filtering, ID translation, data mapping, multiplexing, and Lua action rules. | Stateful aggregation of values across multiple child frames into the reserved summary frames is not established by the available documentation. The product page's `1 kV DC` / `1 s` isolation statement does not establish compliance with this project's `>2.5 kVrms` requirement. Confirm both points with HMS. |
| [PEAK PCAN-Router Pro FD](https://www.peak-system.com/products/hardware/routers-gateways/pcan-router-pro-fd/) | Six CAN channels; Classical CAN range includes `500 kbit/s`; user-programmable C/C++ logic is promising for custom aggregation. | Galvanic isolation was not specified in the product information reviewed. Verify or provide a qualified external isolation design. Price and current stock were not confirmed. |
| [Enovation Controls CANBridge](https://enovationcontrols.com/products/canbridge/) | Configurable filtering and rate reduction between two CAN networks, including protocol-aware filtering. | Published features do not demonstrate arbitrary payload creation or multi-frame aggregation; bitrate, isolation, price, and current availability need vendor confirmation. |

**Screening conclusion:** No reviewed off-the-shelf unit is verified to satisfy both the gateway's isolation requirement and its custom aggregation behavior. A custom STM32G0B1-based gateway with a purpose-designed isolated CAN carrier is the leading direction; commercial devices remain evaluation candidates only.

## Communication Strategy

The following traffic reuses the working registry and provisional schedule:

| Traffic | Direction | Working behavior |
| --- | --- | --- |
| Gateway heartbeat | Gateway to main bus | Every `100 ms` |
| Solar child heartbeat and telemetry | Children to gateway | Children report locally every `100 ms` |
| Gateway summaries | Gateway to main bus | Two reserved summary frames per gateway every `500 ms` |
| Child detail | Gateway to main bus | On request or paced rotation |
| Child fault | Gateway to main bus | Forward promptly under the gateway's emergency ID; include the child slot once the fault payload is defined |
| Routed command and reply | Coordinator ↔ gateway ↔ child | Reserved main-bus ranges `0x1A0`–`0x1AF` and `0x1B0`–`0x1BF`; route, correlation, timeout, and reply payloads remain TBD |

These are message-ID and rate reservations, not complete wire-protocol definitions. Do not infer payload fields, scaling, command authority, or retry behavior from the table. The main-bus utilization target and the local segment's worst-case load must be tested with final payloads and a 32-child traffic model before accepting capacity.

The gateway shall use bounded receive and transmit queues. It shall not replay expired commands after recovery, and low-priority child detail must not delay heartbeat or fault reporting. Any command retry policy must be idempotent or protected by a defined sequence/correlation scheme before implementation.

## Fault and Degraded Behavior

- Each Solar child remains responsible for its own fast control and hardware fault protection. Gateway or Coordinator loss must not disable those protections.
- On the provisional `500 ms` child-missing deadline, mark the child unavailable locally and surface a degraded/stale indication upstream using a future-defined summary or fault field.
- If the local segment is bus-off or otherwise unusable, continue main-bus heartbeat and report the local-segment failure; do not block main-bus service while attempting local recovery.
- If the main bus is unavailable, continue local child supervision and autonomous Solar operation. Do not execute queued Coordinator commands after they have become stale.
- A gateway reset or loss removes remote visibility and routing for its children. The children must continue only their locally safe behavior; the gateway is not a substitute for child-level protection.
- Recovery shall be bounded and observable. Repeated bus-off recovery must not create uncontrolled frame bursts or replay stale telemetry as current.

## Prototype Verification Gates

1. Demonstrate simultaneous, independent operation of both CAN interfaces in Classical CAN mode at the selected bitrate.
2. Verify galvanic boundaries, transceiver power domains, termination placement, and connector isolation on the actual interface carrier.
3. Simulate 32 children at the planned heartbeat/telemetry rates, then measure local and main-bus utilization, queue depth, latency, and loss using finalized frame payloads.
4. Inject local-bus and main-bus disconnects, bus-off conditions, child timeouts, gateway resets, and stale commands; verify that each side degrades independently and stale commands are not replayed.
5. Verify addressed command routing, response correlation, fault origin identification, and provisioning only after their payload and authorization rules are specified.

## Open Decisions

- Prototype measurements to determine whether STM32G0B1RE has adequate processing and memory headroom or whether STM32G474RE is needed.
- Final two-channel CAN transceiver/carrier design, including isolation rating, isolated power, connector pinout, and termination configuration.
- Whether a commercial gateway can prove stateful child aggregation and meet the complete isolation requirement.
- Final 8-byte child-command and routed-reply payloads, including addressing, correlation, expiry, retry, and acknowledgement behavior.
- Summary and child-detail fields, units, scaling, freshness, and rotation/request policy.
- Local Solar emergency identifiers and forwarded fault payload format.
- Provisioning and discovery for new or mismatched child identities.
- Local-segment electrical requirements, physical topology, termination rules, cable length, and maximum tested loading.
- Selected MCU/package and pin map, transceiver components, isolation/power design, connector, and production qualification requirements.
- A complete simulation model for multiple gateways and all 500 Solar children.

## References

- [Working CAN message definitions](../../protocol/CAN_Messages_Definitions.md)
- [Microgrid communications specification](../../protocol/Microgrid_Communications_Specifications.md)
- [Solar Node specification](./Solar_Node.md)
- [ST STM32G0B1RE product page](https://www.st.com/en/microcontrollers-microprocessors/stm32g0b1re.html)
- [ST STM32G0B1RE datasheet](https://www.st.com/resource/en/datasheet/stm32g0b1re.pdf)
- [ST NUCLEO-G0B1RE board page](https://www.st.com/en/evaluation-tools/nucleo-g0b1re.html)
- [NUCLEO-64 user manual](https://www.st.com/resource/en/user_manual/um2324-stm32-nucleo-64-boards-mb1360-stmicroelectronics.pdf)
- [NUCLEO-G0B1RE board schematic](https://www.st.com/resource/en/schematic_pack/mb1360-g0b1re-c02_schematic.pdf)
- [NUCLEO-G0B1RE DigiKey listing](https://www.digikey.com/en/products/detail/stmicroelectronics/NUCLEO-G0B1RE/14317287)
- [WeAct STM32G0B1 Core Board and hardware files](https://github.com/WeActStudio/WeActStudio.STM32G0B1CoreBoard)
- [ST STM32G474RE product and datasheet page](https://www.st.com/en/microcontrollers-microprocessors/stm32g474re.html)
- [ST NUCLEO-G474RE evaluation board](https://www.st.com/en/evaluation-tools/nucleo-g474re.html)
