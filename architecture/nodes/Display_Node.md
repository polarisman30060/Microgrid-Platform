# Display Node

## Purpose

The Display Node provides the human-machine interface (HMI) for the microgrid platform, rendering real-time system topology, module telemetry, and diagnostic controls via both an onboard touchscreen and a remote Wi-Fi web server.

## Responsibilities

- Render real-time energy flow across the `96VDC` Energy Bus, `400VDC` (`±200VDC`) Inverter Bus, and AC outputs
- Display per-module status, Node IDs, serial numbers, battery State of Charge, and active fault alarms
- Provide operator controls for system commissioning, node classification (`Required`, `Optional`, `Ignored`), and requested provisioning of inaccessible Solar MPPT nodes through their segment gateway; gateway routing/payload behavior remains to be defined
- Provide operator-initiated software circuit-breaker reset controls for Inverter Nodes
- Serve a mirrored diagnostic and control interface over local Wi-Fi

## Not Responsible For

- Executing energy dispatch algorithms or fleet power allocation
- Enforcing hardware safety interlocks or module-level fault trips
- Direct low-level CAN bus arbitration management (delegated to Coordinator task/node)

## Interfaces

- Coordinator Node (co-hosted on the Waveshare ESP32-S3-Touch-LCD-7 platform or linked via Microgrid CAN Network `0x700–0x7FF`)
- Local `7-inch` Capacitive Touchscreen LCD
- Local `2.4 GHz` Wi-Fi Network (HTTP/WebSocket Web Interface)
- Split `24VDC` Common Auxiliary Power Bus (`AUX_LOGIC_24V`)

## Inputs

- Aggregated system telemetry, node lists, and fault logs from the Coordinator Node
- Operator touch gestures and remote web client commands
- Control power from `AUX_LOGIC_24V` (Split `24VDC` Common Auxiliary Power Bus)

## Outputs

- Visual telemetry, topology graphics, and alarm notifications to the local touchscreen and remote web clients
- Operator configuration updates, mode requests, and breaker reset commands to the Coordinator Node

## Operating States

- OFF
- STARTUP
- RUNNING
- DEGRADED
- FAULT

## Safety Responsibilities

- Require explicit operator confirmation before executing high-impact actions such as software circuit-breaker resets or node overrides
- Display prominent visual warnings when the system enters `DEGRADED` or `FAULT` states

## Coordinator Interaction

- Exchanges operator commands and system telemetry with the Coordinator Node (co-hosted on the primary ESP32-S3 platform, with `0x700–0x7FF` reserved on the Microgrid CAN Network for standalone remote display panels)

## Fault Conditions

- Touch Controller (`I2C`) Communication Failure
- Display Framebuffer / Memory Allocation Error
- Wi-Fi Interface Initialization Failure
- Loss of Telemetry Stream from Coordinator

## Open Questions

- Authentication and access-control mechanism for remote breaker resets on the Wi-Fi web interface
