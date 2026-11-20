"""Reusable Tkinter panel for one configured microgrid module node."""

from __future__ import annotations

import math
import tkinter as tk
from tkinter import messagebox, ttk
from typing import TYPE_CHECKING

from .battery_node import BatteryNode, NodeEvent
from .module_node import (
    COMMAND_ACTIONS,
    NODE_STATE_NAMES,
    ModuleNode,
    ModuleProfile,
    TelemetryField,
    validate_telemetry_value,
)
from .protocol import CanFrame, crc8_sae_j1850
from .registry import solar_gateway_can_ids

if TYPE_CHECKING:
    from .app import SimulatorApp


class ModulePanel:
    def __init__(
        self,
        app: SimulatorApp,
        parent: ttk.Frame,
        node_key: str,
        node: BatteryNode | ModuleNode,
        profile: ModuleProfile,
    ) -> None:
        self.app = app
        self.node_key = node_key
        self.node = node
        self.profile = profile
        self.field_vars: dict[str, tk.Variable] = {}
        self.current_labels: dict[str, ttk.Label] = {}

        container = ttk.Frame(parent, padding=8)
        container.pack(fill="both", expand=True)
        container.columnconfigure(0, weight=1)
        container.rowconfigure(2, weight=1)

        controls = ttk.LabelFrame(container, text="Node controls", padding=8)
        controls.grid(row=0, column=0, sticky="ew")
        controls.columnconfigure(7, weight=1)
        self.start_button = ttk.Button(
            controls, text="Start", command=lambda: self.app._toggle_node(self.node_key)
        )
        self.start_button.grid(row=0, column=0, sticky="w")
        self.status_label = ttk.Label(controls, text="Stopped")
        self.status_label.grid(row=0, column=1, padx=(8, 18), sticky="w")
        self.loss_button = ttk.Button(
            controls,
            text="Simulate Node Loss",
            command=lambda: self.app._toggle_node_loss(self.node_key),
            state="disabled",
        )
        self.loss_button.grid(row=0, column=2, padx=(0, 8))
        ttk.Button(
            controls,
            text="Inject Fault",
            command=lambda: self.app._inject_fault(self.node_key),
        ).grid(row=0, column=3, padx=4)
        ttk.Button(
            controls,
            text="Clear Fault Locally",
            command=lambda: self.app._clear_fault(self.node_key),
        ).grid(row=0, column=4, padx=4)

        ttk.Label(controls, text="Local state").grid(row=0, column=5, padx=(12, 4))
        self.state_var = tk.StringVar(value=self.node.state_name)
        self.state_combo = ttk.Combobox(
            controls,
            textvariable=self.state_var,
            values=tuple(NODE_STATE_NAMES.values()),
            state="readonly",
            width=21,
        )
        self.state_combo.grid(row=0, column=6, padx=(0, 6))
        ttk.Button(
            controls,
            text="Apply",
            command=lambda: self.app._apply_node_state(self.node_key, self.state_var.get()),
        ).grid(row=0, column=7, sticky="w")

        self.coordinator_label = ttk.Label(container, text="Coordinator: no heartbeat received")
        self.coordinator_label.grid(row=1, column=0, sticky="w", pady=(6, 8))

        details = ttk.Notebook(container)
        details.grid(row=2, column=0, sticky="nsew")
        telemetry_tab = ttk.Frame(details, padding=8)
        contract_tab = ttk.Frame(details, padding=8)
        details.add(telemetry_tab, text="Telemetry")
        details.add(contract_tab, text="Frames and commands")

        self._build_telemetry(telemetry_tab)
        self._build_contract(contract_tab)
        self.refresh()

    def _build_telemetry(self, parent: ttk.Frame) -> None:
        parent.columnconfigure(0, weight=1)
        profile_status = "Draft module profile"
        ttk.Label(
            parent,
            text=(
                f"{profile_status}. These controls edit simulated values only; "
                "they do not control power hardware."
            ),
            wraplength=950,
        ).grid(row=0, column=0, sticky="w", pady=(0, 8))

        if self.profile.global_node_id is None:
            address_info = (
                f"Gateway-local address {self.profile.local_address} "
                f"(Solar segment, instance {self.profile.instance})"
            )
        else:
            address_info = f"GlobalNodeID 0x{self.profile.global_node_id:02X}"
        serial_info = (
            f"Node type 0x{self.profile.node_type:02X}  |  Instance {self.profile.instance}  |  "
            f"{address_info}  |  "
            f"Serial 0x{self.node.serial_number:08X}  |  "
            f"Power-stage profile 0x{self.node.power_stage_profile_id:02X}  |  "
            f"Capabilities 0x{self.node.capability_flags:02X}"
        )
        ttk.Label(parent, text=serial_info).grid(row=1, column=0, sticky="w", pady=(0, 8))

        telemetry_box = ttk.LabelFrame(parent, text="Editable telemetry values", padding=8)
        telemetry_box.grid(row=2, column=0, sticky="nsew")
        telemetry_box.columnconfigure(1, weight=1)
        telemetry_box.columnconfigure(4, weight=1)

        if not self.profile.fields:
            empty_profile_text = (
                "No Display-node telemetry frame or module-specific fields are defined in the current CAN draft."
                if self.profile.key == "display"
                else "Gateway summary, child-detail, and routed-command payloads are reserved but not defined; this emulator does not emit or process them."
            )
            ttk.Label(
                telemetry_box,
                text=empty_profile_text,
                wraplength=900,
            ).grid(row=0, column=0, columnspan=5, sticky="w")
        else:
            for index, field in enumerate(self.profile.fields):
                row = index // 2
                block = (index % 2) * 3
                ttk.Label(
                    telemetry_box,
                    text=(
                        f"{field.label} ({field.unit})\n"
                        f"{field.wire_format}"
                    ),
                    wraplength=230,
                ).grid(row=row, column=block, sticky="w", padx=(0, 6), pady=4)
                current = ttk.Label(telemetry_box, text="")
                current.grid(row=row, column=block + 1, sticky="w", padx=(0, 8), pady=4)
                self.current_labels[field.key] = current
                variable = self._make_field_editor(telemetry_box, field, row, block + 2)
                self.field_vars[field.key] = variable

        if self.profile.fields:
            ttk.Button(
                parent,
                text="Apply telemetry values",
                command=self._apply_telemetry_values,
            ).grid(row=3, column=0, sticky="w", pady=(8, 0))

    def _make_field_editor(
        self,
        parent: ttk.LabelFrame,
        field: TelemetryField,
        row: int,
        column: int,
    ) -> tk.Variable:
        value = self._get_field_value(field.key)
        if field.kind == "bool":
            variable = tk.BooleanVar(value=bool(value))
            ttk.Checkbutton(parent, variable=variable).grid(
                row=row, column=column, sticky="w", pady=4
            )
        elif field.kind == "choice":
            variable = tk.StringVar(value=self._choice_label(field, value))
            combo = ttk.Combobox(
                parent,
                textvariable=variable,
                values=tuple(label for label, _ in field.choices),
                state="readonly",
                width=22,
            )
            combo.grid(row=row, column=column, sticky="ew", pady=4)
        else:
            variable = tk.StringVar(value=str(value))
            ttk.Entry(parent, textvariable=variable, width=12).grid(
                row=row, column=column, sticky="w", pady=4
            )
        return variable

    def _build_contract(self, parent: ttk.Frame) -> None:
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(2, weight=1)
        contract_box = ttk.LabelFrame(parent, text="Wire frames and fields", padding=8)
        contract_box.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        contract_box.columnconfigure(0, weight=1)
        self.contract_text = tk.Text(contract_box, height=8, wrap="word", state="disabled")
        self.contract_text.grid(row=0, column=0, sticky="ew")
        self._write_contract()

        command_box = ttk.LabelFrame(parent, text="CMD_NODE_CONTROL actions", padding=8)
        command_box.grid(row=1, column=0, sticky="ew", pady=(0, 8))
        command_box.columnconfigure(0, weight=1)
        columns = ("action", "meaning", "profile")
        self.command_table = ttk.Treeview(
            command_box, columns=columns, show="headings", height=8
        )
        for column, heading, width in (
            ("action", "Action", 90),
            ("meaning", "Command meaning", 340),
            ("profile", "This node profile", 180),
        ):
            self.command_table.heading(column, text=heading)
            self.command_table.column(column, width=width, anchor="w")
        self.command_table.grid(row=0, column=0, sticky="ew")
        for action, meaning in COMMAND_ACTIONS:
            if self.profile.control_id is None:
                status = "No control contract"
            elif action == 0:
                status = "Supported; no ACK"
            elif action in self.profile.supported_actions:
                status = "Supported"
            else:
                status = "Not applicable"
            self.command_table.insert(
                "", "end", values=(f"0x{action:02X}", meaning, status)
            )

        test_box = ttk.LabelFrame(
            parent, text="Local command test (not sent to the CAN adapter)", padding=8
        )
        test_box.grid(row=2, column=0, sticky="nsew")
        for column in range(8):
            test_box.columnconfigure(column, weight=1 if column in (1, 3, 5) else 0)
        ttk.Label(test_box, text="Action").grid(row=0, column=0, sticky="w")
        self.test_action_var = tk.StringVar()
        action_values = tuple(
            f"0x{action:02X} - {meaning}" for action, meaning in COMMAND_ACTIONS
        )
        self.test_action_combo = ttk.Combobox(
            test_box,
            textvariable=self.test_action_var,
            values=action_values,
            state="readonly",
            width=34,
        )
        self.test_action_combo.current(0)
        self.test_action_combo.grid(row=0, column=1, sticky="w", padx=(4, 12))

        self.target_voltage_var = tk.StringVar(value="96.0")
        self.current_limit_var = tk.StringVar(value="0.0")
        self.secondary_limit_var = tk.StringVar(value="0.0")
        for column, (label, variable) in enumerate(
            (
                ("Target V", self.target_voltage_var),
                ("Current limit A", self.current_limit_var),
                ("Secondary limit A", self.secondary_limit_var),
            )
        ):
            label_column = 2 + column * 2
            ttk.Label(test_box, text=label).grid(
                row=0, column=label_column, sticky="w", padx=(4, 2)
            )
            ttk.Entry(test_box, textvariable=variable, width=9).grid(
                row=0, column=label_column + 1, sticky="w"
            )

        self.test_command_button = ttk.Button(
            test_box, text="Apply local test command", command=self._apply_test_command
        )
        self.test_command_button.grid(row=1, column=0, sticky="w", pady=(8, 0))
        if self.profile.control_id is None:
            self.test_command_button.configure(state="disabled")
        self.last_command_label = ttk.Label(test_box, text="No Coordinator command received.")
        self.last_command_label.grid(row=1, column=1, columnspan=7, sticky="w", padx=8)
        if self.profile.control_id is not None:
            self.test_command_button.configure(state="disabled")

    def _write_contract(self) -> None:
        profile = self.profile
        coordinator_heartbeat = (
            "RX 0x100 - Coordinator heartbeat forwarded by the Solar gateway, every 100 ms."
            if profile.network == "solar_segment"
            else "RX 0x100 - Coordinator heartbeat, 8 bytes, every 100 ms."
        )
        lines = [
            f"Network: {profile.network}. {coordinator_heartbeat}",
        ]
        if profile.control_id is not None:
            command_timing = "on change or explicit request"
            if profile.key == "boost":
                command_timing = "20 ms only during active parallel balancing"
            elif profile.key == "inverter":
                command_timing = "on change; at most 250 ms when justified"
            lines.append(
                f"RX 0x{profile.control_id:03X} - CMD_NODE_CONTROL, 8 bytes, "
                f"{command_timing}: action, target voltage "
                "(0.05 V/bit), current limit/token (0.1 A/bit), secondary limit "
                "(0.1 A/bit), CRC-8/SAE-J1850."
            )
        lines.append(
            f"TX 0x{profile.identity_id:03X} - NODE_IDENTITY, 8 bytes, at startup/on request: "
            "serial number, node type, instance, power-stage profile, capabilities."
        )
        lines.append(
            f"TX 0x{profile.heartbeat_id:03X} - NODE_HEARTBEAT, 8 bytes, every "
            f"{profile.heartbeat_period_ms} ms: "
            "counter, type, local state, warnings, auxiliary voltage, firmware build."
        )
        if profile.emergency_id is not None:
            lines.append(
                f"Assigned emergency/fault ID: 0x{profile.emergency_id:03X}; "
                "fault-frame emission is not implemented in this emulator."
            )
        elif profile.network == "solar_segment":
            lines.append(
                "Solar local-segment emergency identifier is not assigned yet."
            )
        if profile.telemetry_id is not None and profile.telemetry_period_ms is not None:
            field_names = ", ".join(field.label for field in profile.fields)
            lines.append(
                f"TX 0x{profile.telemetry_id:03X} - module telemetry every "
                f"{profile.telemetry_period_ms} ms: {field_names}."
            )
        if profile.slow_telemetry_id is not None:
            slow_names = (
                "minimum/maximum cell voltage, pack/converter temperature, available capacity"
            )
            lines.append(
                f"TX 0x{profile.slow_telemetry_id:03X} - slow Battery telemetry every "
                f"{profile.slow_telemetry_period_ms} ms: {slow_names}."
            )
        if profile.ack_id is not None and profile.supported_actions:
            lines.append(
                f"TX 0x{profile.ack_id:03X} - CMD_ACK for state-changing commands: "
                "echoed action, status, local state, fault summary, uptime."
            )
        if profile.key == "solar_gateway":
            gateway_ids = solar_gateway_can_ids(profile.instance)
            lines.append(
                "Reserved, not emitted/handled: primary summary "
                f"0x{gateway_ids.summary_primary:03X}, secondary summary "
                f"0x{gateway_ids.summary_secondary:03X}, child detail "
                f"0x{gateway_ids.child_detail:03X}, routed control "
                f"0x{gateway_ids.child_command:03X}, routed reply "
                f"0x{gateway_ids.child_acknowledgement:03X}. Payloads are TBD."
            )
        lines.append(
            "Telemetry status bits, scaling, signedness, IDs, and periods are listed in the "
            "field editor and project CAN definition."
        )
        self.contract_text.configure(state="normal")
        self.contract_text.delete("1.0", "end")
        self.contract_text.insert("1.0", "\n".join(lines))
        self.contract_text.configure(state="disabled")

    def _get_field_value(self, key: str) -> object:
        if isinstance(self.node, BatteryNode):
            return getattr(self.node, key)
        return self.node.get_telemetry_value(key)

    @staticmethod
    def _choice_label(field: TelemetryField, value: object) -> str:
        for label, code in field.choices:
            if value == code or value == label:
                return label
        return str(value)

    def _apply_telemetry_values(self) -> None:
        try:
            values: list[tuple[TelemetryField, object]] = []
            for field in self.profile.fields:
                variable = self.field_vars[field.key]
                if field.kind == "bool":
                    value: object = variable.get()
                elif field.kind == "choice":
                    value = variable.get()
                    if isinstance(field.default, int):
                        value = next(
                            code for label, code in field.choices if label == value
                        )
                elif field.kind == "int":
                    text = str(variable.get()).strip()
                    numeric = float(text)
                    if not numeric.is_integer():
                        raise ValueError(f"{field.label} must be a whole number.")
                    value = int(numeric)
                else:
                    value = float(str(variable.get()).strip())
                values.append((field, validate_telemetry_value(field, value)))
        except (ValueError, StopIteration) as error:
            messagebox.showerror("Invalid telemetry value", str(error))
            return

        for field, value in values:
            if isinstance(self.node, BatteryNode):
                setattr(self.node, field.key, value)
            else:
                self.node.set_telemetry_value(field.key, value)
        self.app._log_event(
            "EVENT", f"{self.profile.label} telemetry values applied locally."
        )
        self.refresh()

    def _apply_test_command(self) -> None:
        if self.profile.control_id is None:
            return
        if not self.node.running or self.node.communication_lost:
            messagebox.showwarning(
                "Node must be communicating",
                "Start this module and restore its communications before testing commands.",
            )
            return
        try:
            action_text = self.test_action_var.get().split(" ", 1)[0]
            action = int(action_text, 16)
            target_voltage = float(self.target_voltage_var.get())
            current_limit = float(self.current_limit_var.get())
            secondary_limit = float(self.secondary_limit_var.get())
            if not all(
                math.isfinite(value)
                for value in (target_voltage, current_limit, secondary_limit)
            ):
                raise ValueError("Command values must be finite numbers.")
            if not 0 <= target_voltage <= 3276.75:
                raise ValueError("Target voltage must be between 0 and 3276.75 V.")
            if not -3276.8 <= current_limit <= 3276.7:
                raise ValueError("Current limit must be between -3276.8 and 3276.7 A.")
            if not -3276.8 <= secondary_limit <= 3276.7:
                raise ValueError("Secondary limit must be between -3276.8 and 3276.7 A.")
        except ValueError as error:
            messagebox.showerror("Invalid command values", str(error))
            return

        payload = bytes((action,))
        payload += round(target_voltage * 20).to_bytes(2, "little")
        payload += round(current_limit * 10).to_bytes(2, "little", signed=True)
        payload += round(secondary_limit * 10).to_bytes(2, "little", signed=True)
        frame = CanFrame(
            self.profile.control_id,
            payload + bytes((crc8_sae_j1850(payload),)),
        )
        self.app._log_frame(
            "TEST RX",
            frame,
            f"Local test command for {self.profile.label}; not sent on CAN.",
        )
        responses, events = self.node.handle_frame(frame, self.app._now_ms())
        for event in events:
            self.app._log_event(
                "TEST", event.message if isinstance(event, NodeEvent) else str(event)
            )
        for response in responses:
            self.app._log_frame(
                "TEST ACK",
                response,
                f"Local simulated response from {self.profile.label}; not sent on CAN.",
            )
        self.refresh()

    def refresh(self) -> None:
        if self.node.running:
            status = "Communication lost" if self.node.communication_lost else "Running"
            self.start_button.configure(text="Stop")
            self.loss_button.configure(state="normal")
            self.loss_button.configure(
                text=(
                    "Restore Communications"
                    if self.node.communication_lost
                    else "Simulate Node Loss"
                )
            )
        else:
            status = "Stopped"
            self.start_button.configure(text="Start")
            self.loss_button.configure(state="disabled", text="Simulate Node Loss")
        self.status_label.configure(text=f"{status} - {self.node.state_name}")
        if self.state_combo.focus_get() is not self.state_combo:
            self.state_var.set(self.node.state_name)

        for field in self.profile.fields:
            value = self._get_field_value(field.key)
            if field.kind == "choice":
                value_text = self._choice_label(field, value)
            elif field.kind == "bool":
                value_text = "Yes" if value else "No"
            else:
                value_text = str(value)
            self.current_labels[field.key].configure(text=f"Current: {value_text}")

        control = self.node.last_control
        if control is None:
            text = "No Coordinator command received."
        else:
            action, voltage, current, secondary, crc = control
            result = self.node.last_command
            command_payload = bytes((action,)) + voltage.to_bytes(2, "little")
            command_payload += current.to_bytes(2, "little", signed=True)
            command_payload += secondary.to_bytes(2, "little", signed=True)
            if crc8_sae_j1850(command_payload) != crc:
                status_text = "CRC invalid; command rejected"
            elif action == 0:
                status_text = "no ACK (no-op)"
            elif result is not None:
                status_text = f"ACK status 0x{result[1]:02X}"
            else:
                status_text = "awaiting command validation"
            text = (
                f"Last action 0x{action:02X}; target {voltage * 0.05:.2f} V; "
                f"limit {current * 0.1:.1f} A; secondary {secondary * 0.1:.1f} A; "
                f"{status_text}"
            )
        self.last_command_label.configure(text=text)
        self.test_command_button.configure(
            state=(
                "normal"
                if self.profile.control_id is not None
                and self.node.running
                and not self.node.communication_lost
                else "disabled"
            )
        )

        heartbeat = self.app.coordinator_status_text()
        self.coordinator_label.configure(text=heartbeat)
