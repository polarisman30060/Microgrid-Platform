"""Draft CAN profiles and deterministic node emulators for non-Battery modules."""

from __future__ import annotations

from dataclasses import dataclass
import math

from .protocol import (
    CanFrame,
    CoordinatorHeartbeat,
    decode_coordinator_heartbeat,
    crc8_sae_j1850,
    encode_node_ack,
)
from .registry import (
    MAIN_BUS,
    SOLAR_SEGMENT,
    NodeCanIds,
    main_bus_global_node_id,
    main_bus_node_ids,
    solar_local_address,
    solar_segment_node_ids,
)

NODE_STATE_NAMES = {
    0: "Off",
    1: "Startup",
    2: "Ready",
    3: "Running",
    4: "Degraded Autonomous",
    5: "Fault",
    6: "Lockout",
}

ACK_ACCEPTED = 0x00
ACK_REJECTED_INVALID_STATE = 0x01
ACK_REJECTED_LOCAL_FAULT = 0x02
ACK_REJECTED_PRECHARGE = 0x03
ACK_REJECTED_CRC = 0x04

COMMAND_ACTIONS = (
    (0x00, "No-op / hold current mode"),
    (0x01, "Precharge and close contactor"),
    (0x02, "Enable power stage / normal run"),
    (0x03, "Disable power stage / standby"),
    (0x04, "Remote software breaker trip"),
    (0x05, "Reset software breaker / latched fault"),
    (0x06, "Generator crank / start sequence"),
    (0x07, "Generator cooldown / stop sequence"),
)


@dataclass(frozen=True)
class TelemetryField:
    key: str
    label: str
    unit: str
    wire_format: str
    default: object
    kind: str = "float"
    minimum: float | None = None
    maximum: float | None = None
    choices: tuple[tuple[str, int], ...] = ()


@dataclass(frozen=True)
class ModuleProfile:
    key: str
    label: str
    node_type: int
    instance: int
    serial_number: int
    telemetry_id: int | None
    telemetry_period_ms: int | None
    fields: tuple[TelemetryField, ...]
    supported_actions: tuple[int, ...]
    slow_telemetry_id: int | None = None
    slow_telemetry_period_ms: int | None = None
    ack_bad_crc: bool = True
    has_ack: bool = True
    network: str = MAIN_BUS
    power_stage_profile_id: int = 0
    capability_flags: int = 0

    @property
    def global_node_id(self) -> int | None:
        if self.network == SOLAR_SEGMENT:
            return None
        return main_bus_global_node_id(self.node_type, self.instance)

    @property
    def local_address(self) -> int | None:
        if self.network == SOLAR_SEGMENT:
            return solar_local_address(self.instance)
        return None

    @property
    def node_can_ids(self) -> NodeCanIds:
        if self.network == SOLAR_SEGMENT:
            return solar_segment_node_ids(self.instance)
        global_node_id = self.global_node_id
        if global_node_id is None:
            raise RuntimeError("Main-bus profiles require a GlobalNodeID.")
        return main_bus_node_ids(global_node_id)

    @property
    def control_id(self) -> int | None:
        return self.node_can_ids.control if self.has_ack else None

    @property
    def heartbeat_id(self) -> int:
        return self.node_can_ids.heartbeat

    @property
    def identity_id(self) -> int:
        return self.node_can_ids.identity

    @property
    def ack_id(self) -> int | None:
        return self.node_can_ids.acknowledgement if self.has_ack else None

    @property
    def emergency_id(self) -> int | None:
        return self.node_can_ids.emergency_fault

    @property
    def tx_identifiers(self) -> tuple[tuple[int, str], ...]:
        identifiers = [(self.identity_id, "Identity"), (self.heartbeat_id, "Heartbeat")]
        if self.telemetry_id is not None:
            identifiers.append((self.telemetry_id, "Fast telemetry"))
        if self.slow_telemetry_id is not None:
            identifiers.append((self.slow_telemetry_id, "Slow telemetry"))
        if self.ack_id is not None and any(action != 0 for action in self.supported_actions):
            identifiers.append((self.ack_id, "Command ACK"))
        return tuple(identifiers)

    @property
    def command_period_ms(self) -> int | None:
        return 20 if self.key == "boost" else None

    @property
    def heartbeat_period_ms(self) -> int:
        return {
            "battery": 250,
            "boost": 100,
            "solar": 100,
            "generator": 1000,
            "grid_interface": 1000,
            "inverter": 250,
            "display": 1000,
            "solar_gateway": 100,
        }[self.key]


def _field(
    key: str,
    label: str,
    unit: str,
    wire_format: str,
    default: object,
    kind: str = "float",
    minimum: float | None = None,
    maximum: float | None = None,
    choices: tuple[tuple[str, int], ...] = (),
) -> TelemetryField:
    return TelemetryField(
        key, label, unit, wire_format, default, kind, minimum, maximum, choices
    )


