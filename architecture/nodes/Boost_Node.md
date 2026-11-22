# Boost Node

## Purpose

The Boost Node is a **10 kW 2-phase interleaved Full-Bridge LLC (FB-LLC) resonant converter** (`2 × 5 kW` stages, `90°` primary phase-shifted) with **center-tapped full-bridge rectification**. It provides reinforced galvanic isolation while transferring energy from the negative-grounded `96VDC Energy Bus` to the earth-referenced `400VDC` (`±200VDC` split-rail) Inverter Bus for downstream AC output inverter modules.

## Responsibilities

- Galvanically isolate and step up energy from the `96VDC Energy Bus` to the `400VDC` (`±200VDC` split-rail) Inverter Bus across two `90°`-interleaved FB-LLC phases
- Regulate the `400VDC` (`±200VDC`) Inverter Bus within safe operating limits (`380.0 V – 415.0 V` normal) using Pulse-Frequency Modulation (`PFM`) above the capacitive-mode floor
- Execute soft-start into uncharged `400VDC` split capacitor banks via Phase-Shift Modulation (`PSM`)
- Monitor `96VDC` input voltage/current, `+200VDC` and `-200VDC` split output rails (total voltage and midpoint balance), per-phase resonant tank currents, switching frequency, and thermal limits
- Maintain equal Phase 1 / Phase 2 current sharing via hybrid `PFM + PSM` trim, shed Phase 2 at light loads (with lead-phase rotation), and enter hardware burst mode at no-load/standby
- Participate in coordinated load balancing when multiple Boost Nodes operate in parallel
- Report operating status, capacity, fast bus telemetry (`50 ms`), slow LLC diagnostic telemetry (`1 s`), and faults to the Microgrid CAN Network

## Not Responsible For

- Managing battery charging or discharging
- Controlling solar harvesting, generator operation, or utility-tie power flow
- Controlling downstream AC inverter waveforms or residential circuit breakers
- Direct cell-level battery monitoring

## Interfaces

- `96VDC Energy Bus` (primary side, `LV_BUS-` bonded to `PE` via system DC GFDI)
- `400VDC` (`±200VDC` Split-Rail) Inverter Bus (secondary side, `HV_MID` bonded to `PE` & AC Neutral)
- Split `24VDC` Common Auxiliary Power Bus (`AUX_LOGIC_24V` / `AUX_COIL_24V`)
- Microgrid CAN Network

## Inputs

- Energy from the `96VDC Energy Bus`
- Control power from `AUX_LOGIC_24V` and safety-interlocked coil/enable power from `AUX_COIL_24V`
- Enable, voltage setpoint, and load-sharing commands from the Coordinator

## Outputs

- Galvanically isolated, regulated bipolar DC power (`+200VDC` / `HV_MID` / `-200VDC`, `400VDC` total) to the Inverter Bus
- Status, fast/slow telemetry, and fault reports to the Microgrid CAN Network

## Operating States

- OFF
- STARTUP
- READY
- RUNNING
- DEGRADED
- FAULT
- LOCKOUT

## Safety Responsibilities

- Maintain reinforced galvanic isolation (`≥ 4 kVrms`) between the `96VDC` primary bus, the `±200VDC` secondary bus, and control logic
- Protect internal power stages from resonant tank overcurrent, bus overvoltage/undervoltage, split-rail voltage imbalance, capacitive-mode frequency excursion, and overtemperature conditions
- Terminate primary MOSFET gating immediately (`< 1 µs` hardware fault path), leveraging series resonant capacitors and transformers to block DC fault feed-through from `96VDC` to `400VDC`
- Autonomously enter a protective state without Coordinator approval when safety limits are exceeded
- Physically isolate from the `96VDC Energy Bus` via main isolation contactors (`Q_ISOL`) when safe operation cannot be maintained

## Coordinator Interaction

- Transmits periodic heartbeats (`100 ms`), fast bus telemetry (`TELEM_BOOST_FAST`, `50 ms`, including active LLC phase mask, LLC operating mode, and split-rail balance status), and slow LLC diagnostic telemetry (`TELEM_BOOST_SLOW`, `1 s`, including switching frequency, split-rail voltage imbalance, Phase 1/2 tank RMS currents, magnetics temperature, and LLC diagnostic flags)
- Receives startup, standby, and parallel current-sharing setpoints (`20 ms` when paralleled) from the Coordinator
- Transitions to deterministic Primary/Secondary fallback behavior if Coordinator communications are lost (`> 500 ms`)

## Fault Conditions

- `96VDC` Bus Undervoltage (`< 84.0 V`) / Overvoltage (`> 108.0 V`)
- `400VDC` Bus Overvoltage (`> 435.0 V`) / Undervoltage (`< 360.0 V`) / Short-Circuit
- Split-Bus Rail Imbalance (`|V_HV+ − |V_HV-|| > 20.0 V`)
- Primary Resonant Tank Overcurrent / Hardware Overcurrent Trip (`HRTIM1_FLT1`)
- Capacitive-Mode Minimum Switching Frequency Trip
- Phase 1 / Phase 2 Current Imbalance Fault
- Heatsink, Transformer, or Resonant Inductor Overtemperature (Snap-Disc Trip)
- Pre-Charge Timeout or Contactor Mirror Feedback Fault
- Coordinator Communication Timeout

## Open Questions

- Exact Primary/Secondary bus voltage droop curve during Coordinator-loss degraded mode
- Optimal light-load phase-shedding hysteresis thresholds (`~35% / 45%` nominal) and burst-mode entry/exit tuning on bench hardware
