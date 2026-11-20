import unittest

from microgrid_simulator.module_node import (
    BOOST_PROFILE,
    DISPLAY_PROFILE,
    GENERATOR_PROFILE,
    GRID_PROFILE,
    INVERTER_PROFILE,
    MODULE_PROFILES,
    SOLAR_PROFILE,
    ACK_ACCEPTED,
    ACK_REJECTED_CRC,
    ModuleNode,
    validate_telemetry_value,
)
from microgrid_simulator.battery_node import BatteryNode
from microgrid_simulator.protocol import CanFrame, crc8_sae_j1850


def command(profile, action, crc_valid=True):
    data = bytes((action,)) + bytes(6)
    crc = crc8_sae_j1850(data)
    if not crc_valid:
        crc ^= 0xFF
    return CanFrame(profile.control_id, data + bytes((crc,)))


class ModuleNodeTests(unittest.TestCase):
    def test_profiles_emit_identity_heartbeat_and_module_telemetry(self):
        for key, profile in MODULE_PROFILES.items():
            with self.subTest(profile=key):
                node = BatteryNode() if key == "battery" else ModuleNode(profile)
                node.start(0)

                frames = node.frames_due(0)
                identifiers = [frame.identifier for frame in frames]

                self.assertIn(profile.identity_id, identifiers)
                self.assertIn(profile.heartbeat_id, identifiers)
                if profile.telemetry_id is None:
                    self.assertEqual(len(frames), 2)
                else:
                    telemetry = next(
                        frame for frame in frames if frame.identifier == profile.telemetry_id
                    )
                    self.assertEqual(len(telemetry.data), 8)

    def test_gateway_emits_only_the_defined_identity_and_heartbeat_frames(self):
        profile = MODULE_PROFILES["solar_gateway"]
        node = ModuleNode(profile)
        node.start(0)

        identifiers = [frame.identifier for frame in node.frames_due(0)]

        self.assertEqual(set(identifiers), {profile.identity_id, profile.heartbeat_id})
        self.assertEqual(profile.heartbeat_id, 0x638)

    def test_initial_telemetry_wire_examples_cover_each_module(self):
        expected = {
            "boost": (0x300, "80 07 D8 0E 00 00 26 40"),
            "solar": (0x400, "D0 02 64 00 00 00 23 00"),
            "generator": (0x410, "80 07 00 00 00 00 26 00"),
            "grid_interface": (0x420, "B0 04 00 00 64 23 03 62"),
            "inverter": (0x500, "B0 04 00 00 00 00 64 00"),
        }
        for key, (identifier, data_hex) in expected.items():
            with self.subTest(profile=key):
                node = ModuleNode(MODULE_PROFILES[key])
                node.start(0)
                frame = next(
                    item
                    for item in node.frames_due(0)
                    if item.identifier == identifier
                )
                self.assertEqual(frame.data, bytes.fromhex(data_hex))

    def test_noop_has_no_ack_and_supported_command_uses_profile_ack(self):
        node = ModuleNode(SOLAR_PROFILE)
        node.start(0)

        noop_responses, _ = node.handle_frame(command(SOLAR_PROFILE, 0), 10)
        accepted_responses, _ = node.handle_frame(command(SOLAR_PROFILE, 1), 20)

        self.assertEqual(noop_responses, [])
        self.assertEqual(accepted_responses[0].identifier, SOLAR_PROFILE.ack_id)
        self.assertEqual(accepted_responses[0].data[1], ACK_ACCEPTED)
        self.assertTrue(node.get_telemetry_value("output_connected"))

    def test_invalid_crc_ack_follows_profile(self):
        solar = ModuleNode(SOLAR_PROFILE)
        solar.start(0)
        boost = ModuleNode(BOOST_PROFILE)
        boost.start(0)

        solar_responses, _ = solar.handle_frame(command(SOLAR_PROFILE, 1, False), 0)
        boost_responses, _ = boost.handle_frame(command(BOOST_PROFILE, 1, False), 0)

        self.assertEqual(solar_responses[0].data[1], ACK_REJECTED_CRC)
        self.assertEqual(boost_responses, [])

    def test_generator_start_advances_through_warmup_to_generating(self):
        node = ModuleNode(GENERATOR_PROFILE)
        node.start(0)
        heartbeat = CanFrame(0x100, bytes(8))
        node.handle_frame(heartbeat, 0)

        responses, _ = node.handle_frame(command(GENERATOR_PROFILE, 6), 0)
        self.assertEqual(responses[0].data[1], ACK_ACCEPTED)
        self.assertEqual(node.get_telemetry_value("generator_mode"), "Cranking")

        node.handle_frame(heartbeat, 500)
        node.update(1000)
        self.assertEqual(node.get_telemetry_value("generator_mode"), "Warmup")

        node.handle_frame(heartbeat, 1500)
        node.update(2000)
        self.assertEqual(node.get_telemetry_value("generator_mode"), "Generating")
        self.assertEqual(node.local_state, 3)

    def test_solar_child_accepts_gateway_forwarded_coordinator_heartbeat(self):
        node = ModuleNode(SOLAR_PROFILE)
        node.start(0)
        node.handle_frame(CanFrame(0x100, bytes(8)), 400)

        self.assertEqual(node.update(600), [])

    def test_profiles_use_registry_ids_without_same_bus_transmitter_collisions(self):
        tx_owners = {}
        for key, profile in MODULE_PROFILES.items():
            for identifier, _ in profile.tx_identifiers:
                tx_owners.setdefault((profile.network, identifier), []).append(key)

        conflicts = {
            bus_id: owners
            for bus_id, owners in tx_owners.items()
            if len(owners) > 1
        }
        self.assertEqual(conflicts, {})
        self.assertEqual(MODULE_PROFILES["battery"].global_node_id, 0x01)
        self.assertEqual(MODULE_PROFILES["boost"].global_node_id, 0x06)
        self.assertEqual(MODULE_PROFILES["generator"].global_node_id, 0x0B)
        self.assertEqual(MODULE_PROFILES["grid_interface"].global_node_id, 0x0D)
        self.assertEqual(MODULE_PROFILES["inverter"].global_node_id, 0x0F)
        self.assertEqual(MODULE_PROFILES["display"].global_node_id, 0x37)
        self.assertEqual(MODULE_PROFILES["solar_gateway"].global_node_id, 0x38)
        self.assertIsNone(SOLAR_PROFILE.global_node_id)
        self.assertEqual(SOLAR_PROFILE.local_address, 1)
        self.assertEqual(SOLAR_PROFILE.control_id, 0x121)
        self.assertEqual(MODULE_PROFILES["battery"].control_id, 0x121)
        self.assertEqual(MODULE_PROFILES["battery"].ack_id, 0x760)
        self.assertEqual(MODULE_PROFILES["battery"].heartbeat_period_ms, 250)
        self.assertEqual(MODULE_PROFILES["generator"].telemetry_period_ms, 500)

    def test_telemetry_validation_enforces_wire_range(self):
        field = next(field for field in GRID_PROFILE.fields if field.key == "grid_frequency_hz")

        self.assertEqual(validate_telemetry_value(field, 60.0), 60.0)
        with self.assertRaises(ValueError):
            validate_telemetry_value(field, 49.9)
        with self.assertRaises(ValueError):
            validate_telemetry_value(field, 75.6)


if __name__ == "__main__":
    unittest.main()