BATTERY_PROFILE = ModuleProfile(
    key="battery",
    label="Battery 0",
    node_type=0x01,
    instance=0,
    serial_number=0x00001000,
    telemetry_id=0x200,
    telemetry_period_ms=100,
    slow_telemetry_id=0x210,
    slow_telemetry_period_ms=1000,
    supported_actions=(0, 1, 2, 3, 5),
    fields=(
        _field("bus_voltage_v", "96 V bus voltage", "V", "uint16, 0.05 V/bit", 96.0, minimum=0, maximum=3276.75),
        _field("pack_voltage_v", "Pack voltage", "V", "uint16, 0.05 V/bit", 96.0, minimum=0, maximum=3276.75),
        _field("current_a", "Pack current", "A", "int16, 0.1 A/bit", 0.0, minimum=-3276.8, maximum=3276.7),
        _field("soc_percent", "State of charge", "%", "uint8, 0.5%/bit", 80.0, minimum=0, maximum=100),
        _field("operating_mode", "Operating mode", "", "byte 7 bits 0-1", 0, "choice", choices=(("Idle", 0), ("Charging", 1), ("Discharging", 2), ("Fault", 3))),
        _field("main_contactor_closed", "Main contactor closed", "", "byte 7 bit 2", False, "bool"),
        _field("precharge_closed", "Precharge closed", "", "byte 7 bit 3", False, "bool"),
        _field("cell_balancing", "Cell balancing active", "", "byte 7 bit 4", False, "bool"),
        _field("charge_allowed", "Charge allowed", "", "byte 7 bit 5", True, "bool"),
        _field("discharge_allowed", "Discharge allowed", "", "byte 7 bit 6", True, "bool"),
        _field("aux_lifeline_healthy", "Auxiliary lifeline healthy", "", "byte 7 bit 7", True, "bool"),
        _field("min_cell_voltage_mv", "Minimum cell voltage", "mV", "uint16, 1 mV/bit", 3300, "int", 0, 65535),
        _field("max_cell_voltage_mv", "Maximum cell voltage", "mV", "uint16, 1 mV/bit", 3350, "int", 0, 65535),
        _field("max_pack_temperature_c", "Maximum pack temperature", "C", "int8, 1 C/bit", 25, "int", -128, 127),
        _field("converter_temperature_c", "Converter temperature", "C", "int8, 1 C/bit", 25, "int", -128, 127),
        _field("available_capacity_ah", "Available capacity", "Ah", "uint16, 0.1 Ah/bit", 100.0, minimum=0, maximum=6553.5),
    ),
)

BOOST_PROFILE = ModuleProfile(
    key="boost",
    label="Boost 0",
    node_type=0x02,
    instance=0,
    serial_number=0x4E554300,
    telemetry_id=0x300,
    telemetry_period_ms=50,
    slow_telemetry_id=0x310,
    slow_telemetry_period_ms=1000,
    supported_actions=(0, 1, 2, 3, 5),
    ack_bad_crc=False,
    fields=(
        _field("lv_input_voltage_v", "Low-voltage input", "V", "uint16, 0.05 V/bit", 96.0, minimum=0, maximum=3276.75),
        _field("hv_output_voltage_v", "High-voltage output (split total)", "V", "uint16, 0.1 V/bit", 400.0, minimum=0, maximum=6553.5),
        _field("total_input_current_a", "Total input current", "A", "int16, 0.1 A/bit", 0.0, minimum=-3276.8, maximum=3276.7),
        _field("max_stage_temperature_c", "Maximum stage temperature", "C", "int8, 1 C/bit", 38, "int", -128, 127),
        _field(
            "active_llc_phases",
            "Active LLC phases",
            "",
            "byte 7 bits 0-1",
            "Off",
            "choice",
            choices=(("Off", 0), ("Phase 1 Only", 1), ("Phase 2 Only", 2), ("Both (90 deg)", 3)),
        ),
        _field(
            "llc_operating_mode",
            "LLC operating mode",
            "",
            "byte 7 bits 2-3",
            "Standby/Off",
            "choice",
            choices=(("Standby/Off", 0), ("SoftStart PSM", 1), ("Normal PFM", 2), ("Burst LightLoad", 3)),
        ),
        _field("split_rail_balance_ok", "Split-rail balance OK", "", "byte 7 bit 4", True, "bool"),
        _field("main_contactor_closed", "Main contactor closed", "", "byte 7 bit 5", False, "bool"),
        _field("primary_role", "Primary role", "", "byte 7 bit 6", True, "bool"),
        _field("autonomous_droop_active", "Autonomous droop active", "", "byte 7 bit 7", False, "bool"),
        _field("switching_freq_khz", "Switching frequency", "kHz", "slow uint16, 10 Hz/bit", 0.0, minimum=0.0, maximum=655.35),
        _field("split_rail_imbalance_v", "Split-rail imbalance (HV+ - |HV-|)", "V", "slow int16, 0.1 V/bit", 0.0, minimum=-3276.8, maximum=3276.7),
        _field("phase1_tank_current_rms_a", "Phase 1 tank current RMS", "A", "slow uint8, 0.5 A/bit", 0.0, minimum=0.0, maximum=127.5),
        _field("phase2_tank_current_rms_a", "Phase 2 tank current RMS", "A", "slow uint8, 0.5 A/bit", 0.0, minimum=0.0, maximum=127.5),
        _field("magnetics_temperature_c", "Magnetics/rectifier temperature", "C", "slow int8, 1 C/bit", 36, "int", -128, 127),
        _field("psm_balance_trim_active", "PSM balance trim active", "", "slow byte 7 bit 2", False, "bool"),
        _field("magnetics_snap_disc_alarm", "Magnetics snap-disc alarm", "", "slow byte 7 bit 3", False, "bool"),
        _field("thermal_derating_active", "Thermal derating active", "", "slow byte 7 bit 4", False, "bool"),
        _field("capacitive_mode_warning", "Capacitive-mode warning", "", "slow byte 7 bit 5", False, "bool"),
        _field("phase_imbalance_warning", "Phase imbalance warning", "", "slow byte 7 bit 6", False, "bool"),
    ),
)

