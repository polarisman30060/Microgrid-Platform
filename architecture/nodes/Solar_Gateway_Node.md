# Solar Gateway Node

## Status

Rough architecture proposal. The registry assignments and traffic periods referenced here are working allocations; the gateway payloads, electrical interface, and production hardware are not approved.

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

## Preliminary Hardware Recommendation

### Prototype

Use the existing NUCLEO-G474RE development platform as the MCU evaluation base, paired with an interface carrier that provides two electrically independent CAN paths: one for the main bus and one for the local Solar segment. Configure both for Classical CAN at the project's provisional `500 kbps` bitrate. The Nucleo board is a firmware-development platform, not the finished gateway electrical design.

The STM32G474RE is a reasonable prototype MCU candidate because it is in the STM32G4 family already used by the module firmware and supports FDCAN operation in Classical CAN mode. Before committing a schematic, verify the selected package's independent FDCAN instances, pin mapping, clocks, and board-level pin availability against the current ST datasheet.

### Production Direction

For a production candidate, carry the selected STM32G4 MCU onto a dedicated gateway PCB with:

- Separate CAN transceiver channels for the main bus and Solar-local bus.
- Galvanic isolation appropriate to each bus boundary, including the isolated power required by the chosen transceiver implementation.
- A protected low-voltage auxiliary input compatible with the system's control-power design.
- Per-segment termination configurable only when the gateway is physically at a segment end.
- Hardware provisions for reset, watchdog, debug, and clear bus/service indicators.

Exact transceiver part numbers, isolation ratings, connector pinouts, enclosure, environmental range, and auxiliary-power conversion are schematic-level decisions. They require component datasheet review and electrical validation; this document is not a safety or compliance approval.

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
- [ST STM32G474RE product and datasheet page](https://www.st.com/en/microcontrollers-microprocessors/stm32g474re.html)
- [ST NUCLEO-G474RE evaluation board](https://www.st.com/en/evaluation-tools/nucleo-g474re.html)
