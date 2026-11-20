"""Deterministic single-Battery-Node model and its draft CAN profile."""

from __future__ import annotations

from dataclasses import dataclass

from .protocol import (
    BATTERY_ACK_ID,
    BATTERY_COMMAND_ID,
    BATTERY_FAST_TELEMETRY_ID,
    BATTERY_HEARTBEAT_ID,
    BATTERY_IDENTITY_ID,
    BATTERY_INSTANCE,
    BATTERY_SLOW_TELEMETRY_ID,
    CAN_MAX_DATA_LENGTH,
    NODE_TYPE_BATTERY,
    CanFrame,
    crc8_sae_j1850,
    encode_battery_ack,
)

from .module_node import NODE_STATE_NAMES

ACK_ACCEPTED = 0x00
ACK_REJECTED_INVALID_STATE = 0x01
ACK_REJECTED_LOCAL_FAULT = 0x02
ACK_REJECTED_PRECHARGE = 0x03

HEARTBEAT_PERIOD_MS = 250
FAST_TELEMETRY_PERIOD_MS = 100
SLOW_TELEMETRY_PERIOD_MS = 1000
COORDINATOR_TIMEOUT_MS = 500
COORDINATOR_TIMEOUT_WARNING = 1 << 3


@dataclass(frozen=True)
class NodeEvent:
    message: str


