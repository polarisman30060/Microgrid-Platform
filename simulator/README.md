# PC Battery Node Simulator

This Windows desktop application uses the Waveshare USB-CAN-A as a serial-to-CAN adapter and emulates one configured Battery Node (`GlobalNodeID 0x10`). The Coordinator sees ordinary Battery identity, heartbeat, telemetry, and ACK frames; no simulator-specific Coordinator mode is used.

## Run

1. Connect the USB-CAN-A to an **isolated communications bench** with no power hardware attached.
2. Use Waveshare's configuration tool to select **variable-length serial framing**, **standard 11-bit CAN**, **normal mode**, and **500 kbit/s** CAN. The app uses **2,000,000 baud, 8N1** for the USB serial connection and does not change adapter CAN settings.
3. Run `run_simulator.bat`. It creates `simulator\.venv`, installs `pyserial`, and opens the desktop interface.
4. Select the adapter COM port, check the isolated-bench confirmation, and connect.
5. Start the Battery Node. Use the controls to edit telemetry, inject/clear a simulated local fault, or suppress/restore all node frames to test the Coordinator's loss detection.

The window displays Coordinator heartbeat freshness, Battery state and measurements, CAN TX/RX frames, and simulator events. If Coordinator heartbeats stop for 500 ms, the simulated Battery enters degraded-autonomous state and sets the Coordinator-timeout warning; when heartbeats return, the warning clears but the local state remains degraded until changed in the UI. The initial periodic frames are identity on startup/every 1 s, heartbeat every 100 ms, fast telemetry every 50 ms, and slow telemetry every 250 ms.

## Tests

From this directory, run:

```powershell
python -m unittest discover -s tests -v
```

The protocol and Battery model tests do not require a USB-CAN adapter or `pyserial`.

## Scope and safety

This is a communications emulator and early node model, not an electrical microgrid simulator. It does not model power flow or energize real contactors or power stages. The configured Boost 0 profile is frozen for isolated simulator and compatible-firmware development, but the wider CAN contract and Battery 0 profile remain draft. Invalid Battery command CRCs are dropped without an ACK as the conservative response to the unresolved project-wide CRC-error rule.
