# PC Microgrid Simulator Requirements

## Purpose

Define a computer-based microgrid simulator that uses the Waveshare USB-CAN-A to join the Coordinator's CAN bus and emulate the communications of actual microgrid nodes. The first milestone is one Battery Node. The Coordinator shall see an ordinary configured node and shall not require a simulator-specific mode or protocol.

The PC program models node behavior and microgrid energy flows; it does not switch real power. Keep it on an isolated CAN test setup. It does not replace electrical design, protection analysis, hardware-in-the-loop validation, or a certified power-system tool.

**Implementation status:** The desktop application covers local communications emulation for one representative instance each of Battery, Boost, Solar, Solar Segment Gateway, Generator, Grid Interface, Inverter, and Display; the Waveshare USB-CAN-A variable-length serial transport; per-module controls, telemetry editors, frame/action visibility, and protocol/model tests. Profiles use the approved working GlobalNodeID map, and Solar child traffic is identified as gateway-local and blocked from the main-bus adapter. The Gateway tab currently emits only identity/heartbeat; aggregation/routing payloads and hardware are not implemented, and the app does not yet instantiate all 500 Solar children. This is not yet the averaged-value electrical energy model: power flow, scenario persistence, and the full system state machine remain future work, so the application does not yet meet all model-fidelity or scenario acceptance criteria below.

## Project topology to represent

- **96 VDC energy bus:** Shared connection for battery, solar, generator, and grid-interface nodes.
- **400 VDC (`±200 VDC` split-rail) inverter bus:** Galvanically isolated from the `96 VDC` bus through one or more `10 kW` 2-phase FB-LLC boost nodes; supplies the earth-referenced half-bridge AC inverter nodes.
- **AC loads and outputs:** Represent 120 V single-phase and 120/240 V split-phase inverter outputs and their connected loads.
- **Auxiliary/control lifeline:** Represent availability and node dependency for startup and communication scenarios. Detailed 50 kHz switching behavior is not required in the initial model.
- **Coordinator and module nodes:** Model supervisory startup/dispatch separately from each node's local operating state and protection limits.

Refer to [`../architecture/System_Architecture.md`](../architecture/System_Architecture.md), the node definitions in [`../architecture/nodes/`](../architecture/nodes/), and the [`../architecture/System_State_MAchine.md`](../architecture/System_State_MAchine.md) for system behavior. Any conflicting or unspecified electrical values must be configurable or explicitly marked unresolved, not silently invented.

## Initial model fidelity

The first release should be a deterministic, averaged-value model suitable for energy flow, bus response, state-machine, and communications testing. Bidirectional CAN integration with the Coordinator is part of the initial development baseline, not a later optional feature.

The initial Battery profile uses a configured Battery Node. Under the working registry, Battery instance `0` has `GlobalNodeID = 0x01` and uses:

- Coordinator command: `0x121` (`CMD_NODE_CONTROL`).
- Battery fast/slow telemetry: `0x200` / `0x210`.
- Battery heartbeat and identity: `0x601` / `0x701`.
- Battery command acknowledgement: `0x760`.

These are working registry assignments, not simulator-specific messages or production approvals. Battery timing is a `250 ms` heartbeat, `100 ms` fast telemetry, and `1 s` slow telemetry. The current module emulators provide configurable telemetry, local state/fault controls, and command-response behavior without controlling physical contactors or power converters. The simulator still needs the electrical model described below.

- Model power balance and stored energy on the DC buses. Where capacitance values are available, use them to calculate bus-voltage response to a net power imbalance; otherwise make capacitance an explicit scenario parameter.
- Model battery state of charge, usable energy, charge/discharge limits, and conversion efficiency.
- Model solar availability and MPPT output limits, generator start/ramp/cooldown behavior, grid import availability/limits, boost conversion limits, inverter limits, and time-varying loads.
- Enforce conversion limits, bus voltage bounds, operating-state interlocks, and documented source/storage priorities.
- Provide configurable parameters for values absent or disputed in project documents, including efficiencies, capacitances, ratings, response delays, droop curves, thermal behavior, and fault thresholds.
- Use a declared simulation time step and deterministic scenario inputs so results can be reproduced and compared in tests.

Switch-level PWM, semiconductor transients, detailed electromagnetic interference, and sub-microsecond protection validation are explicitly outside this initial model's claims.

## Scenarios and observability

The simulator shall support repeatable scenarios for:

- Cold start, discovery, sequenced bus startup, normal dispatch, and orderly shutdown.
- Battery-node communications: transmit ordinary Battery Node identity, heartbeat, and telemetry frames; receive Coordinator heartbeats and commands; and respond with the normal acknowledgement and simulated state changes.
- Solar and load profile changes, battery charging/discharging, generator dispatch, and grid import.
- Required or optional node loss, stale telemetry, coordinator/CAN loss, and recovery.
- Bus over/undervoltage, converter capacity limits, overload, and node-local fault/lockout behavior.
- Protocol message encoding/decoding and command acknowledgement outcomes after the CAN contract is approved.

Expose simulation time, node states, source/load power, bus voltage, battery state of charge, active limits/faults, and event history. Mark stale or invalid values visibly. Scenario inputs and outputs should be saveable in a documented, versioned format.

## Software boundaries