class BatteryNode:
    """One configured Battery Node, instance 0 (GlobalNodeID 0x01)."""

    def __init__(self) -> None:
        self.serial_number = 0x00001000
        self.firmware_build_id = 1
        self.power_stage_profile_id = 0
        self.capability_flags = 0
        self.local_state = 2
        self.warning_flags = 0
        self.aux_voltage_mv = 30000
        self.soc_percent = 80.0
        self.bus_voltage_v = 96.0
        self.pack_voltage_v = 96.0
        self.current_a = 0.0
        self.operating_mode = 0
        self.main_contactor_closed = False
        self.precharge_closed = False
        self.cell_balancing = False
        self.charge_allowed = True
        self.discharge_allowed = True
        self.aux_lifeline_healthy = True
        self.min_cell_voltage_mv = 3300
        self.max_cell_voltage_mv = 3350
        self.max_pack_temperature_c = 25
        self.converter_temperature_c = 25
        self.available_capacity_ah = 100.0
        self.fault_summary = 0
        self.running = False
        self.communication_lost = False
        self.started_at_ms = 0
        self.heartbeat_counter = 0
        self.coordinator_heartbeat: tuple[int, int, int] | None = None
        self.last_command: tuple[int, int] | None = None
        self.last_control: tuple[int, int, int, int, int] | None = None
        self.target_voltage_setpoint = 0
        self.charge_current_limit_deciamps = 0
        self.discharge_current_limit_deciamps = 0
        self._next_frame_ms: dict[str, int] = {}
        self._coordinator_timeout_latched = False

    @property
    def state_name(self) -> str:
        return NODE_STATE_NAMES.get(self.local_state, "Unknown")

    def start(self, now_ms: int) -> None:
        self.running = True
        self.communication_lost = False
        self.started_at_ms = now_ms
        self.heartbeat_counter = 0
        self.coordinator_heartbeat = None
        self._coordinator_timeout_latched = False
        self.warning_flags &= ~COORDINATOR_TIMEOUT_WARNING
        self._next_frame_ms = {
            "identity": now_ms,
            "heartbeat": now_ms,
            "fast": now_ms,
            "slow": now_ms,
        }

    def stop(self) -> None:
        self.running = False
        self._next_frame_ms.clear()

    def set_communication_lost(self, lost: bool, now_ms: int) -> None:
        self.communication_lost = lost
        if not lost and self.running:
            self._next_frame_ms = {
                "identity": now_ms,
                "heartbeat": now_ms,
                "fast": now_ms,
                "slow": now_ms,
            }

    def inject_fault(self, lockout: bool = False) -> None:
        self.local_state = 6 if lockout else 5
        self.operating_mode = 3
        self.main_contactor_closed = False
        self.precharge_closed = False
        self.current_a = 0.0
        self.fault_summary = 1

    def clear_fault_locally(self) -> None:
        if self.local_state in (5, 6):
            self.local_state = 2
            self.operating_mode = 0
            self.fault_summary = 0
            self.main_contactor_closed = False
            self.precharge_closed = False
            self.current_a = 0.0

    def update(self, now_ms: int) -> list[NodeEvent]:
        if not self.running or self.communication_lost:
            return []

        last_heartbeat_ms = (
            self.coordinator_heartbeat[2]
            if self.coordinator_heartbeat is not None
            else self.started_at_ms
        )
        if now_ms - last_heartbeat_ms > COORDINATOR_TIMEOUT_MS:
            if not self._coordinator_timeout_latched:
                self._coordinator_timeout_latched = True
                self.warning_flags |= COORDINATOR_TIMEOUT_WARNING
                if self.local_state in (2, 3):
                    self.local_state = 4
                return [NodeEvent("Coordinator heartbeat timed out; Battery entered degraded mode.")]
        elif self._coordinator_timeout_latched:
            self._coordinator_timeout_latched = False
            self.warning_flags &= ~COORDINATOR_TIMEOUT_WARNING
            return [
                NodeEvent(
                    "Coordinator heartbeat restored; Battery remains in its current local state."
                )
            ]
        return []

    def frames_due(self, now_ms: int) -> list[CanFrame]:
        if not self.running or self.communication_lost:
            return []

        frames: list[CanFrame] = []
        if (
            "identity" in self._next_frame_ms
            and now_ms >= self._next_frame_ms["identity"]
        ):
            frames.append(self._identity_frame(now_ms))
            self._next_frame_ms.pop("identity", None)

        periods = (
            ("heartbeat", HEARTBEAT_PERIOD_MS, self._heartbeat_frame),
            ("fast", FAST_TELEMETRY_PERIOD_MS, self._fast_telemetry_frame),
            ("slow", SLOW_TELEMETRY_PERIOD_MS, self._slow_telemetry_frame),
        )
        for name, period, builder in periods:
            if now_ms >= self._next_frame_ms.get(name, now_ms):
                frames.append(builder(now_ms))
                self._next_frame_ms[name] = now_ms + period
        return frames

    def handle_frame(self, frame: CanFrame, now_ms: int) -> tuple[list[CanFrame], list[NodeEvent]]:
        responses: list[CanFrame] = []
        events: list[NodeEvent] = []
        if not self.running or self.communication_lost:
            return responses, events
        if frame.identifier == 0x100 and not frame.extended and not frame.remote:
            if len(frame.data) == CAN_MAX_DATA_LENGTH:
                self.coordinator_heartbeat = (
                    frame.data[0],
                    frame.data[1],
                    now_ms,
                )
            else:
                events.append(NodeEvent("Ignored Coordinator heartbeat with invalid DLC."))
            return responses, events

        if frame.identifier != BATTERY_COMMAND_ID:
            return responses, events
        if frame.extended or frame.remote or len(frame.data) != CAN_MAX_DATA_LENGTH:
            events.append(NodeEvent("Ignored malformed Battery command frame."))
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
            events.append(NodeEvent("Dropped Battery command with invalid CRC; no ACK sent."))
            return responses, events

        self.target_voltage_setpoint = self.last_control[1]
        self.charge_current_limit_deciamps = self.last_control[2]
        self.discharge_current_limit_deciamps = self.last_control[3]
        if action == 0x00:
            self.last_command = (action, ACK_ACCEPTED)
            events.append(NodeEvent("Battery accepted no-op setpoint update; no ACK is sent."))
            return responses, events

        ack_status = self._apply_command(action)
        self.last_command = (action, ack_status)
        uptime_ms = max(0, now_ms - self.started_at_ms) if self.running else 0
        responses.append(
            encode_battery_ack(
                action,
                ack_status,
                self.local_state,
                self.fault_summary,
                uptime_ms,
            )
        )
        result = "accepted" if ack_status == ACK_ACCEPTED else f"rejected ({ack_status})"
        events.append(NodeEvent(f"Battery command 0x{action:02X} {result}."))
        return responses, events

    def _apply_command(self, action: int) -> int:
        if action == 0x00:
            return ACK_ACCEPTED
        if action == 0x01:
            if self.local_state in (5, 6):
                return ACK_REJECTED_LOCAL_FAULT
            if self.local_state != 2:
                return ACK_REJECTED_INVALID_STATE
            if abs(self.bus_voltage_v - self.pack_voltage_v) > 1.5:
                return ACK_REJECTED_PRECHARGE
            self.precharge_closed = True
            self.main_contactor_closed = True
            self.precharge_closed = False
            return ACK_ACCEPTED
        if action == 0x02:
            if self.local_state in (5, 6):
                return ACK_REJECTED_LOCAL_FAULT
            if self.local_state != 2 or not self.main_contactor_closed:
                return ACK_REJECTED_INVALID_STATE
            self.local_state = 3
            return ACK_ACCEPTED
        if action == 0x03:
            if self.local_state in (5, 6):
                return ACK_REJECTED_LOCAL_FAULT
            self.local_state = 2
            self.operating_mode = 0
            self.current_a = 0.0
            self.main_contactor_closed = False
            self.precharge_closed = False
            return ACK_ACCEPTED
        if action == 0x05:
            self.clear_fault_locally()
            return ACK_ACCEPTED
        return ACK_REJECTED_INVALID_STATE

    def _identity_frame(self, _now_ms: int) -> CanFrame:
        data = self.serial_number.to_bytes(4, byteorder="little")
        data += bytes(
            (
                NODE_TYPE_BATTERY,
                BATTERY_INSTANCE,
                self.power_stage_profile_id,
                self.capability_flags,
            )
        )
        return CanFrame(BATTERY_IDENTITY_ID, data)

    def _heartbeat_frame(self, _now_ms: int) -> CanFrame:
        data = bytes(
            (
                self.heartbeat_counter,
                NODE_TYPE_BATTERY,
                self.local_state,
                self.warning_flags,
            )
        )
        data += self.aux_voltage_mv.to_bytes(2, byteorder="little")
        data += self.firmware_build_id.to_bytes(2, byteorder="little")
        self.heartbeat_counter = (self.heartbeat_counter + 1) & 0xFF
        return CanFrame(BATTERY_HEARTBEAT_ID, data)

    def _fast_telemetry_frame(self, _now_ms: int) -> CanFrame:
        soc_half_percent = max(0, min(200, round(self.soc_percent * 2)))
        mode_and_flags = self.operating_mode & 0x03
        mode_and_flags |= int(self.main_contactor_closed) << 2
        mode_and_flags |= int(self.precharge_closed) << 3
        mode_and_flags |= int(self.cell_balancing) << 4
        mode_and_flags |= int(self.charge_allowed) << 5
        mode_and_flags |= int(self.discharge_allowed) << 6
        mode_and_flags |= int(self.aux_lifeline_healthy) << 7
        data = _u16(round(self.bus_voltage_v * 20))
        data += _u16(round(self.pack_voltage_v * 20))
        data += _i16(round(self.current_a * 10))
        data += bytes((soc_half_percent, mode_and_flags))
        return CanFrame(BATTERY_FAST_TELEMETRY_ID, data)

    def _slow_telemetry_frame(self, _now_ms: int) -> CanFrame:
        data = _u16(self.min_cell_voltage_mv)
        data += _u16(self.max_cell_voltage_mv)
        data += _i8(self.max_pack_temperature_c)
        data += _i8(self.converter_temperature_c)
        data += _u16(round(self.available_capacity_ah * 10))
        return CanFrame(BATTERY_SLOW_TELEMETRY_ID, data)


def _u16(value: int) -> bytes:
    return max(0, min(0xFFFF, value)).to_bytes(2, byteorder="little")


def _i16(value: int) -> bytes:
    value = max(-0x8000, min(0x7FFF, value))
    return value.to_bytes(2, byteorder="little", signed=True)


def _i8(value: int) -> bytes:
    value = max(-128, min(127, value))
    return value.to_bytes(1, byteorder="little", signed=True)
