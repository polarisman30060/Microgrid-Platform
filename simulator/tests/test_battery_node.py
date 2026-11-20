import unittest

from microgrid_simulator.battery_node import (
    ACK_ACCEPTED,
    ACK_REJECTED_INVALID_STATE,
    ACK_REJECTED_LOCAL_FAULT,
    BatteryNode,
)
from microgrid_simulator.protocol import (
    BATTERY_ACK_ID,
    BATTERY_COMMAND_ID,
    BATTERY_FAST_TELEMETRY_ID,
    BATTERY_HEARTBEAT_ID,
    BATTERY_IDENTITY_ID,
    BATTERY_SLOW_TELEMETRY_ID,
    CanFrame,
    crc8_sae_j1850,
)


def command(
    action: int,
    crc_valid: bool = True,
    target_voltage_raw: int = 0,
    current_limit_raw: int = 0,
    secondary_limit_raw: int = 0,
) -> CanFrame:
    data = bytes((action,))
    data += target_voltage_raw.to_bytes(2, "little")
    data += current_limit_raw.to_bytes(2, "little", signed=True)
    data += secondary_limit_raw.to_bytes(2, "little", signed=True)
    crc = crc8_sae_j1850(data) if crc_valid else crc8_sae_j1850(data) ^ 0xFF
    return CanFrame(BATTERY_COMMAND_ID, data + bytes((crc,)))


class BatteryNodeTests(unittest.TestCase):
    def test_initial_start_emits_normal_identity_heartbeat_and_telemetry(self):
        node = BatteryNode()
        node.start(1000)

        frames = node.frames_due(1000)

        self.assertEqual(
            [frame.identifier for frame in frames],
            [
                BATTERY_IDENTITY_ID,
                BATTERY_HEARTBEAT_ID,
                BATTERY_FAST_TELEMETRY_ID,
                BATTERY_SLOW_TELEMETRY_ID,
            ],
        )
        self.assertEqual(frames[0].data[4:6], b"\x01\x00")
        self.assertEqual(frames[1].data[2], 2)

    def test_telemetry_periods_are_deterministic(self):
        node = BatteryNode()
        node.start(0)
        node.frames_due(0)

        self.assertEqual(
            [frame.identifier for frame in node.frames_due(50)],
            [],
        )
        self.assertEqual(
            [frame.identifier for frame in node.frames_due(100)],
            [BATTERY_FAST_TELEMETRY_ID],
        )
        self.assertEqual(
            [frame.identifier for frame in node.frames_due(250)],
            [BATTERY_HEARTBEAT_ID, BATTERY_FAST_TELEMETRY_ID],
        )
        self.assertEqual(
            [frame.identifier for frame in node.frames_due(1000)],
            [BATTERY_HEARTBEAT_ID, BATTERY_FAST_TELEMETRY_ID, BATTERY_SLOW_TELEMETRY_ID],
        )

    def test_identity_is_not_periodically_retransmitted(self):
        node = BatteryNode()
        node.start(0)
        node.frames_due(0)

        self.assertNotIn(
            BATTERY_IDENTITY_ID,
            [frame.identifier for frame in node.frames_due(1000)],
        )

    def test_valid_precharge_then_enable_commands_change_simulated_state(self):
        node = BatteryNode()
        node.start(0)

        precharge_acks, _ = node.handle_frame(command(1), 10)
        run_acks, _ = node.handle_frame(command(2), 20)

        self.assertEqual(precharge_acks[0].identifier, BATTERY_ACK_ID)
        self.assertEqual(precharge_acks[0].data[1], ACK_ACCEPTED)
        self.assertEqual(run_acks[0].data[1], ACK_ACCEPTED)
        self.assertEqual(node.local_state, 3)

    def test_running_command_without_closed_contactor_is_rejected(self):
        node = BatteryNode()
        node.start(0)

        acks, _ = node.handle_frame(command(2), 0)

        self.assertEqual(acks[0].data[1], ACK_REJECTED_INVALID_STATE)

    def test_fault_rejects_state_changing_command(self):
        node = BatteryNode()
        node.start(0)
        node.inject_fault()

        acks, _ = node.handle_frame(command(1), 0)

        self.assertEqual(acks[0].data[1], ACK_REJECTED_LOCAL_FAULT)
        self.assertEqual(node.local_state, 5)

    def test_precharge_is_rejected_when_pack_voltage_delta_exceeds_limit(self):
        node = BatteryNode()
        node.start(0)
        node.pack_voltage_v = 90.0

        acks, _ = node.handle_frame(command(1), 0)

        self.assertEqual(acks[0].data[1], 0x03)
        self.assertFalse(node.main_contactor_closed)

    def test_bad_crc_is_dropped_without_ack(self):
        node = BatteryNode()
        node.start(0)

        acks, events = node.handle_frame(command(0, crc_valid=False), 0)

        self.assertEqual(acks, [])
        self.assertIn("invalid CRC", events[0].message)

    def test_noop_updates_setpoints_without_sending_ack(self):
        node = BatteryNode()
        node.start(0)

        acks, events = node.handle_frame(
            command(0, target_voltage_raw=1920, current_limit_raw=125, secondary_limit_raw=-20),
            10,
        )

        self.assertEqual(acks, [])
        self.assertEqual(node.target_voltage_setpoint, 1920)
        self.assertEqual(node.charge_current_limit_deciamps, 125)
        self.assertEqual(node.discharge_current_limit_deciamps, -20)
        self.assertIn("no ACK", events[0].message)

    def test_reset_command_clears_a_fault_and_is_acknowledged(self):
        node = BatteryNode()
        node.start(0)
        node.inject_fault()

        acks, _ = node.handle_frame(command(5), 10)

        self.assertEqual(acks[0].data[1], ACK_ACCEPTED)
        self.assertEqual(node.local_state, 2)
        self.assertEqual(node.fault_summary, 0)

    def test_node_loss_suppresses_frames_until_restored(self):
        node = BatteryNode()
        node.start(0)
        node.set_communication_lost(True, 10)

        self.assertEqual(node.frames_due(1000), [])
        node.set_communication_lost(False, 1000)
        self.assertEqual(len(node.frames_due(1000)), 4)

    def test_coordinator_timeout_enters_degraded_and_sets_warning(self):
        node = BatteryNode()
        node.start(0)
        node.handle_frame(CanFrame(0x100, bytes(8)), 0)

        events = node.update(501)

        self.assertEqual(node.local_state, 4)
        self.assertEqual(node.warning_flags & (1 << 3), 1 << 3)
        self.assertIn("timed out", events[0].message)

    def test_coordinator_recovery_clears_warning_but_keeps_local_state(self):
        node = BatteryNode()
        node.start(0)
        node.update(501)
        node.handle_frame(CanFrame(0x100, bytes(8)), 600)

        events = node.update(601)

        self.assertEqual(node.local_state, 4)
        self.assertEqual(node.warning_flags & (1 << 3), 0)
        self.assertIn("restored", events[0].message)


if __name__ == "__main__":
    unittest.main()
