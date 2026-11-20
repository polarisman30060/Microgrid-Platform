# PC Microgrid Module Simulator

This Windows desktop application uses the Waveshare USB-CAN-A as a serial-to-CAN adapter and emulates one representative instance of Battery, Boost, Solar, Solar Segment Gateway, Generator, Grid Interface, Inverter, and Display nodes. Each module has its own tab with local start/stop, fault and communications-loss controls, editable telemetry, frame details, and a per-profile command-action table. Start or stop one module or all modules together. It does not yet instantiate the full 500-Solar-node fleet.

The models use the working registry's main-bus GlobalNodeIDs and emit identity, heartbeat, and applicable telemetry frames. They receive Coordinator heartbeats and applicable module commands. Solar MPPT is modeled as a child on a gateway-local segment; its frames are not routed to the PC's main-bus adapter. The Solar Gateway tab currently emits only its identity and heartbeat; aggregation and child-routing payloads are reserved but not implemented yet. Local command injection is available for testing without a Coordinator and never transmits the test command on the physical adapter. The Display profile currently has common identity/heartbeat only because no Display telemetry or command contract is defined.

## Run

1. Connect the USB-CAN-A to an **isolated communications bench** with no power hardware attached.
2. Use Waveshare's configuration tool to select **variable-length serial framing**, **standard 11-bit CAN**, **normal mode**, and **500 kbit/s** CAN. The app uses **2,000,000 baud, 8N1** for the USB serial connection and does not change adapter CAN settings.
3. Run `run_simulator.bat`. It creates `simulator\.venv`, installs `pyserial`, and opens the desktop interface.
4. Select the adapter COM port, check the isolated-bench confirmation, and connect.
5. Start individual modules or choose **Start All Modules**. Edit each module's simulated telemetry, inject/clear faults, or suppress/restore its communications.

The window displays Coordinator heartbeat freshness, each module's state and telemetry, CAN TX/RX frames, and simulator events. Module tabs enumerate the working frame IDs, payload fields, periods, and applicable `CMD_NODE_CONTROL` actions.

### CAN registry and Solar topology

Main-bus IDs are registry-assigned: Battery 0 `GlobalNodeID 0x01`, Boost 0 `0x06`, Generator 0 `0x0B`, Grid Interface 0 `0x0D`, Inverter 0 `0x0F`, Display 0 `0x37`, and Solar Segment Gateway 0 `0x38`. Solar children use local slot/addressing behind their gateway. Since the app has no gateway payload implementation or bus selector, it intentionally does not transmit Solar child frames through the main-bus adapter. The app checks for duplicate IDs within each simulated physical network and blocks physical TX if a conflict is found.

Identity is sent at startup/on request. Battery heartbeat/fast/slow telemetry use `250 ms` / `100 ms` / `1 s`; Boost heartbeat/telemetry use `100 ms` / `50 ms`; Solar child heartbeat/telemetry use `100 ms` on the local segment; Generator and Grid heartbeat/telemetry use `1 s` / `500 ms`; Inverter heartbeat/telemetry use `250 ms` / `200 ms`; Display heartbeat uses `1 s`. The models are deterministic communications/command emulators, not a power-flow or validated electrical model. Illustrative data and state changes do not switch physical contactors or power stages.

## Tests

From this directory, run:

```powershell
python -m unittest discover -s tests -v
```

The protocol and node model tests do not require a USB-CAN adapter or `pyserial`.

## Scope and safety

This is a communications emulator and early node model, not an electrical microgrid simulator. It does not model power flow or energize real contactors or power stages. The CAN IDs and provisional schedule use the working registry in the protocol definition; payloads for gateway aggregation/routing and several other behaviors remain draft. Boost 0 was reopened and now uses its registry-assigned IDs. Invalid Battery and Boost command CRCs are dropped without an ACK; other module behavior remains profile-specific and subject to protocol review.