- Keep the physical/energy model independent of its user interface and of ESP-IDF firmware.
- Keep simulation time deterministic and controllable (pause, step, resume, reset, and accelerated run).
- Make the simulator an ordinary CAN peer for the Coordinator through the Waveshare USB-CAN-A from the first end-to-end milestone. Main-bus profiles send node frames and receive/respond to Coordinator frames on the main bus. Solar child frames remain local to the gateway segment; the PC must not transmit them as if they were main-bus traffic.
- Treat the USB-CAN-A as a USB-serial-CAN adapter, not as a native PC CAN interface. Keep its serial transport and framing behind an adapter interface so the battery model and message logic can be tested without hardware.
- On Windows, use the Waveshare CH341SER driver and the adapter's COM port. Keep the USB serial baud rate (`2,000,000` by default) separate from the CAN bitrate (`500 kbit/s` for this project).
- Select one USB-CAN-A serial framing mode for the simulator and configure the adapter to match. The initial recommendation is Waveshare's variable-length framing (`0xAA` header, type/DLC, CAN ID, data, `0x55` terminator); Waveshare's fixed 20-byte format is an alternative, not a simultaneous mode. The PC parser must handle packets split or combined across serial reads.
- Configure the adapter for CAN normal mode and standard 11-bit frames. Verify its stored settings before each test session; USB-CAN-A settings can persist across power cycles.
- Reuse registry and protocol definitions rather than reimplementing message layouts separately. The Boost 0 profile has been reopened; the registry is working, while gateway payloads and other unresolved protocol behavior remain draft.
- The simulator must not activate physical contactors or power stages. Connect it only to an isolated communications bench, never to a live power system.

## Acceptance criteria

- Identical scenario inputs produce identical outputs and event ordering.
- The Coordinator can discover and communicate with the simulated Battery Node using only the ordinary node protocol, and behaves identically to how it would for a real Battery Node.
- The simulator reports malformed, missing, stale, and unexpected Coordinator messages deterministically.
- At each step, source, storage, conversion-loss, and load power balance within documented numerical tolerance.
- Storage state of charge, converter output, and bus voltage remain within configured bounds unless the scenario deliberately injects a fault.
- Startup and degraded-mode transitions match the approved system state machine; faults remain locally enforceable without Coordinator messages.
- Tests cover load/source steps, limits, missing nodes, communication loss, bad/stale input, and recovery.
- Results clearly state model fidelity and do not claim hardware, switching, or protection validation.

## Decisions still needed

- The implementation targets Windows with Python and a Tkinter desktop interface; the UI, transport, and per-node models are separate modules.
- The current delivery is an interactive multi-module communications simulator, with automated protocol and node-model tests. Electrical simulation and scenario persistence remain future work.
- Supply/review electrical ratings, bus capacitances, efficiencies, battery capacity/chemistry parameters, load profiles, droop curves, and protection thresholds.
- Decide which scenario format and plotting/export formats are needed.
- The simulator implements Waveshare variable-length framing only; configure the adapter to match. Fixed 20-byte framing is not supported.
- Validate the selected framing mode and COM-port settings with the official Waveshare tool before implementing or debugging the simulator parser.
- Complete the Solar gateway child-control, reply, summary, detail, and emergency-reporting payloads before implementing a gateway emulator or physical gateway firmware.

The initial PC application uses Waveshare variable-length serial framing and `pyserial` for COM-port access. It does not configure the adapter's CAN bitrate or framing mode; set and verify those with Waveshare's configuration tool before each bench session. Run `simulator\run_simulator.bat` to create a local Python environment, install the serial dependency, and start the application.

## Nucleo Boost 0 Communications Simulator

The first embedded simulator target is a configured Boost instance 0 on the ST Nucleo-G474RE. Its standalone PlatformIO firmware project builds successfully; physical flashing and bench behavior remain unverified. It uses the working profile in [`../protocol/CAN_Messages_Definitions.md`](../protocol/CAN_Messages_Definitions.md), Section 3.6.1. It sends Boost telemetry, heartbeat, and identity frames, receives Coordinator heartbeat/control frames, and returns simulated ACKs. It must not drive contactors, PWM, or other power-stage controls. Use an isolated communications bench with an external galvanically isolated CAN transceiver; the Nucleo MCU's FDCAN peripheral is not itself a CANH/CANL transceiver. Coordinator decoding of Boost frames and command transmission are prerequisites for full end-to-end testing.

## Additional Nucleo Module Simulators

Firmware images for Battery 0, Solar local slot 0, Generator 0, Grid Interface 0, and Inverter 0 are built from the separate projects under [`../firmware/nucleo_simulators/`](../firmware/nucleo_simulators/). They use the working ID registry and provisional schedule for isolated communications testing only; payloads and module behaviors remain draft. Telemetry values and state transitions are illustrative placeholders, not hardware ratings or validated electrical behavior. The firmware configures no power-stage outputs. Main-bus IDs are registry-assigned uniquely for the approved quotas. Solar is a gateway-local node and must not be put directly on the main bus; the gateway itself has no firmware target yet.

## Adapter references

- [Waveshare USB-CAN-A product documentation](https://www.waveshare.com/wiki/USB-CAN-A): product features, Windows driver, CAN configuration, and downloadable C/Python demos.
- [Waveshare serial-to-CAN protocol definition](https://www.waveshare.com/wiki/Secondary_Development_Serial_Conversion_Definition_of_CAN_Protocol): variable-length and fixed 20-byte wire formats.
