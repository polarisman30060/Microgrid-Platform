# Nucleo-G474RE Simulators

Each module simulator has its own PlatformIO project directory under this folder. The five profile projects each have a `src/main.c` entry point and share their implementation in [`shared/src/main.c`](shared/src/main.c); the standalone Boost 0 project remains self-contained under [`boost/`](boost/).

## Module projects

| Module | Project directory | Control / telemetry CAN ID |
| --- | --- | --- |
| Battery 0 | `battery\` | `0x121` / `0x200`, `0x210` |
| Boost 0 | `boost\` | `0x126` / `0x300` |
| Solar MPPT local slot 0 | `solar\` | `0x121` / local `0x400` |
| Generator 0 | `generator\` | `0x12B` / `0x410` |
| Grid Interface 0 | `grid_interface\` | `0x12D` / `0x420` |
| Inverter 0 | `inverter\` | `0x12F` / `0x500` |

Open the desired module directory in VS Code as a PlatformIO project. The Solar image is a local-segment child and must not be attached directly to the main bus. From the repository root, build a specific image with:

```text
pio run -d firmware\nucleo_simulators\battery
pio run -d firmware\nucleo_simulators\boost
pio run -d firmware\nucleo_simulators\solar
pio run -d firmware\nucleo_simulators\generator
pio run -d firmware\nucleo_simulators\grid_interface
pio run -d firmware\nucleo_simulators\inverter
```

Upload only the selected image by adding `-t upload` to its build command. Each project uses a unique default serial number.

## Protocol status and safety

All projects use the working ID registry and provisional timing in [`../../protocol/CAN_Messages_Definitions.md`](../../protocol/CAN_Messages_Definitions.md). Boost 0 has been reopened; these are experimental communications peers, not approved production firmware or conformance fixtures. Their fixed telemetry values and state transitions are illustrative placeholders, not hardware ratings or validated electrical models.

The main-bus registry assignments are unique for the configured quotas. Solar MPPT children use IDs only on a gateway-local segment; Solar gateway routing/aggregation payloads and physical gateway hardware are not implemented. The Display is not included because it has no standalone telemetry/command contract, and the Coordinator has its own ESP32-S3 firmware.

All projects target the ST Nucleo-G474RE. Use an external **galvanically isolated CAN transceiver**; the MCU FDCAN peripheral is not a CANH/CANL transceiver. Use an isolated communications bench with termination at both physical ends. Never connect these simulators to energized power equipment. The firmware configures no ADC, PWM, contactor, or power-stage outputs.
