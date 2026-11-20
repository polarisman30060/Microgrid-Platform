import unittest

from microgrid_simulator.protocol import (
    CanFrame,
    FrameFormatError,
    WaveshareFrameParser,
    crc8_sae_j1850,
    decode_coordinator_heartbeat,
    encode_battery_ack,
    encode_node_ack,
    encode_waveshare_frame,
)


class ProtocolTests(unittest.TestCase):
    def test_sae_j1850_crc_check_value(self):
        self.assertEqual(crc8_sae_j1850(b"123456789"), 0x4B)

    def test_encodes_waveshare_standard_frame_example(self):
        frame = CanFrame(0x123, bytes.fromhex("11 22 33 44 55 66 77 88"))
        self.assertEqual(
            encode_waveshare_frame(frame),
            bytes.fromhex("AA C8 23 01 11 22 33 44 55 66 77 88 55"),
        )

    def test_parser_handles_split_and_concatenated_frames(self):
        first = encode_waveshare_frame(CanFrame(0x100, b"\x01\x02"))
        second = encode_waveshare_frame(CanFrame(0x710, bytes(range(8))))
        parser = WaveshareFrameParser()

        self.assertEqual(parser.feed(first[:3]), [])
        self.assertEqual(parser.feed(first[3:] + second), [
            CanFrame(0x100, b"\x01\x02"),
            CanFrame(0x710, bytes(range(8))),
        ])

    def test_parser_resynchronizes_after_bad_terminator(self):
        good = encode_waveshare_frame(CanFrame(0x100, b"\x00"))
        parser = WaveshareFrameParser()
        parsed = parser.feed(b"\xAA\xC1\x01\x01\x00" + good)

        self.assertEqual(parsed, [CanFrame(0x100, b"\x00")])
        self.assertEqual(parser.malformed_packets, 1)

    def test_parser_rejects_standard_identifier_outside_11_bits(self):
        parser = WaveshareFrameParser()

        self.assertEqual(parser.feed(bytes.fromhex("AA C0 FF 7F 55")), [])
        self.assertEqual(parser.malformed_packets, 1)

    def test_rejects_out_of_range_standard_identifier(self):
        with self.assertRaises(FrameFormatError):
            encode_waveshare_frame(CanFrame(0x800, b""))

    def test_decodes_coordinator_heartbeat_fields(self):
        frame = CanFrame(0x100, bytes.fromhex("07 02 00 01 78 56 34 12"))

        heartbeat = decode_coordinator_heartbeat(frame)

        self.assertEqual(heartbeat.counter, 7)
        self.assertEqual(heartbeat.state, 2)
        self.assertEqual(heartbeat.active_node_count, 1)
        self.assertEqual(heartbeat.uptime_seconds, 0x12345678)

    def test_generic_ack_encoder_preserves_battery_wrapper(self):
        generic = encode_node_ack(0x760, 2, 0, 3, 1, 0x12345678)
        battery = encode_battery_ack(2, 0, 3, 1, 0x12345678)

        self.assertEqual(generic.identifier, 0x760)
        self.assertEqual(generic, battery)
        self.assertEqual(generic.data, bytes.fromhex("02 00 03 01 78 56 34 12"))


if __name__ == "__main__":
    unittest.main()
