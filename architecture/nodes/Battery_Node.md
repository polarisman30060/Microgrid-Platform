# Battery Node

## Purpose

The Battery Node provides a standardized energy storage interface between a battery system and the 96VDC Energy Bus.

## Responsibilities

- Exchange energy with the 96VDC Energy Bus
- Protect the connected battery system
- Control battery charging and discharging
- Determine and report battery operating limits
- Estimate and report battery state

## Not Responsible For

- Determine fleet charging or discharging strategy
- Manage or control other Battery Nodes
- Manage generation sources or system loads

## Interfaces

- 96VDC Energy Bus
- Microgrid CAN Network

## Inputs

- Power requests from the Coordinator
- Energy from the 96VDC Energy Bus

## Outputs

- Energy to the 96VDC Energy Bus
- Battery operating limits to the Microgrid CAN Network
- Battery state information to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Operating Modes

- IDLE
- CHARGING
- DISCHARGING

## Safety Responsibilities

- Protect the connected battery system from operating outside safe limits
- Enter a protective state without Coordinator approval
- Isolate the battery system from the microgrid when safe operation cannot be maintained

## Fault Handling Behavior

- The Battery Node shall autonomously enter a protective state when a safety limit is exceeded.