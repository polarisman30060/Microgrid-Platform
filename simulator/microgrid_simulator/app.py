"""Tkinter interface for the PC-side Battery Node simulator."""

from __future__ import annotations

import queue
import time
import tkinter as tk
from tkinter import messagebox, ttk

from .battery_node import BatteryNode, NODE_STATE_NAMES
from .protocol import CanFrame, decode_coordinator_heartbeat
from .transport import SERIAL_BAUD_RATE, TransportError, WaveshareSerialTransport

POLL_INTERVAL_MS = 10
COORDINATOR_TIMEOUT_MS = 500
MAX_LOG_ROWS = 500


class SimulatorApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Microgrid Battery Node Simulator")
        self.root.geometry("1120x820")
        self.root.minsize(900, 680)

        self.node = BatteryNode()
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.transport = WaveshareSerialTransport(
            on_frame=lambda frame: self.events.put(("frame", frame)),
            on_error=lambda error: self.events.put(("error", error)),
        )
        self._coordinator_last_seen_ms: int | None = None
        self._coordinator_counter: int | None = None
        self._connected = False

        self._build_ui()
        self._refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self._close)
        self.root.after(POLL_INTERVAL_MS, self._poll)

    def _build_ui(self) -> None:
        container = ttk.Frame(self.root, padding=12)
        container.pack(fill="both", expand=True)
        container.columnconfigure(0, weight=1)
        container.rowconfigure(4, weight=1)

        ttk.Label(
            container,
            text="Microgrid Battery Node Simulator",
            font=("Segoe UI", 18, "bold"),
        ).grid(row=0, column=0, sticky="w")
        ttk.Label(
            container,
            text=(
                "Emulates Battery Node 0x10 using the draft CAN profile. "
                "It does not control physical power equipment."
            ),
        ).grid(row=1, column=0, sticky="w", pady=(2, 10))

        self._build_connection_panel(container)
        self._build_battery_panel(container)
        self._build_event_log(container)

    def _build_connection_panel(self, parent: ttk.Frame) -> None:
        panel = ttk.LabelFrame(parent, text="Waveshare USB-CAN-A connection", padding=10)
        panel.grid(row=2, column=0, sticky="ew", pady=(0, 10))
        panel.columnconfigure(5, weight=1)

        ttk.Label(panel, text="COM port").grid(row=0, column=0, sticky="w")
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(panel, textvariable=self.port_var, width=14)
        self.port_combo.grid(row=0, column=1, padx=(6, 8))
        ttk.Button(panel, text="Refresh", command=self._refresh_ports).grid(
            row=0, column=2, padx=(0, 12)
        )

        self.connect_button = ttk.Button(panel, text="Connect", command=self._connect)
        self.connect_button.grid(row=0, column=3, padx=(0, 8))
        self.connection_status = ttk.Label(panel, text="Disconnected")
        self.connection_status.grid(row=0, column=4, sticky="w")

        ttk.Label(
            panel,
            text=(
                f"Serial: {SERIAL_BAUD_RATE:,} baud, 8N1  |  CAN: 500 kbit/s, "
                "standard ID, normal mode  |  Waveshare variable-length framing"
            ),
        ).grid(row=1, column=0, columnspan=6, sticky="w", pady=(8, 2))
        ttk.Label(
            panel,
            text="Configure and verify the adapter using Waveshare's tool before connecting.",
        ).grid(row=2, column=0, columnspan=6, sticky="w")

        self.isolated_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            panel,
            text="I confirm this is an isolated CAN bench with no power hardware attached.",
            variable=self.isolated_var,
        ).grid(row=3, column=0, columnspan=6, sticky="w", pady=(8, 0))

    def _build_battery_panel(self, parent: ttk.Frame) -> None:
        panel = ttk.LabelFrame(parent, text="Battery Node 0 (GlobalNodeID 0x10)", padding=10)
        panel.grid(row=3, column=0, sticky="nsew", pady=(0, 10))
        panel.columnconfigure(1, weight=1)
        panel.columnconfigure(3, weight=1)

        self.node_button = ttk.Button(panel, text="Start Battery Node", command=self._toggle_node)
        self.node_button.grid(row=0, column=0, sticky="w")
        self.node_status = ttk.Label(panel, text="Stopped")
        self.node_status.grid(row=0, column=1, sticky="w", padx=8)
        self.loss_button = ttk.Button(
            panel,
            text="Simulate Node Loss",
            command=self._toggle_node_loss,
            state="disabled",
        )
        self.loss_button.grid(row=0, column=2, sticky="w")
        ttk.Button(panel, text="Inject Fault", command=self._inject_fault).grid(
            row=0, column=3, sticky="w", padx=(8, 0)
        )
        ttk.Button(panel, text="Clear Fault Locally", command=self._clear_fault).grid(
            row=0, column=4, sticky="w", padx=(8, 0)
        )

        self.coordinator_status = ttk.Label(panel, text="Coordinator: no heartbeat received")
        self.coordinator_status.grid(row=1, column=0, columnspan=5, sticky="w", pady=(8, 6))

        measurements = ttk.LabelFrame(panel, text="Simulated measurements", padding=8)
        measurements.grid(row=2, column=0, columnspan=5, sticky="ew")
        values = (
            ("State", "state"),
            ("SOC (%)", "soc"),
            ("96 V bus voltage (V)", "bus_voltage"),
            ("Battery pack voltage (V)", "pack_voltage"),
            ("Pack current (A)", "current"),
        )
        self.measurement_labels: dict[str, ttk.Label] = {}
        for index, (caption, key) in enumerate(values):
            column = index % 3
            row = index // 3
            ttk.Label(measurements, text=f"{caption}:").grid(
                row=row, column=column * 2, sticky="w", padx=(0, 4), pady=2
            )
            value_label = ttk.Label(measurements, text="")
            value_label.grid(row=row, column=column * 2 + 1, sticky="w", padx=(0, 20), pady=2)
            self.measurement_labels[key] = value_label

        controls = ttk.LabelFrame(panel, text="Set node state / measurements", padding=8)
        controls.grid(row=3, column=0, columnspan=5, sticky="ew", pady=(8, 0))
        controls.columnconfigure(5, weight=1)

        self.state_var = tk.StringVar(value="Ready")
        ttk.Label(controls, text="Local state").grid(row=0, column=0, sticky="w")
        self.state_combo = ttk.Combobox(
            controls,
            textvariable=self.state_var,
            values=("Ready", "Running", "Degraded Autonomous", "Fault", "Lockout"),
            state="readonly",
            width=22,
        )
        self.state_combo.grid(row=0, column=1, padx=(6, 12))
        ttk.Button(controls, text="Apply State", command=self._apply_state).grid(
            row=0, column=2, padx=(0, 14)
        )

        self.soc_var = tk.StringVar(value="80.0")
        self.bus_voltage_var = tk.StringVar(value="96.0")
        self.pack_voltage_var = tk.StringVar(value="96.0")
        self.current_var = tk.StringVar(value="0.0")
        for column, (caption, variable, width) in enumerate(
            (
                ("SOC", self.soc_var, 7),
                ("Bus V", self.bus_voltage_var, 8),
                ("Pack V", self.pack_voltage_var, 8),
                ("Current A", self.current_var, 8),
            ),
            start=3,
        ):
            ttk.Label(controls, text=caption).grid(row=0, column=column, sticky="w")
            ttk.Entry(controls, textvariable=variable, width=width).grid(
                row=1, column=column, sticky="w", padx=(0, 8)
            )
        ttk.Button(controls, text="Apply Values", command=self._apply_measurements).grid(
            row=1, column=7, sticky="w"
        )

    def _build_event_log(self, parent: ttk.Frame) -> None:
        panel = ttk.LabelFrame(parent, text="CAN frames and event log", padding=8)
        panel.grid(row=4, column=0, sticky="nsew")
        panel.columnconfigure(0, weight=1)
        panel.rowconfigure(0, weight=1)

        columns = ("time", "direction", "identifier", "data", "details")
        self.log = ttk.Treeview(panel, columns=columns, show="headings", height=12)
        for column, heading, width in (
            ("time", "Time", 95),
            ("direction", "Direction", 75),
            ("identifier", "CAN ID", 75),
            ("data", "Data", 260),
            ("details", "Details", 480),
        ):
            self.log.heading(column, text=heading)
            self.log.column(column, width=width, anchor="w")
        scrollbar = ttk.Scrollbar(panel, orient="vertical", command=self.log.yview)
        self.log.configure(yscrollcommand=scrollbar.set)
        self.log.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")

    def _refresh_ports(self) -> None:
        try:
            from serial.tools import list_ports
        except ImportError:
            self.port_combo["values"] = ()
            self.port_var.set("")
            self._log_event("SYSTEM", "pyserial is missing; use run_simulator.bat.")
            return

        ports = [port.device for port in list_ports.comports()]
        self.port_combo["values"] = ports
        if self.port_var.get() not in ports and ports:
            self.port_var.set(ports[0])

    def _connect(self) -> None:
        if self._connected:
            self._disconnect("Disconnected by user.")
            return
        if not self.isolated_var.get():
            messagebox.showwarning(
                "Isolated bench required",
                "Confirm the CAN bus is isolated and no power hardware is attached.",
                parent=self.root,
            )
            return
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("COM port required", "Select the adapter's COM port.", parent=self.root)
            return
        try:
            self.transport.connect(port)
        except TransportError as error:
            messagebox.showerror("USB-CAN connection failed", str(error), parent=self.root)
            self._log_event("SYSTEM", str(error))
            return

        self._connected = True
        self.connection_status.configure(text=f"Connected: {port}")
        self.connect_button.configure(text="Disconnect")
        self._log_event("SYSTEM", f"Connected to {port} at {SERIAL_BAUD_RATE:,} baud.")

    def _disconnect(self, reason: str) -> None:
        self.transport.close()
        self._connected = False
        self.connection_status.configure(text="Disconnected")
        self.connect_button.configure(text="Connect")
        if reason:
            self._log_event("SYSTEM", reason)

    def _toggle_node(self) -> None:
        if self.node.running:
            self.node.stop()
            self.node_button.configure(text="Start Battery Node")
            self.loss_button.configure(state="disabled", text="Simulate Node Loss")
            self.node_status.configure(text="Stopped")
            self._log_event("EVENT", "Battery Node stopped.")
        else:
            self.node.start(self._now_ms())
            self.node_button.configure(text="Stop Battery Node")
            self.loss_button.configure(state="normal", text="Simulate Node Loss")
            self.node_status.configure(text="Running")
            self._log_event("EVENT", "Battery Node started.")

    def _toggle_node_loss(self) -> None:
        lost = not self.node.communication_lost
        self.node.set_communication_lost(lost, self._now_ms())
        self.loss_button.configure(
            text="Restore Node Communications" if lost else "Simulate Node Loss"
        )
        self.node_status.configure(text="Communication lost" if lost else "Running")
        self._log_event(
            "EVENT",
            "Battery Node communications paused." if lost else "Battery Node communications restored.",
        )

    def _inject_fault(self) -> None:
        self.node.inject_fault()
        self.state_var.set("Fault")
        self._log_event("EVENT", "Injected a simulated local Battery fault.")
        self._update_measurements()

    def _clear_fault(self) -> None:
        previous_state = self.node.local_state
        self.node.clear_fault_locally()
        if self.node.local_state == previous_state:
            self._log_event("EVENT", "No local fault was active.")
        else:
            self.state_var.set("Ready")
            self._log_event("EVENT", "Simulated local fault cleared; node returned to Ready.")
        self._update_measurements()

    def _apply_state(self) -> None:
        state_by_name = {name: state for state, name in NODE_STATE_NAMES.items()}
        state = state_by_name.get(self.state_var.get())
        if state is None or state == 0:
            self._log_event("EVENT", "Selected Battery state is not supported.")
            return
        if state in (5, 6):
            self.node.inject_fault(lockout=state == 6)
        else:
            self.node.local_state = state
            if state == 2:
                self.node.operating_mode = 0
                self.node.main_contactor_closed = False
                self.node.precharge_closed = False
                self.node.current_a = 0.0
            elif state == 3:
                self.node.main_contactor_closed = True
            self.node.fault_summary = 0
        self._log_event("EVENT", f"Battery state set to {self.node.state_name}.")
        self._update_measurements()

    def _apply_measurements(self) -> None:
        try:
            soc = float(self.soc_var.get())
            bus_voltage = float(self.bus_voltage_var.get())
            pack_voltage = float(self.pack_voltage_var.get())
            current = float(self.current_var.get())
        except ValueError:
            messagebox.showerror(
                "Invalid measurements",
                "Enter numeric values for SOC, voltages, and current.",
                parent=self.root,
            )
            return
        if not 0.0 <= soc <= 100.0:
            messagebox.showerror("Invalid SOC", "SOC must be between 0 and 100%.", parent=self.root)
            return
        if not 0.0 <= bus_voltage <= 3276.75 or not 0.0 <= pack_voltage <= 3276.75:
            messagebox.showerror(
                "Invalid voltage",
                "Voltage must be between 0 and 3276.75 V for this wire encoding.",
                parent=self.root,
            )
            return
        if not -3276.8 <= current <= 3276.7:
            messagebox.showerror(
                "Invalid current",
                "Current must be between -3276.8 and 3276.7 A for this wire encoding.",
                parent=self.root,
            )
            return
        self.node.soc_percent = soc
        self.node.bus_voltage_v = bus_voltage
        self.node.pack_voltage_v = pack_voltage
        self.node.current_a = current
        self._log_event(
            "EVENT",
            f"Measurements applied: SOC {soc:.1f}%, bus {bus_voltage:.2f} V, "
            f"pack {pack_voltage:.2f} V, current {current:.1f} A.",
        )
        self._update_measurements()

    def _poll(self) -> None:
        while True:
            try:
                kind, payload = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == "frame":
                self._process_received_frame(payload)
            elif kind == "error":
                self._disconnect(str(payload))

        if self.node.running and self.transport.connected:
            for frame in self.node.frames_due(self._now_ms()):
                if not self._send_frame(frame):
                    break

        for event in self.node.update(self._now_ms()):
            self._log_event("EVENT", event.message)
        self._update_measurements()
        self._update_coordinator_status()
        self.root.after(POLL_INTERVAL_MS, self._poll)

    def _process_received_frame(self, payload: object) -> None:
        if not isinstance(payload, CanFrame):
            return
        frame = payload
        details = self._frame_description(frame)
        self._log_frame("RX", frame, details)

        if frame.identifier == 0x100 and not frame.extended and not frame.remote:
            try:
                heartbeat = decode_coordinator_heartbeat(frame)
            except ValueError as error:
                self._log_event("ERROR", str(error))
                return
            self._coordinator_last_seen_ms = self._now_ms()
            self._coordinator_counter = heartbeat.counter

        if self.node.running and not self.node.communication_lost:
            responses, events = self.node.handle_frame(frame, self._now_ms())
            for event in events:
                self._log_event("EVENT", event.message)
            for response in responses:
                self._send_frame(response)

    def _send_frame(self, frame: CanFrame) -> bool:
        try:
            self.transport.send(frame)
        except TransportError as error:
            self._disconnect(str(error))
            return False
        self._log_frame("TX", frame, self._frame_description(frame))
        return True

    def _frame_description(self, frame: CanFrame) -> str:
        if frame.identifier == 0x100 and not frame.extended and len(frame.data) == 8:
            try:
                heartbeat = decode_coordinator_heartbeat(frame)
            except ValueError:
                return "Malformed Coordinator heartbeat"
            return (
                f"Coordinator heartbeat #{heartbeat.counter}; state {heartbeat.state}; "
                f"active nodes {heartbeat.active_node_count}"
            )
        if frame.identifier == 0x130:
            return "Battery control command"
        if frame.identifier == 0x200:
            return "Battery fast telemetry"
        if frame.identifier == 0x210:
            return "Battery slow telemetry"
        if frame.identifier == 0x610:
            return "Battery heartbeat"
        if frame.identifier == 0x710:
            return "Battery identity"
        if frame.identifier == 0x750:
            return "Battery command ACK"
        return "Unknown CAN frame"

    def _update_measurements(self) -> None:
        self.measurement_labels["state"].configure(text=self.node.state_name)
        self.measurement_labels["soc"].configure(text=f"{self.node.soc_percent:.1f}")
        self.measurement_labels["bus_voltage"].configure(text=f"{self.node.bus_voltage_v:.2f}")
        self.measurement_labels["pack_voltage"].configure(text=f"{self.node.pack_voltage_v:.2f}")
        self.measurement_labels["current"].configure(text=f"{self.node.current_a:.1f}")
        if not self.node.running:
            self.node_status.configure(text="Stopped")
        elif self.node.communication_lost:
            self.node_status.configure(text="Communication lost")
        else:
            self.node_status.configure(text="Running")

    def _update_coordinator_status(self) -> None:
        if self._coordinator_last_seen_ms is None:
            self.coordinator_status.configure(text="Coordinator: no heartbeat received")
            return
        age_ms = self._now_ms() - self._coordinator_last_seen_ms
        if age_ms > COORDINATOR_TIMEOUT_MS:
            text = f"Coordinator: heartbeat stale ({age_ms} ms since last frame)"
        else:
            text = (
                f"Coordinator: online, heartbeat #{self._coordinator_counter}, "
                f"last frame {age_ms} ms ago"
            )
        if self.node.coordinator_heartbeat is None:
            text += " | Battery has not received one"
        elif self.node.communication_lost:
            text += " | Battery communications paused"
        self.coordinator_status.configure(text=text)

    def _log_frame(self, direction: str, frame: CanFrame, details: str) -> None:
        frame_type = "EXT " if frame.extended else ""
        if frame.remote:
            frame_type += "RTR "
        data = " ".join(f"{value:02X}" for value in frame.data)
        self._insert_log(direction, f"{frame_type}0x{frame.identifier:03X}", data, details)

    def _log_event(self, category: str, message: str) -> None:
        self._insert_log(category, "", "", message)

    def _insert_log(self, first: str, identifier: str, data: str, details: str) -> None:
        timestamp = time.strftime("%H:%M:%S")
        self.log.insert("", "end", values=(timestamp, first, identifier, data, details))
        children = self.log.get_children()
        if len(children) > MAX_LOG_ROWS:
            self.log.delete(children[0])
        self.log.yview_moveto(1.0)

    @staticmethod
    def _now_ms() -> int:
        return time.monotonic_ns() // 1_000_000

    def _close(self) -> None:
        self.transport.close()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    SimulatorApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