SOLAR_PROFILE = ModuleProfile(
    key="solar",
    label="Solar MPPT 0",
    node_type=0x03,
    instance=0,
    serial_number=0x4E554330,
    telemetry_id=0x400,
    telemetry_period_ms=100,
    supported_actions=(0, 1, 2, 3, 5),
    network=SOLAR_SEGMENT,
    fields=(
        _field("pv_input_voltage_v", "PV input voltage", "V", "uint16, 0.05 V/bit", 36.0, minimum=0, maximum=3276.75),
        _field("pv_input_current_a", "PV input current", "A", "uint16, 0.05 A/bit", 5.0, minimum=0, maximum=3276.75),
        _field("bus_output_current_a", "96 V bus output current", "A", "uint16, 0.05 A/bit", 0.0, minimum=0, maximum=3276.75),
        _field("enclosure_temperature_c", "Enclosure temperature", "C", "int8, 1 C/bit", 35, "int", -128, 127),
        _field("mppt_mode", "MPPT mode", "", "byte 7 bits 0-1", "Off/Night", "choice", choices=(("Off/Night", 0), ("MPPT Tracking", 1), ("CV Bus Curtailment", 2), ("Snow Melt Reserved", 3))),
        _field("output_connected", "96 V output connected", "", "byte 7 bit 2", False, "bool"),
        _field("curtailment_active", "Curtailment active", "", "byte 7 bit 3", False, "bool"),
    ),
)

GENERATOR_PROFILE = ModuleProfile(
    key="generator",
    label="Generator 0",
    node_type=0x04,
    instance=0,
    serial_number=0x4E554340,
    telemetry_id=0x410,
    telemetry_period_ms=500,
    supported_actions=(0, 3, 5, 6, 7),
    fields=(
        _field("bus_output_voltage_v", "96 V bus output voltage", "V", "uint16, 0.05 V/bit", 96.0, minimum=0, maximum=3276.75),
        _field("bus_current_a", "Bus current", "A", "int16, 0.1 A/bit", 0.0, minimum=-3276.8, maximum=3276.7),
        _field("engine_rpm", "Engine speed", "RPM", "uint16, 1 RPM/bit", 0, "int", 0, 65535),
        _field("alternator_temperature_c", "Alternator/converter temperature", "C", "int8, 1 C/bit", 38, "int", -128, 127),
        _field("generator_mode", "Generator mode", "", "byte 7 bits 0-2", "Standby", "choice", choices=(("Standby", 0), ("Cranking", 1), ("Warmup", 2), ("Generating", 3), ("Cooldown", 4), ("Fault", 5))),
        _field("main_contactor_closed", "Main contactor closed", "", "byte 7 bit 3", False, "bool"),
        _field("low_oil_pressure", "Low oil pressure alarm", "", "byte 7 bit 4", False, "bool"),
        _field("low_fuel_warning", "Low fuel warning", "", "byte 7 bit 5", False, "bool"),
        _field("crank_timeout", "Crank timeout/start fail", "", "byte 7 bit 6", False, "bool"),
    ),
)

GRID_PROFILE = ModuleProfile(
    key="grid_interface",
    label="Grid Interface 0",
    node_type=0x05,
    instance=0,
    serial_number=0x4E554350,
    telemetry_id=0x420,
    telemetry_period_ms=500,
    supported_actions=(0, 1, 2, 3, 5),
    fields=(
        _field("grid_ac_voltage_v", "Grid AC RMS voltage", "V", "uint16, 0.1 V/bit", 120.0, minimum=0, maximum=6553.5),
        _field("dc_bus_output_current_a", "96 V output current", "A", "int16, 0.1 A/bit", 0.0, minimum=-3276.8, maximum=3276.7),
        _field("grid_frequency_hz", "Grid frequency", "Hz", "uint8, 0.1 Hz/bit offset from 50 Hz", 60.0, minimum=50, maximum=75.5),
        _field("heatsink_temperature_c", "Heatsink temperature", "C", "int8, 1 C/bit", 35, "int", -128, 127),
        _field("grid_present", "Grid present and in window", "", "byte 6 bit 0", True, "bool"),
        _field("pll_locked", "PLL phase locked", "", "byte 6 bit 1", True, "bool"),
        _field("ac_input_contactor_closed", "AC input contactor closed", "", "byte 6 bit 2", False, "bool"),
        _field("output_contactor_closed", "96 V output contactor closed", "", "byte 6 bit 3", False, "bool"),
        _field("anti_islanding_trip", "Anti-islanding/grid fault trip", "", "byte 6 bit 4", False, "bool"),
        _field("power_factor_percent", "Power factor", "%", "uint8, 1%/bit", 98, "int", 0, 100),
    ),
)

