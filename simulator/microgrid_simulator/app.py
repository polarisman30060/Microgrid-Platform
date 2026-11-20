"""Tkinter interface for simulating all supported microgrid module nodes."""

from __future__ import annotations

import queue
import time
import tkinter as tk
from tkinter import messagebox, ttk

from .battery_node import BatteryNode, NodeEvent
from .module_node import MODULE_PROFILES, NODE_STATE_NAMES, ModuleNode
from .module_panel import ModulePanel
from .registry import (
    MAIN_BUS,
    MAX_INSTANCES,
    NODE_TYPE_SOLAR_GATEWAY,
    solar_gateway_can_ids,
)
from .protocol import (
    COORDINATOR_HEARTBEAT_ID,
    CanFrame,
    decode_coordinator_heartbeat,
)
from .transport import SERIAL_BAUD_RATE, TransportError, WaveshareSerialTransport

POLL_INTERVAL_MS = 10
COORDINATOR_TIMEOUT_MS = 500
MAX_LOG_ROWS = 500
MODULE_ORDER = (
    "battery",
    "boost",
    "solar",
    "solar_gateway",
    "generator",
    "grid_interface",
    "inverter",
    "display",
)


class SimulatorApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Microgrid Module Simulator")
        self.root.geometry("1280x940")
        self.root.minsize(1000, 760)

        self.profiles = {key: MODULE_PROFILES[key] for key in MODULE_ORDER}
        self.nodes: dict[str, BatteryNode | ModuleNode] = {
            key: BatteryNode() if key == "battery" else ModuleNode(self.profiles[key])
            for key in MODULE_ORDER
        }
        self.node = self.nodes["battery"]
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.transport = WaveshareSerialTransport(
            on_frame=lambda frame: self.events.put(("frame", frame)),
            on_error=lambda error: self.events.put(("error", error)),
        )
        self._coordinator_last_seen_ms: int | None = None
        self._coordinator_counter: int | None = None
        self._connected = False
        self._reported_collisions: set[tuple[str, int]] = set()
        self._reported_non_main_bus_frames: set[tuple[str, int]] = set()
        self.module_panels: dict[str, ModulePanel] = {}

        self._build_ui()
        self._refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self._close)
        self.root.after(POLL_INTERVAL_MS, self._poll)

    def _build_ui(self) -> None:
        container = ttk.Frame(self.root, padding=12)
        container.pack(fill="both", expand=True)
        container.columnconfigure(0, weight=1)
        container.rowconfigure(5, weight=3)
        container.rowconfigure(6, weight=1)

        ttk.Label(
            container,
            text="Microgrid Module Simulator",
            font=("Segoe UI", 18, "bold"),
        ).grid(row=0, column=0, sticky="w")
        ttk.Label(
            container,
            text=(
                "Simulates Battery, Boost, Solar, Solar Gateway, Generator, "
                "Grid Interface, Inverter, and Display nodes. No power hardware is controlled."
            ),
        ).grid(row=1, column=0, sticky="w", pady=(2, 6))

        toolbar = ttk.Frame(container)
        toolbar.grid(row=2, column=0, sticky="ew", pady=(0, 6))
        ttk.Button(toolbar, text="Start All Modules", command=self._start_all).pack(
            side="left"
        )
        ttk.Button(toolbar, text="Stop All Modules", command=self._stop_all).pack(
            side="left", padx=(6, 12)
        )
        ttk.Label(
            toolbar,
            text="Each module runs locally; physical CAN transmission requires an isolated bench.",
        ).pack(side="left")

        self.collision_warning = ttk.Label(
            container,
            text="",
            foreground="#9b1c1c",
            wraplength=1180,
        )
        self.collision_warning.grid(row=3, column=0, sticky="ew", pady=(0, 8))
        self._update_collision_warning()

        self._build_connection_panel(container)
        self._build_module_tabs(container)
        self._build_event_log(container)

    def _build_connection_panel(self, parent: ttk.Frame) -> None:
        panel = ttk.LabelFrame(parent, text="Waveshare USB-CAN-A connection", padding=8)
        panel.grid(row=4, column=0, sticky="ew", pady=(0, 8))
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
        ).grid(row=1, column=0, columnspan=6, sticky="w", pady=(6, 2))
        ttk.Label(
            panel,
            text="Configure and verify the adapter using Waveshare's tool before connecting.",
        ).grid(row=2, column=0, columnspan=6, sticky="w")

        self.isolated_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            panel,
            text="I confirm this is an isolated CAN bench with no power hardware attached.",
            variable=self.isolated_var,
        ).grid(row=3, column=0, columnspan=6, sticky="w", pady=(6, 0))

    def _build_module_tabs(self, parent: ttk.Frame) -> None:
        self.module_tabs = ttk.Notebook(parent)
        self.module_tabs.grid(row=5, column=0, sticky="nsew", pady=(0, 8))
        for key in MODULE_ORDER:
            profile = self.profiles[key]
            tab = ttk.Frame(self.module_tabs)
            self.module_tabs.add(tab, text=profile.label)
            self.module_panels[key] = ModulePanel(
                self, tab, key, self.nodes[key], profile
            )

    def _build_event_log(self, parent: ttk.Frame) -> None:
        panel = ttk.LabelFrame(parent, text="CAN frames and event log", padding=6)
        panel.grid(row=6, column=0, sticky="nsew")
        panel.columnconfigure(0, weight=1)
        panel.rowconfigure(0, weight=1)

        columns = ("time", "direction", "identifier", "data", "details")
        self.log = ttk.Treeview(panel, columns=columns, show="headings", height=7)
        for column, heading, width in (
            ("time", "Time", 80),
            ("direction", "Direction", 75),
            ("identifier", "CAN ID", 75),
            ("data", "Data", 230),
            ("details", "Details", 700),
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
            messagebox.showwarning(
                "COM port required", "Select the adapter's COM port.", parent=self.root
            )
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

    def _start_all(self) -> None:
        now_ms = self._now_ms()
        for key, node in self.nodes.items():
            if not node.running:
                node.start(now_ms)
                self._log_event("EVENT", f"{self.profiles[key].label} started.")
        self._reported_collisions.clear()
        self._update_panels()

    def _stop_all(self) -> None:
        for key, node in self.nodes.items():
            if node.running:
                node.stop()
                self._log_event("EVENT", f"{self.profiles[key].label} stopped.")
        self._reported_collisions.clear()
        self._update_panels()

    def _toggle_node(self, key: str) -> None:
        node = self.nodes[key]
        profile = self.profiles[key]
        if node.running:
            node.stop()
            self._log_event("EVENT", f"{profile.label} stopped.")
        else:
            node.start(self._now_ms())
            self._log_event("EVENT", f"{profile.label} started.")
        self._reported_collisions.clear()
        self._update_panels()

    def _toggle_node_loss(self, key: str) -> None:
        node = self.nodes[key]
        lost = not node.communication_lost
        node.set_communication_lost(lost, self._now_ms())
        profile = self.profiles[key]
        message = "communications paused" if lost else "communications restored"
        self._log_event("EVENT", f"{profile.label} {message}.")
        self._reported_collisions.clear()
        self._update_panels()

    def _inject_fault(self, key: str) -> None:
        self.nodes[key].inject_fault()
        self._log_event("EVENT", f"Injected a simulated local {self.profiles[key].label} fault.")
        self._update_panels()

    def _clear_fault(self, key: str) -> None:
        node = self.nodes[key]
        previous_state = node.local_state
        node.clear_fault_locally()
        if node.local_state == previous_state:
            self._log_event("EVENT", f"No local fault was active on {self.profiles[key].label}.")
        else:
            self._log_event(
                "EVENT", f"Cleared the local {self.profiles[key].label} fault."
            )
        self._update_panels()

    def _apply_node_state(self, key: str, state_name: str) -> None:
        state_by_name = {name: state for state, name in NODE_STATE_NAMES.items()}
        state = state_by_name.get(state_name)
        if state is None:
            self._log_event("ERROR", f"Unknown local state: {state_name}.")
            return
        node = self.nodes[key]
        if state in (5, 6):
            node.inject_fault(lockout=state == 6)
        else:
            node.local_state = state
            node.fault_summary = 0
            if isinstance(node, BatteryNode):
                if state in (0, 1, 2):
                    node.operating_mode = 0
                    node.main_contactor_closed = False
                    node.precharge_closed = False
                    node.current_a = 0.0
                elif state == 3:
                    node.main_contactor_closed = True
            else:
                node._power_stage_enabled = state in (3, 4)
                node._output_connected = state in (3, 4)
                if key == "generator" and state in (0, 1, 2):
                    node.telemetry["generator_mode"] = "Standby"
                    node.telemetry["engine_rpm"] = 0
                    node.telemetry["main_contactor_closed"] = False
                if state in (0, 1, 2):
                    for field_name in (
                        "main_contactor_closed",
                        "output_connected",
                        "ac_input_contactor_closed",
                        "output_contactor_closed",
                        "sync_locked",
                    ):
                        node._set_if_present(field_name, False)
                    node._set_if_present("active_phases", 0)
                    node._set_if_present("mppt_mode", "Off/Night")
                    node._set_if_present("breaker_status", "Open")
        self._log_event(
            "EVENT", f"{self.profiles[key].label} local state set to {node.state_name}."
        )
        self._update_panels()

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

        now_ms = self._now_ms()
        for key, node in self.nodes.items():
            for event in node.update(now_ms):
                message = event.message if isinstance(event, NodeEvent) else str(event)
                self._log_event("EVENT", message)
            if self._connected and node.running:
                for frame in node.frames_due(now_ms):
                    if not self._send_module_frame(key, frame):
                        break

        self._update_panels()
        self.root.after(POLL_INTERVAL_MS, self._poll)

    def _process_received_frame(self, payload: object) -> None:
        if not isinstance(payload, CanFrame):
            return
        frame = payload
        self._log_frame("RX", frame, self._frame_description(frame))

        if frame.identifier == COORDINATOR_HEARTBEAT_ID and not frame.extended and not frame.remote:
            try:
                heartbeat = decode_coordinator_heartbeat(frame)
            except ValueError as error:
                self._log_event("ERROR", str(error))
            else:
                self._coordinator_last_seen_ms = self._now_ms()
                self._coordinator_counter = heartbeat.counter

        now_ms = self._now_ms()
        for key, node in self.nodes.items():
            profile = self.profiles[key]
            if (
                profile.network != MAIN_BUS
                and frame.identifier != COORDINATOR_HEARTBEAT_ID
            ):
                continue
            if not node.running or node.communication_lost:
                continue
            responses, events = node.handle_frame(frame, now_ms)
            for event in events:
                message = event.message if isinstance(event, NodeEvent) else str(event)
                self._log_event("EVENT", message)
            for response in responses:
                self._send_module_frame(key, response)

    def _send_module_frame(self, key: str, frame: CanFrame) -> bool:
        profile = self.profiles[key]
        if profile.network != MAIN_BUS:
            marker = (key, frame.identifier)
            if marker not in self._reported_non_main_bus_frames:
                self._reported_non_main_bus_frames.add(marker)
                self._log_event(
                    "LOCAL BUS",
                    f"Did not send {profile.label} frame 0x{frame.identifier:03X} "
                    "to the main-bus adapter; Solar MPPT nodes belong on a "
                    "gateway-local CAN segment.",
                )
            return True

        marker = (profile.network, frame.identifier)
        conflicts = self._all_tx_owners().get(marker, ())
        if len(conflicts) > 1:
            if marker not in self._reported_collisions:
                self._reported_collisions.add(marker)
                names = ", ".join(conflicts)
                self._log_event(
                    "ID CONFLICT",
                    f"Blocked physical TX 0x{frame.identifier:03X} from "
                    f"{profile.label}; conflicting registry users: {names}. "
                    "This ID remains blocked until the registry is corrected. "
                    "Local simulation continues.",
                )
            return True

        try:
            self.transport.send(frame)
        except TransportError as error:
            self._disconnect(str(error))
            return False
        self._log_frame("TX", frame, self._frame_description(frame))
        return True

    def _all_tx_owners(self) -> dict[tuple[str, int], tuple[str, ...]]:
        owners: dict[tuple[str, int], list[str]] = {}
        for key, profile in self.profiles.items():
            for identifier, name in profile.tx_identifiers:
                owners.setdefault((profile.network, identifier), []).append(
                    f"{profile.label} {name.lower()}"
                )
            if profile.control_id is not None:
                owners.setdefault((profile.network, profile.control_id), []).append(
                    f"Coordinator control for {profile.label}"
                )
        return {key: tuple(names) for key, names in owners.items()}

    def _frame_description(self, frame: CanFrame) -> str:
        if frame.identifier == COORDINATOR_HEARTBEAT_ID:
            if frame.extended or frame.remote or len(frame.data) != 8:
                return "Malformed Coordinator heartbeat"
            try:
                heartbeat = decode_coordinator_heartbeat(frame)
            except ValueError:
                return "Malformed Coordinator heartbeat"
            return (
                f"Coordinator heartbeat #{heartbeat.counter}; state {heartbeat.state}; "
                f"active nodes {heartbeat.active_node_count}"
            )

        for instance in range(MAX_INSTANCES[NODE_TYPE_SOLAR_GATEWAY]):
            gateway_ids = solar_gateway_can_ids(instance)
            reserved_ids = {
                gateway_ids.summary_primary: "primary summary",
                gateway_ids.summary_secondary: "secondary summary",
                gateway_ids.child_detail: "child detail",
                gateway_ids.child_command: "routed child command",
                gateway_ids.child_acknowledgement: "routed child reply",
            }
            if frame.identifier in reserved_ids:
                return (
                    f"Solar gateway {instance} {reserved_ids[frame.identifier]} "
                    "(payload/behavior TBD)"
                )

        matches: list[str] = []
        for key, profile in self.profiles.items():
            if profile.network != MAIN_BUS:
                continue
            if profile.control_id == frame.identifier:
                matches.append(f"{profile.label} control command")
            matches.extend(
                f"{profile.label} {name.lower()}"
                for identifier, name in profile.tx_identifiers
                if identifier == frame.identifier
            )
        if not matches:
            return "Unknown CAN frame"
        if len(matches) > 1:
            return "Draft ID collision: " + " / ".join(matches)
        return matches[0]

    def coordinator_status_text(self) -> str:
        if self._coordinator_last_seen_ms is None:
            return "Coordinator: no heartbeat received"
        age_ms = self._now_ms() - self._coordinator_last_seen_ms
        if age_ms > COORDINATOR_TIMEOUT_MS:
            return f"Coordinator: heartbeat stale ({age_ms} ms since last frame)"
        return (
            f"Coordinator: online, heartbeat #{self._coordinator_counter}, "
            f"last frame {age_ms} ms ago"
        )

    def _update_collision_warning(self) -> None:
        collisions = self._all_tx_owners()
        descriptions = []
        for (network, identifier), names in collisions.items():
            if len(names) > 1:
                descriptions.append(f"{network} 0x{identifier:03X}: {' / '.join(names)}")
        if descriptions:
            self.collision_warning.configure(
                text=(
                    "CAN ID CONFLICTS: "
                    + "; ".join(descriptions)
                    + ". Physical TX on a colliding ID is blocked."
                )
            )
        else:
            self.collision_warning.configure(
                text=(
                    "No duplicate IDs in the configured main-bus profiles. "
                    "Solar MPPT traffic is gateway-local and is not sent by the main-bus adapter."
                )
            )

    def _update_panels(self) -> None:
        for panel in self.module_panels.values():
            panel.refresh()

    def _log_frame(self, direction: str, frame: CanFrame, details: str) -> None:
        frame_type = "EXT " if frame.extended else ""
        if frame.remote:
            frame_type += "RTR "
        data = " ".join(f"{value:02X}" for value in frame.data)
        self._insert_log(
            direction, f"{frame_type}0x{frame.identifier:03X}", data, details
        )

    def _log_event(self, category: str, message: str) -> None:
        self._insert_log(category, "", "", message)

    def _insert_log(self, direction: str, identifier: str, data: str, details: str) -> None:
        timestamp = time.strftime("%H:%M:%S")
        self.log.insert(
            "", "end", values=(timestamp, direction, identifier, data, details)
        )
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
