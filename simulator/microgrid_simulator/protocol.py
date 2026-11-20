"""CAN application payloads and Waveshare variable-length serial framing."""

from __future__ import annotations

from dataclasses import dataclass

from .registry import main_bus_global_node_id

COORDINATOR_HEARTBEAT_ID = 0x100
BATTERY_GLOBAL_NODE_ID = main_bus_global_node_id(0x01, 0)
BATTERY_COMMAND_ID = 0x120 + BATTERY_GLOBAL_NODE_ID
BATTERY_FAST_TELEMETRY_ID = 0x200
BATTERY_SLOW_TELEMETRY_ID = 0x210
BATTERY_HEARTBEAT_ID = 0x600 + BATTERY_GLOBAL_NODE_ID
BATTERY_IDENTITY_ID = 0x700 + BATTERY_GLOBAL_NODE_ID
BATTERY_ACK_ID = 0x760 + BATTERY_GLOBAL_NODE_ID - 1

NODE_TYPE_BATTERY = 0x01
BATTERY_INSTANCE = 0x00
CAN_MAX_DATA_LENGTH = 8


@dataclass(frozen=True)
class CanFrame:
    identifier: int
    data: bytes
    extended: bool = False
    remote: bool = False


@dataclass(frozen=True)
class CoordinatorHeartbeat:
    counter: int
    state: int
    flags: int
    active_node_count: int
    uptime_seconds: int


class FrameFormatError(ValueError):
    """Raised when a CAN frame cannot be represented by the Waveshare format."""


def crc8_sae_j1850(data: bytes) -> int:
    crc = 0xFF
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1D) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc ^ 0xFF


def encode_waveshare_frame(frame: CanFrame) -> bytes:
    if frame.remote:
        raise FrameFormatError("Remote frames are not supported by this simulator.")
    if not 0 <= len(frame.data) <= CAN_MAX_DATA_LENGTH:
        raise FrameFormatError("Classical CAN payload must contain 0 to 8 bytes.")
    max_identifier = 0x1FFFFFFF if frame.extended else 0x7FF
    if not 0 <= frame.identifier <= max_identifier:
        raise FrameFormatError("CAN identifier is out of range for its frame type.")

    frame_type = 0xC0 | len(frame.data)
    identifier_length = 2
    if frame.extended:
        frame_type |= 0x20
        identifier_length = 4
    identifier = frame.identifier.to_bytes(identifier_length, byteorder="little")
    return b"\xAA" + bytes((frame_type,)) + identifier + frame.data + b"\x55"


class WaveshareFrameParser:
    """Incremental parser for Waveshare's USB-CAN-A variable-length framing."""

    def __init__(self) -> None:
        self._buffer = bytearray()
        self.malformed_packets = 0

    def feed(self, chunk: bytes) -> list[CanFrame]:
        self._buffer.extend(chunk)
        frames: list[CanFrame] = []

        while self._buffer:
            try:
                header_index = self._buffer.index(0xAA)
            except ValueError:
                self._buffer.clear()
                break
            if header_index:
                del self._buffer[:header_index]
            if len(self._buffer) < 2:
                break

            frame_type = self._buffer[1]
            if frame_type & 0xC0 != 0xC0 or frame_type & 0x0F > CAN_MAX_DATA_LENGTH:
                self.malformed_packets += 1
                del self._buffer[0]
                continue

            extended = bool(frame_type & 0x20)
            remote = bool(frame_type & 0x10)
            identifier_length = 4 if extended else 2
            data_length = frame_type & 0x0F
            packet_length = 2 + identifier_length + data_length + 1
            if len(self._buffer) < packet_length:
                break
            if self._buffer[packet_length - 1] != 0x55:
                self.malformed_packets += 1
                del self._buffer[0]
                continue

            identifier_start = 2
            data_start = identifier_start + identifier_length
            identifier = int.from_bytes(
                self._buffer[identifier_start:data_start], byteorder="little"
            )
            max_identifier = 0x1FFFFFFF if extended else 0x7FF
            if identifier > max_identifier:
                self.malformed_packets += 1
                del self._buffer[:packet_length]
                continue
            data = bytes(self._buffer[data_start : data_start + data_length])
            frames.append(CanFrame(identifier, data, extended, remote))
            del self._buffer[:packet_length]

        return frames


def decode_coordinator_heartbeat(frame: CanFrame) -> CoordinatorHeartbeat:
    if frame.identifier != COORDINATOR_HEARTBEAT_ID or frame.extended or frame.remote:
        raise FrameFormatError("Frame is not a standard Coordinator heartbeat.")
    if len(frame.data) != CAN_MAX_DATA_LENGTH:
        raise FrameFormatError("Coordinator heartbeat must contain 8 data bytes.")
    data = frame.data
    return CoordinatorHeartbeat(
        counter=data[0],
        state=data[1],
        flags=data[2],
        active_node_count=data[3],
        uptime_seconds=int.from_bytes(data[4:8], byteorder="little"),
    )


def encode_node_ack(
    identifier: int,
    action: int,
    status: int,
    local_state: int,
    fault_summary: int,
    uptime_ms: int,
) -> CanFrame:
    data = bytes((action, status, local_state, fault_summary))
    data += (uptime_ms & 0xFFFFFFFF).to_bytes(4, byteorder="little")
    return CanFrame(identifier, data)


def encode_battery_ack(
    action: int,
    status: int,
    local_state: int,
    fault_summary: int,
    uptime_ms: int,
) -> CanFrame:
    return encode_node_ack(
        BATTERY_ACK_ID, action, status, local_state, fault_summary, uptime_ms
    )