INVERTER_PROFILE = ModuleProfile(
    key="inverter",
    label="AC Inverter 0",
    node_type=0x06,
    instance=0,
    serial_number=0x4E554360,
    telemetry_id=0x500,
    telemetry_period_ms=200,
    supported_actions=(0, 1, 2, 3, 4, 5),
    fields=(
        _field("ac_rms_voltage_v", "AC RMS voltage", "V", "uint16, 0.1 V/bit", 120.0, minimum=0, maximum=6553.5),
        _field("ac_rms_current_a", "AC RMS current", "A", "uint16, 0.05 A/bit", 0.0, minimum=0, maximum=3276.75),
        _field("active_power_w", "Active power", "W", "int16, 1 W/bit", 0, "int", -32768, 32767),
        _field("ac_frequency_hz", "AC frequency", "Hz", "uint8, 0.1 Hz/bit offset from 50 Hz", 60.0, minimum=50, maximum=75.5),
        _field("breaker_status", "Breaker status", "", "byte 7 bits 0-2", "Open", "choice", choices=(("Open", 0), ("Closed/Normal", 1), ("Tripped Overload I2t", 2), ("Tripped Short Circuit", 3), ("Remote Tripped", 4), ("Locked Out", 5))),
        _field("split_phase_240v", "Split-phase 240 V mode", "", "byte 7 bit 3", False, "bool"),
        _field("sync_locked", "Zero-cross sync locked", "", "byte 7 bit 4", False, "bool"),
        _field("sync_time_master", "Designated sync time master", "", "byte 7 bit 5", False, "bool"),
    ),
)

DISPLAY_PROFILE = ModuleProfile(
    key="display",
    label="Display 0",
    node_type=0x07,
    instance=0,
    serial_number=0x4E554370,
    telemetry_id=None,
    telemetry_period_ms=None,
    supported_actions=(),
    fields=(),
    has_ack=False,
)

SOLAR_GATEWAY_PROFILE = ModuleProfile(
    key="solar_gateway",
    label="Solar Segment Gateway 0",
    node_type=0x09,
    instance=0,
    serial_number=0x4E554390,
    telemetry_id=None,
    telemetry_period_ms=None,
    supported_actions=(),
    fields=(),
    has_ack=False,
)

MODULE_PROFILES = {
    profile.key: profile
    for profile in (
        BATTERY_PROFILE,
        BOOST_PROFILE,
        SOLAR_PROFILE,
        GENERATOR_PROFILE,
        GRID_PROFILE,
        INVERTER_PROFILE,
        DISPLAY_PROFILE,
        SOLAR_GATEWAY_PROFILE,
    )
}


def validate_telemetry_value(field: TelemetryField, value: object) -> object:
    if field.kind == "bool":
        if not isinstance(value, bool):
            raise ValueError(f"{field.label} must be true or false.")
        return value

    if field.kind == "choice":
        valid_values = {choice_value for _, choice_value in field.choices}
        valid_labels = {label for label, _ in field.choices}
        if value in valid_values:
            return value
        if value in valid_labels:
            if isinstance(field.default, int):
                return next(code for label, code in field.choices if label == value)
            return value
        raise ValueError(f"Choose a valid value for {field.label}.")

    if field.kind == "int":
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not float(value).is_integer():
            raise ValueError(f"{field.label} must be a whole number.")
        normalized: int | float = int(value)
    else:
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(float(value)):
            raise ValueError(f"{field.label} must be numeric.")
        normalized = float(value)

    if field.minimum is not None and normalized < field.minimum:
        raise ValueError(f"{field.label} must be at least {field.minimum}.")
    if field.maximum is not None and normalized > field.maximum:
        raise ValueError(f"{field.label} must be at most {field.maximum}.")
    return normalized


