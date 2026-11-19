# PC Microgrid Simulator Requirements

## Purpose

Define a computer-based microgrid simulator that uses the Waveshare USB-CAN-A to join the Coordinator's CAN bus and emulate the communications of actual microgrid nodes. The first milestone is one Battery Node. The Coordinator shall see an ordinary configured node and shall not require a simulator-specific mode or protocol.

The PC program models node behavior and microgrid energy flows; it does not switch real power. Keep it on an isolated CAN test setup. It does not replace electrical design, protection analysis, hardware-in-the-loop validation, or a certified power-system tool.

**Implementation status:** The first desktop application milestone now covers the Battery 0 communications emulator, USB-CAN-A variable-length serial transport, basic node controls/event injection, and protocol/model tests. The averaged-value electrical energy model, other module types, scenario persistence, and full system state machine are future work; this application does not yet meet all of the model-fidelity or scenario acceptance criteria below.

## Project topology to represent

- **96 VDC energy bus:** Shared connection for battery, solar, generator, and grid-interface nodes.
- **380 VDC inverter bus:** Supplied from the 96 VDC bus through one or more boost nodes; supplies the AC inverter nodes.
- **AC loads and outputs:** Represent 120 V single-phase and 120/240 V split-phase inverter outputs and their connected loads.
- **Auxiliary/control lifeline:** Represent availability and node dependency for startup and communication scenarios. Detailed 50 kHz switching behavior is not required in the initial model.
- **Coordinator and module nodes:** Model supervisory startup/dispatch separately from each node's local operating state and protection limits.

Refer to [`../architecture/System_Architecture.md`](../architecture/System_Architecture.md), the node definitions in [`../architecture/nodes/`](../architecture/nodes/), and the [`../architecture/System_State_MAchine.md`](../architecture/System_State_MAchine.md) for system behavior. Any conflicting or unspecified electrical values must be configurable or explicitly marked unresolved, not silently invented.

## Initial model fidelity

The first release should be a deterministic, averaged-value model suitable for energy flow, bus response, state-machine, and communications testing. Bidirectional CAN integration with the Coordinator is part of the initial development baseline, not a later optional feature.

The initial increment is a single configured Battery Node. Under the current draft, Battery instance `0` has `GlobalNodeID = 0x10` and uses:

- Coordinator command: `0x130` (`CMD_NODE_CONTROL`).
- Battery fast/slow telemetry: `0x200` / `0x210`.
- Battery heartbeat and identity: `0x610` / `0x710`.
- Battery command acknowledgement: `0x750`.

These are draft profile assignments, not simulator-specific messages. Validate and freeze the Battery profile before treating them as production values. The one-battery model should simulate a configurable battery state of charge, operating mode, charge/discharge limits, contactor state, and response to Coordinator commands without controlling physical contactors or power converters.

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
- Make the simulator an ordinary CAN peer for the Coordinator through the Waveshare USB-CAN-A from the first end-to-end milestone. The PC shall send node frames and receive/respond to Coordinator frames on the same bus.
- Treat the USB-CAN-A as a USB-serial-CAN adapter, not as a native PC CAN interface. Keep its serial transport and framing behind an adapter interface so the battery model and message logic can be tested without hardware.
- On Windows, use the Waveshare CH341SER driver and the adapter's COM port. Keep the USB serial baud rate (`2,000,000` by default) separate from the CAN bitrate (`500 kbit/s` for this project).
- Select one USB-CAN-A serial framing mode for the simulator and configure the adapter to match. The initial recommendation is Waveshare's variable-length framing (`0xAA` header, type/DLC, CAN ID, data, `0x55` terminator); Waveshare's fixed 20-byte format is an alternative, not a simultaneous mode. The PC parser must handle packets split or combined across serial reads.
- Configure the adapter for CAN normal mode and standard 11-bit frames. Verify its stored settings before each test session; USB-CAN-A settings can persist across power cycles.
- Reuse approved protocol codecs or shared conformance fixtures rather than reimplementing message layouts separately. Do not encode the currently unresolved CAN application contract as if approved.
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

- The first implementation targets Windows with Python and a Tkinter desktop interface; the UI and transport remain separate from the Battery Node model so the interface can grow as scenario controls are added.
- The initial delivery is an interactive desktop application, with automated tests for the protocol codec and Battery Node model.
- Supply/review electrical ratings, bus capacitances, efficiencies, battery capacity/chemistry parameters, load profiles, droop curves, and protection thresholds.
- Decide which scenario format and plotting/export formats are needed.
- The simulator implements Waveshare variable-length framing only; configure the adapter to match. Fixed 20-byte framing is not supported.
- Validate the selected framing mode and COM-port settings with the official Waveshare tool before implementing or debugging the simulator parser.
- Review and approve the single-Battery CAN profile before implementing its production-format frames.

The initial PC application uses Waveshare variable-length serial framing and `pyserial` for COM-port access. It does not configure the adapter's CAN bitrate or framing mode; set and verify those with Waveshare's configuration tool before each bench session. Run `simulator\run_simulator.bat` to create a local Python environment, install the serial dependency, and start the application.

## Adapter references

- [Waveshare USB-CAN-A product documentation](https://www.waveshare.com/wiki/USB-CAN-A): product features, Windows driver, CAN configuration, and downloadable C/Python demos.
- [Waveshare serial-to-CAN protocol definition](https://www.waveshare.com/wiki/Secondary_Development_Serial_Conversion_Definition_of_CAN_Protocol): variable-length and fixed 20-byte wire formats.
