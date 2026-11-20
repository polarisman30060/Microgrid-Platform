import unittest

from microgrid_simulator.registry import (
    MAX_INSTANCES,
    NODE_TYPE_BATTERY,
    NODE_TYPE_BOOST,
    NODE_TYPE_DISPLAY,
    NODE_TYPE_GENERATOR,
    NODE_TYPE_GRID_INTERFACE,
    NODE_TYPE_INVERTER,
    NODE_TYPE_SOLAR,
    NODE_TYPE_SOLAR_GATEWAY,
    main_bus_global_node_id,
    main_bus_node_ids,
    solar_gateway_can_ids,
    solar_local_address,
    solar_segment_node_ids,
)


class RegistryTests(unittest.TestCase):
    def test_main_bus_global_node_ids_are_contiguous_and_match_approved_ranges(self):
        types = (
            NODE_TYPE_BATTERY,
            NODE_TYPE_BOOST,
            NODE_TYPE_GENERATOR,
            NODE_TYPE_GRID_INTERFACE,
            NODE_TYPE_INVERTER,
            NODE_TYPE_DISPLAY,
            NODE_TYPE_SOLAR_GATEWAY,
        )
        global_ids = [
            main_bus_global_node_id(node_type, instance)
            for node_type in types
            for instance in range(MAX_INSTANCES[node_type])
        ]

        self.assertEqual(global_ids, list(range(0x01, 0x48)))
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_BATTERY, 4), 0x05)
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_BOOST, 4), 0x0A)
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_GENERATOR, 1), 0x0C)
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_GRID_INTERFACE, 1), 0x0E)
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_INVERTER, 39), 0x36)
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_DISPLAY, 0), 0x37)
        self.assertEqual(main_bus_global_node_id(NODE_TYPE_SOLAR_GATEWAY, 15), 0x47)

    def test_assigned_identifiers_are_unique_on_each_physical_bus(self):
        main_ids = [0x100, 0x120, 0x7C0]
        telemetry_bases = {
            NODE_TYPE_BATTERY: (0x200, 0x210),
            NODE_TYPE_BOOST: (0x300,),
            NODE_TYPE_GENERATOR: (0x410,),
            NODE_TYPE_GRID_INTERFACE: (0x420,),
            NODE_TYPE_INVERTER: (0x500,),
            NODE_TYPE_DISPLAY: (),
            NODE_TYPE_SOLAR_GATEWAY: (),
        }
        node_types = (
            NODE_TYPE_BATTERY,
            NODE_TYPE_BOOST,
            NODE_TYPE_GENERATOR,
            NODE_TYPE_GRID_INTERFACE,
            NODE_TYPE_INVERTER,
            NODE_TYPE_DISPLAY,
            NODE_TYPE_SOLAR_GATEWAY,
        )
        for node_type in node_types:
            for instance in range(MAX_INSTANCES[node_type]):
                global_id = main_bus_global_node_id(node_type, instance)
                ids = main_bus_node_ids(global_id)
                main_ids.extend(
                    (
                        ids.emergency_fault,
                        ids.control,
                        ids.heartbeat,
                        ids.identity,
                        ids.acknowledgement,
                    )
                )
                main_ids.extend(
                    base + instance for base in telemetry_bases[node_type]
                )
                if node_type == NODE_TYPE_SOLAR_GATEWAY:
                    gateway_ids = solar_gateway_can_ids(instance)
                    main_ids.extend(
                        (
                            gateway_ids.summary_primary,
                            gateway_ids.summary_secondary,
                            gateway_ids.child_detail,
                            gateway_ids.child_command,
                            gateway_ids.child_acknowledgement,
                        )
                    )

        solar_segment_ids = [0x100]
        for instance in range(MAX_INSTANCES[NODE_TYPE_SOLAR]):
            ids = solar_segment_node_ids(instance)
            solar_segment_ids.extend(
                (
                    ids.control,
                    0x400 + instance,
                    ids.heartbeat,
                    ids.identity,
                    ids.acknowledgement,
                )
            )

        self.assertEqual(len(main_ids), len(set(main_ids)))
        self.assertEqual(len(solar_segment_ids), len(set(solar_segment_ids)))
        self.assertIsNone(solar_segment_node_ids(0).emergency_fault)

    def test_solar_nodes_are_addressed_locally_and_gateway_ids_are_reserved(self):
        self.assertEqual(solar_local_address(0), 1)
        self.assertEqual(solar_local_address(31), 32)
        self.assertEqual(solar_segment_node_ids(0).control, 0x121)
        self.assertEqual(solar_segment_node_ids(31).heartbeat, 0x620)

        gateway = solar_gateway_can_ids(0)
        self.assertEqual(gateway.global_node_id, 0x38)
        self.assertEqual(gateway.summary_primary, 0x430)
        self.assertEqual(gateway.summary_secondary, 0x440)
        self.assertEqual(gateway.child_detail, 0x450)
        self.assertEqual(gateway.child_command, 0x1A0)
        self.assertEqual(gateway.child_acknowledgement, 0x1B0)
        self.assertEqual(solar_gateway_can_ids(15).child_command, 0x1AF)

    def test_registry_rejects_out_of_range_instances_and_solar_global_ids(self):
        with self.assertRaises(ValueError):
            main_bus_global_node_id(NODE_TYPE_BATTERY, 5)
        with self.assertRaises(ValueError):
            main_bus_global_node_id(NODE_TYPE_SOLAR, 0)
        with self.assertRaises(ValueError):
            solar_local_address(32)
        with self.assertRaises(ValueError):
            solar_gateway_can_ids(16)
        with self.assertRaises(ValueError):
            main_bus_node_ids(0x48)


if __name__ == "__main__":
    unittest.main()