class ModuleNode:
    """Deterministic communications emulator for one non-Battery module."""

    def __init__(self, profile: ModuleProfile) -> None:
        self.profile = profile
        self.serial_number = profile.serial_number
        self.power_stage_profile_id = profile.power_stage_profile_id
        self.capability_flags = profile.capability_flags
        self.firmware_build_id = 1
        self.aux_voltage_mv = 30000
        self.local_state = 2
        self.warning_flags = 0
        self.fault_summary = 0
        self.telemetry = {field.key: field.default for field in profile.fields}
        self.running = False
        self.communication_lost = False
        self.started_at_ms = 0
        self.heartbeat_counter = 0
        self.coordinator_heartbeat: CoordinatorHeartbeat | None = None
        self.last_control: tuple[int, int, int, int, int] | None = None
        self.last_command: tuple[int, int] | None = None
        self._next_frame_ms: dict[str, int] = {}
        self._coordinator_timeout_latched = False
        self._output_connected = False
        self._power_stage_enabled = False
        self._generator_transition_deadline_ms: int | None = None

    @property
    def state_name(self) -> str:
        return NODE_STATE_NAMES.get(self.local_state, "Unknown")

    @property
    def command_id(self) -> int | None:
        return self.profile.control_id

    @property
    def tx_identifiers(self) -> tuple[tuple[int, str], ...]:
        return self.profile.tx_identifiers

    def get_telemetry_value(self, key: str) -> object:
        return self.telemetry[key]

    def set_telemetry_value(self, key: str, value: object) -> None:
        field = next((item for item in self.profile.fields if item.key == key), None)
        if field is None:
            raise ValueError(f"{key} is not a field in the {self.profile.label} profile.")
        self.telemetry[key] = validate_telemetry_value(field, value)

    def start(self, now_ms: int) -> None:
        self.running = True
        self.communication_lost = False
        self.started_at_ms = now_ms
        self.heartbeat_counter = 0
        self.coordinator_heartbeat = None
        self._coordinator_timeout_latched = False
        self.warning_flags &= ~(1 << 3)
        self._next_frame_ms = {
            "identity": now_ms,
            "heartbeat": now_ms,
        }
        if self.profile.telemetry_id is not None:
            self._next_frame_ms["telemetry"] = now_ms

    def stop(self) -> None:
        self.running = False
        self._next_frame_ms.clear()

    def set_communication_lost(self, lost: bool, now_ms: int) -> None:
        self.communication_lost = lost
        if not lost and self.running:
            self.start(now_ms)

    def inject_fault(self, lockout: bool = False) -> None:
        self.local_state = 6 if lockout else 5
        self.fault_summary = 1
        self._power_stage_enabled = False
        if self.profile.key == "generator":
            self.telemetry["generator_mode"] = "Fault"
        if self.profile.key == "inverter":
            self.telemetry["breaker_status"] = "Locked Out" if lockout else "Open"

    def clear_fault_locally(self) -> None:
        if self.local_state in (5, 6):
            self.local_state = 2
            self.fault_summary = 0
            self._power_stage_enabled = False
            self._output_connected = False
            if self.profile.key == "generator":
                self.telemetry["generator_mode"] = "Standby"
            if self.profile.key == "inverter":
                self.telemetry["breaker_status"] = "Open"

    def update(self, now_ms: int) -> list[str]:
        events: list[str] = []
        if not self.running:
            return events

        last_heartbeat_ms = (
            self.coordinator_heartbeat_received_ms
            if self.coordinator_heartbeat is not None
            else self.started_at_ms
        )
        if now_ms - last_heartbeat_ms > 500 and not self._coordinator_timeout_latched:
            self._coordinator_timeout_latched = True
            self.warning_flags |= 1 << 3
            if self.local_state in (2, 3):
                self.local_state = 4
            if self.profile.key == "generator" and self.telemetry["generator_mode"] in (
                "Cranking",
                "Warmup",
                "Generating",
            ):
                self._begin_generator_cooldown(now_ms)
            elif self.profile.key == "grid_interface":
                self._power_stage_enabled = False
                self._output_connected = False
                self.telemetry["ac_input_contactor_closed"] = False
                self.telemetry["output_contactor_closed"] = False
            events.append(f"{self.profile.label} Coordinator heartbeat timed out.")
        elif now_ms - last_heartbeat_ms <= 500 and self._coordinator_timeout_latched:
            self._coordinator_timeout_latched = False
            self.warning_flags &= ~(1 << 3)
            events.append(f"{self.profile.label} Coordinator heartbeat restored.")

        if self.profile.key == "generator":
            self._update_generator(now_ms)
        return events

    @property
    def coordinator_heartbeat_received_ms(self) -> int:
        return getattr(self, "_coordinator_received_ms", self.started_at_ms)

    def frames_due(self, now_ms: int) -> list[CanFrame]:
        if not self.running or self.communication_lost:
            return []

        due: list[CanFrame] = []
        if (
            "identity" in self._next_frame_ms
            and now_ms >= self._next_frame_ms["identity"]
        ):
            due.append(self._identity_frame(now_ms))
            self._next_frame_ms.pop("identity", None)

        schedule = (
            ("heartbeat", self.profile.heartbeat_period_ms, self._heartbeat_frame),
        )
        if self.profile.telemetry_id is not None and self.profile.telemetry_period_ms is not None:
            schedule += (("telemetry", self.profile.telemetry_period_ms, self._telemetry_frame),)
        if (
            self.profile.slow_telemetry_id is not None
            and self.profile.slow_telemetry_period_ms is not None
        ):
            schedule += (
                (
                    "slow_telemetry",
                    self.profile.slow_telemetry_period_ms,
                    self._slow_telemetry_frame,
                ),
            )

        for name, period, builder in schedule:
            if now_ms >= self._next_frame_ms.get(name, now_ms):
                due.append(builder(now_ms))
                self._next_frame_ms[name] = now_ms + period
        return due

    def handle_frame(self, frame: CanFrame, now_ms: int) -> tuple[list[CanFrame], list[str]]:
        responses: list[CanFrame] = []
        events: list[str] = []
        if not self.running or self.communication_lost:
            return responses, events

        if frame.identifier == 0x100:
            try:
                self.coordinator_heartbeat = decode_coordinator_heartbeat(frame)
            except ValueError as error:
                events.append(f"Ignored Coordinator heartbeat: {error}")
                return responses, events
            self._coordinator_received_ms = now_ms
            return responses, events

        if self.command_id is None or frame.identifier != self.command_id:
            return responses, events
        if frame.extended or frame.remote or len(frame.data) != 8:
            events.append(f"Ignored malformed {self.profile.label} control command.")
            return responses, events

        data = frame.data
        action = data[0]
        self.last_control = (
            action,
            int.from_bytes(data[1:3], "little"),
            int.from_bytes(data[3:5], "little", signed=True),
            int.from_bytes(data[5:7], "little", signed=True),
            data[7],
        )
        if crc8_sae_j1850(data[:7]) != data[7]:
            events.append(f"{self.profile.label} command CRC was invalid; command not applied.")
            if self.profile.ack_bad_crc and self.profile.ack_id is not None:
                responses.append(self._ack_frame(action, ACK_REJECTED_CRC, now_ms))
            return responses, events

        if action == 0:
            self.last_command = (action, ACK_ACCEPTED)
            events.append(f"{self.profile.label} accepted no-op setpoint update; no ACK is sent.")
            return responses, events

        status = self._apply_action(action, now_ms)
        self.last_command = (action, status)
        if self.profile.ack_id is not None:
            responses.append(self._ack_frame(action, status, now_ms))
        result = "accepted" if status == ACK_ACCEPTED else f"rejected ({status})"
        events.append(f"{self.profile.label} command 0x{action:02X} {result}.")
        return responses, events

    def _apply_action(self, action: int, now_ms: int) -> int:
        if action not in self.profile.supported_actions:
            return ACK_REJECTED_INVALID_STATE
        if self.local_state == 6 and action != 5:
            return ACK_REJECTED_LOCAL_FAULT
        if self.local_state == 5 and action not in (5, 3, 7):
            return ACK_REJECTED_LOCAL_FAULT

        if action == 1:
            self._output_connected = True
            self._set_if_present("main_contactor_closed", True)
            self._set_if_present("output_connected", True)
            self._set_if_present("ac_input_contactor_closed", True)
            self.local_state = 2
            return ACK_ACCEPTED

        if action == 2:
            if not self._output_connected:
                return ACK_REJECTED_PRECHARGE
            self._power_stage_enabled = True
            self.local_state = 3
            if self.profile.key == "boost":
                self.telemetry["active_llc_phases"] = "Both (90 deg)"
                self.telemetry["llc_operating_mode"] = "Normal PFM"
                self.telemetry["switching_freq_khz"] = 45.0
                self.telemetry["phase1_tank_current_rms_a"] = 25.0
                self.telemetry["phase2_tank_current_rms_a"] = 25.0
            elif self.profile.key == "solar":
                self.telemetry["mppt_mode"] = "MPPT Tracking"
            elif self.profile.key == "grid_interface":
                self.telemetry["ac_input_contactor_closed"] = True
                self.telemetry["output_contactor_closed"] = True
            elif self.profile.key == "inverter":
                self.telemetry["breaker_status"] = "Closed/Normal"
                self.telemetry["sync_locked"] = True
            return ACK_ACCEPTED

        if action == 3:
            if self.profile.key == "generator":
                self._begin_generator_cooldown(now_ms)
            else:
                self._power_stage_enabled = False
                self._output_connected = False
                self.local_state = 2
                self._set_if_present("active_llc_phases", "Off")
                self._set_if_present("llc_operating_mode", "Standby/Off")
                self._set_if_present("switching_freq_khz", 0.0)
                self._set_if_present("phase1_tank_current_rms_a", 0.0)
                self._set_if_present("phase2_tank_current_rms_a", 0.0)
                self._set_if_present("main_contactor_closed", False)
                self._set_if_present("output_connected", False)
                self._set_if_present("ac_input_contactor_closed", False)
                self._set_if_present("output_contactor_closed", False)
                self._set_if_present("mppt_mode", "Off/Night")
                self._set_if_present("breaker_status", "Open")
                self._set_if_present("sync_locked", False)
            return ACK_ACCEPTED

        if action == 4 and self.profile.key == "inverter":
            self._power_stage_enabled = False
            self.telemetry["breaker_status"] = "Remote Tripped"
            self.local_state = 5
            self.fault_summary = 1
            return ACK_ACCEPTED

        if action == 5:
            self.clear_fault_locally()
            return ACK_ACCEPTED

        if action == 6 and self.profile.key == "generator":
            if self.telemetry["generator_mode"] not in ("Standby", "Cooldown"):
                return ACK_REJECTED_INVALID_STATE
            self.telemetry["generator_mode"] = "Cranking"
            self.telemetry["engine_rpm"] = 300
            self.telemetry["bus_current_a"] = -5.0
            self.local_state = 1
            self._generator_transition_deadline_ms = now_ms + 1000
            return ACK_ACCEPTED

        if action == 7 and self.profile.key == "generator":
            self._begin_generator_cooldown(now_ms)
            return ACK_ACCEPTED

        return ACK_REJECTED_INVALID_STATE

    def _begin_generator_cooldown(self, now_ms: int) -> None:
        if self.profile.key != "generator":
            return
        self.telemetry["generator_mode"] = "Cooldown"
        self.telemetry["engine_rpm"] = 1200
        self.telemetry["bus_current_a"] = 0.0
        self.telemetry["main_contactor_closed"] = False
        self._output_connected = False
        self._power_stage_enabled = False
        self._generator_transition_deadline_ms = now_ms + 1000

    def _update_generator(self, now_ms: int) -> None:
        deadline = self._generator_transition_deadline_ms
        if deadline is None or now_ms < deadline:
            return

        mode = self.telemetry["generator_mode"]
        if mode == "Cranking":
            self.telemetry["generator_mode"] = "Warmup"
            self.telemetry["engine_rpm"] = 1200
            self.telemetry["bus_current_a"] = 0.0
            self._generator_transition_deadline_ms = now_ms + 1000
        elif mode == "Warmup":
            self.telemetry["generator_mode"] = "Generating"
            self.telemetry["engine_rpm"] = 1800
            self.telemetry["bus_current_a"] = 10.0
            self.telemetry["main_contactor_closed"] = True
            self._output_connected = True
            self._power_stage_enabled = True
            self.local_state = 4 if self._coordinator_timeout_latched else 3
            self._generator_transition_deadline_ms = None
        elif mode == "Cooldown":
            self.telemetry["generator_mode"] = "Standby"
            self.telemetry["engine_rpm"] = 0
            self.telemetry["bus_current_a"] = 0.0
            self.telemetry["main_contactor_closed"] = False
            self.local_state = 4 if self._coordinator_timeout_latched else 2
            self._generator_transition_deadline_ms = None

    def _set_if_present(self, key: str, value: object) -> None:
        if key in self.telemetry:
            self.telemetry[key] = value

    def _identity_frame(self, _now_ms: int) -> CanFrame:
        data = self.serial_number.to_bytes(4, "little")
        data += bytes(
            (
                self.profile.node_type,
                self.profile.instance,
                self.power_stage_profile_id,
                self.capability_flags,
            )
        )
        return CanFrame(self.profile.identity_id, data)

    def _heartbeat_frame(self, _now_ms: int) -> CanFrame:
        data = bytes(
            (
                self.heartbeat_counter,
                self.profile.node_type,
                self.local_state,
                self.warning_flags,
            )
        )
        data += self.aux_voltage_mv.to_bytes(2, "little")
        data += self.firmware_build_id.to_bytes(2, "little")
        self.heartbeat_counter = (self.heartbeat_counter + 1) & 0xFF
        return CanFrame(self.profile.heartbeat_id, data)

    def _telemetry_frame(self, _now_ms: int) -> CanFrame:
        values = self.telemetry
        if self.profile.key == "boost":
            phase_mask = _choice_code(BOOST_PROFILE, "active_llc_phases", values["active_llc_phases"])
            llc_mode = _choice_code(BOOST_PROFILE, "llc_operating_mode", values["llc_operating_mode"])
            status = (phase_mask & 0x03) | ((llc_mode & 0x03) << 2)
            status |= int(bool(values["split_rail_balance_ok"])) << 4
            status |= int(bool(values["main_contactor_closed"])) << 5
            status |= int(bool(values["primary_role"])) << 6
            status |= int(bool(values["autonomous_droop_active"])) << 7
            data = _u16(float(values["lv_input_voltage_v"]) * 20)
            data += _u16(float(values["hv_output_voltage_v"]) * 10)
            data += _i16(float(values["total_input_current_a"]) * 10)
            data += _i8(int(values["max_stage_temperature_c"]))
            data += bytes((status,))
        elif self.profile.key == "solar":
            mode = _choice_code(SOLAR_PROFILE, "mppt_mode", values["mppt_mode"])
            status = mode
            status |= int(bool(values["output_connected"])) << 2
            status |= int(bool(values["curtailment_active"])) << 3
            data = _u16(float(values["pv_input_voltage_v"]) * 20)
            data += _u16(float(values["pv_input_current_a"]) * 20)
            data += _u16(float(values["bus_output_current_a"]) * 20)
            data += _i8(int(values["enclosure_temperature_c"]))
            data += bytes((status,))
        elif self.profile.key == "generator":
            mode = _choice_code(GENERATOR_PROFILE, "generator_mode", values["generator_mode"])
            status = mode
            status |= int(bool(values["main_contactor_closed"])) << 3
            status |= int(bool(values["low_oil_pressure"])) << 4
            status |= int(bool(values["low_fuel_warning"])) << 5
            status |= int(bool(values["crank_timeout"])) << 6
            data = _u16(float(values["bus_output_voltage_v"]) * 20)
            data += _i16(float(values["bus_current_a"]) * 10)
            data += _u16(int(values["engine_rpm"]))
            data += _i8(int(values["alternator_temperature_c"]))
            data += bytes((status,))
        elif self.profile.key == "grid_interface":
            status = 0
            status |= int(bool(values["grid_present"])) << 0
            status |= int(bool(values["pll_locked"])) << 1
            status |= int(bool(values["ac_input_contactor_closed"])) << 2
            status |= int(bool(values["output_contactor_closed"])) << 3
            status |= int(bool(values["anti_islanding_trip"])) << 4
            frequency = round((float(values["grid_frequency_hz"]) - 50.0) * 10)
            data = _u16(float(values["grid_ac_voltage_v"]) * 10)
            data += _i16(float(values["dc_bus_output_current_a"]) * 10)
            data += bytes((max(0, min(255, frequency)),))
            data += _i8(int(values["heatsink_temperature_c"]))
            data += bytes((status, int(values["power_factor_percent"])))
        elif self.profile.key == "inverter":
            breaker = _choice_code(INVERTER_PROFILE, "breaker_status", values["breaker_status"])
            status = breaker
            status |= int(bool(values["split_phase_240v"])) << 3
            status |= int(bool(values["sync_locked"])) << 4
            status |= int(bool(values["sync_time_master"])) << 5
            frequency = round((float(values["ac_frequency_hz"]) - 50.0) * 10)
            data = _u16(float(values["ac_rms_voltage_v"]) * 10)
            data += _u16(float(values["ac_rms_current_a"]) * 20)
            data += _i16(int(values["active_power_w"]))
            data += bytes((max(0, min(255, frequency)), status))
        else:
            raise RuntimeError(f"No telemetry encoder exists for {self.profile.key}.")
        return CanFrame(self.profile.telemetry_id or 0, data)

    def _slow_telemetry_frame(self, _now_ms: int) -> CanFrame:
        values = self.telemetry
        if self.profile.key == "boost":
            phase_mask = _choice_code(BOOST_PROFILE, "active_llc_phases", values["active_llc_phases"])
            active_count = bin(phase_mask & 0x03).count("1")
            flags = active_count & 0x03
            flags |= int(bool(values["psm_balance_trim_active"])) << 2
            flags |= int(bool(values["magnetics_snap_disc_alarm"])) << 3
            flags |= int(bool(values["thermal_derating_active"])) << 4
            flags |= int(bool(values["capacitive_mode_warning"])) << 5
            flags |= int(bool(values["phase_imbalance_warning"])) << 6
            data = _u16(float(values["switching_freq_khz"]) * 100)
            data += _i16(float(values["split_rail_imbalance_v"]) * 10)
            data += bytes((_u8(float(values["phase1_tank_current_rms_a"]) * 2),))
            data += bytes((_u8(float(values["phase2_tank_current_rms_a"]) * 2),))
            data += _i8(int(values["magnetics_temperature_c"]))
            data += bytes((flags,))
            return CanFrame(self.profile.slow_telemetry_id or 0, data)
        raise RuntimeError(f"No slow telemetry encoder exists for {self.profile.key}.")

    def _ack_frame(self, action: int, status: int, now_ms: int) -> CanFrame:
        uptime_ms = max(0, now_ms - self.started_at_ms)
        return encode_node_ack(
            self.profile.ack_id or 0,
            action,
            status,
            self.local_state,
            self.fault_summary,
            uptime_ms,
        )


def _choice_code(profile: ModuleProfile, key: str, value: object) -> int:
    field = next(item for item in profile.fields if item.key == key)
    for label, code in field.choices:
        if value == label or value == code:
            return code
    raise ValueError(f"Invalid {field.label}: {value!r}")


def _u8(value: float | int) -> int:
    return max(0, min(0xFF, round(value)))


def _u16(value: float | int) -> bytes:
    value = max(0, min(0xFFFF, round(value)))
    return int(value).to_bytes(2, "little")


def _i16(value: float | int) -> bytes:
    value = max(-0x8000, min(0x7FFF, round(value)))
    return int(value).to_bytes(2, "little", signed=True)


def _i8(value: int) -> bytes:
    value = max(-128, min(127, value))
    return value.to_bytes(1, "little", signed=True)
