# Nucleo-G474RE Boost 0 CAN Simulator

Standalone firmware project for the ST Nucleo-G474RE. It emulates the reopened Boost 0 profile in [`../../../protocol/CAN_Messages_Definitions.md`](../../../protocol/CAN_Messages_Definitions.md), Section 3.6.1. This project is grouped with the other Nucleo simulators under [`../`](../).

The firmware uses FDCAN1 in Classical CAN mode at 500 kbit/s. It sends Boost telemetry, node heartbeat, and identity frames; receives Coordinator heartbeats and control commands; and returns simulated ACKs. Commands only update in-memory simulated state and telemetry. The firmware does not configure PWM, ADC, contactor, or other power-stage outputs.

The PlatformIO target has been built successfully. Physical flashing and CAN-bench behavior have not yet been verified because no Nucleo/ST-LINK is currently connected.

## Required hardware

- ST Nucleo-G474RE board, connected to the PC through its on-board ST-LINK USB connector for flashing.
- External **galvanically isolated CAN transceiver**. The Nucleo-G474RE provides the MCU's FDCAN peripheral, not a CANH/CANL transceiver.
- Isolated CAN bench with 120-ohm termination at the two physical ends. Do not connect this emulator to energized power equipment.

FDCAN1 uses `PB8` as RX and `PB9` as TX (Arduino-header `D15` / `D14`). Connect these to the transceiver's logic-side RXD/TXD, provide the transceiver's required logic-side supply, then connect its isolated bus-side CANH/CANL to the test bus. Keep the Nucleo logic ground on the transceiver logic side; do not defeat the CAN isolation. Verify the transceiver module pinout and supply requirements before wiring.

The on-board ST-LINK virtual COM port uses USART2 (`PA2` TX / `PA3` RX) at 115200 baud for startup and error messages. The green LD2 LED toggles with each node heartbeat.

## Build and flash with VS Code

1. Install the **PlatformIO IDE** extension. In VS Code, open Extensions (`Ctrl+Shift+X`), search for `PlatformIO IDE` by PlatformIO, and install it. The C/C++ extension is optional; PlatformIO supplies the embedded toolchain and project integration.
2. Open `firmware\nucleo_simulators\boost` as the PlatformIO project folder. On the first build, PlatformIO downloads the STM32 platform, STM32Cube HAL, and ARM compiler.
3. Connect the Nucleo over USB, then select **PlatformIO: Build** from the Command Palette.
4. Select **PlatformIO: Upload** to build and flash over the on-board ST-LINK. If multiple upload ports are present, set the correct port in the PlatformIO status bar or project environment.
5. To view logs, open the PlatformIO serial monitor at 115200 baud.

The project can also be built from a terminal with `pio run` and flashed with `pio run -t upload`, with the project folder as the working directory.

## Simulator behavior

- Standard 11-bit Classical CAN, 500 kbit/s, 8-byte payloads.
- Transmits identity (`0x706`) at boot, 2-phase FB-LLC fast telemetry (`0x300`, `400 V` nominal split-rail) every 50 ms, slow LLC diagnostic telemetry (`0x310`, `45.00 kHz` / tank RMS / split-rail balance) every 1 s, and node heartbeat (`0x606`) every 100 ms.
- Receives Coordinator heartbeat (`0x100`) and Boost control (`0x126`). Coordinator timeout is reported after 500 ms.
- Valid control CRC uses CRC-8/SAE-J1850 over bytes 0-6. Invalid CRC is dropped without an ACK. Supported and unsupported actions follow the reopened Boost profile; state-changing command ACKs use `0x765`, while no-op commands do not produce an ACK.
- Telemetry values are fixed/simulated, including `30000 mV` for the emulated auxiliary supply. `Power_Stage_Profile_ID` is `0x00` (no physical power stage), and identity capability flags are zero.
- The default test serial is `0x4E554300`. Change `BOOST_SIM_SERIAL_NUMBER` in `platformio.ini` before using another simulator so every node has a unique test serial.

The Coordinator firmware currently does not decode Boost frames or send node commands. This board can be built and flashed independently, but end-to-end testing requires the planned Coordinator Boost 0 support and an isolated CAN transceiver/bus.
