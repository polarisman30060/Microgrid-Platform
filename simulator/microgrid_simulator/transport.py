"""Serial transport for the Waveshare USB-CAN-A variable-length protocol."""

from __future__ import annotations

import threading
from collections.abc import Callable

from .protocol import CanFrame, WaveshareFrameParser, encode_waveshare_frame

SERIAL_BAUD_RATE = 2_000_000


class TransportError(RuntimeError):
    """Raised when the USB-CAN serial connection cannot send or stay open."""


class WaveshareSerialTransport:
    def __init__(
        self,
        on_frame: Callable[[CanFrame], None],
        on_error: Callable[[str], None],
    ) -> None:
        self._on_frame = on_frame
        self._on_error = on_error
        self._serial = None
        self._reader: threading.Thread | None = None
        self._stop_event = threading.Event()
        self._write_lock = threading.Lock()
        self._parser = WaveshareFrameParser()

    @property
    def connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    def connect(self, port: str) -> None:
        if self.connected:
            raise TransportError("The serial adapter is already connected.")
        try:
            import serial
        except ImportError as error:
            raise TransportError(
                "pyserial is missing. Run simulator\\run_simulator.bat to install it."
            ) from error

        try:
            self._serial = serial.Serial(
                port=port,
                baudrate=SERIAL_BAUD_RATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.05,
                write_timeout=1.0,
            )
        except (serial.SerialException, OSError) as error:
            self._serial = None
            raise TransportError(f"Could not open {port}: {error}") from error

        self._stop_event.clear()
        self._reader = threading.Thread(target=self._read_loop, name="usb-can-reader", daemon=True)
        self._reader.start()

    def send(self, frame: CanFrame) -> None:
        if not self.connected:
            raise TransportError("USB-CAN adapter is not connected.")
        packet = encode_waveshare_frame(frame)
        try:
            with self._write_lock:
                written = self._serial.write(packet)
            if written != len(packet):
                raise OSError(f"serial write was partial ({written}/{len(packet)} bytes)")
        except OSError as error:
            raise TransportError(f"Could not write CAN frame: {error}") from error

    def close(self) -> None:
        self._stop_event.set()
        serial_port = self._serial
        self._serial = None
        if serial_port is not None:
            serial_port.close()
        if self._reader is not None:
            self._reader.join(timeout=1.0)
            self._reader = None

    def _read_loop(self) -> None:
        try:
            while not self._stop_event.is_set():
                serial_port = self._serial
                if serial_port is None:
                    return
                chunk = serial_port.read(256)
                for frame in self._parser.feed(chunk):
                    self._on_frame(frame)
        except OSError as error:
            if not self._stop_event.is_set():
                self._on_error(f"USB-CAN receive failed: {error}")
