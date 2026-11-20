"""CAN identifiers and node-address mapping for the approved registry draft."""

from __future__ import annotations

from dataclasses import dataclass

MAIN_BUS = "main"
SOLAR_SEGMENT = "solar_segment"

NODE_TYPE_BATTERY = 0x01
NODE_TYPE_BOOST = 0x02
NODE_TYPE_SOLAR = 0x03
NODE_TYPE_GENERATOR = 0x04
NODE_TYPE_GRID_INTERFACE = 0x05
NODE_TYPE_INVERTER = 0x06
NODE_TYPE_DISPLAY = 0x07
NODE_TYPE_COORDINATOR = 0x08
NODE_TYPE_SOLAR_GATEWAY = 0x09

MAX_INSTANCES = {
    NODE_TYPE_BATTERY: 5,
    NODE_TYPE_BOOST: 5,
    NODE_TYPE_SOLAR: 32,
    NODE_TYPE_GENERATOR: 2,
    NODE_TYPE_GRID_INTERFACE: 2,
    NODE_TYPE_INVERTER: 40,
    NODE_TYPE_DISPLAY: 1,
    NODE_TYPE_SOLAR_GATEWAY: 16,
}

_MAIN_GLOBAL_NODE_BASES = {
    NODE_TYPE_BATTERY: 0x01,
    NODE_TYPE_BOOST: 0x06,
    NODE_TYPE_GENERATOR: 0x0B,
    NODE_TYPE_GRID_INTERFACE: 0x0D,
    NODE_TYPE_INVERTER: 0x0F,
    NODE_TYPE_DISPLAY: 0x37,
    NODE_TYPE_SOLAR_GATEWAY: 0x38,
}


@dataclass(frozen=True)
class NodeCanIds:
    emergency_fault: int | None
    control: int
    heartbeat: int
    identity: int
    acknowledgement: int


@dataclass(frozen=True)
class SolarGatewayCanIds:
    global_node_id: int
    summary_primary: int
    summary_secondary: int
    child_detail: int
    child_command: int
    child_acknowledgement: int


def main_bus_global_node_id(node_type: int, instance: int) -> int:
    """Return a registry-assigned main-bus address for a non-Solar node."""
    if node_type == NODE_TYPE_SOLAR:
        raise ValueError("Solar MPPT nodes use a gateway-local address, not a main-bus ID.")
    count = MAX_INSTANCES.get(node_type)
    if count is None:
        raise ValueError(f"Node type 0x{node_type:02X} has no main-bus address allocation.")
    if not 0 <= instance < count:
        raise ValueError(
            f"Instance {instance} is outside the configured range 0..{count - 1} "
            f"for node type 0x{node_type:02X}."
        )
    return _MAIN_GLOBAL_NODE_BASES[node_type] + instance


def solar_local_address(instance: int) -> int:
    """Map Solar instance 0..31 to its segment-local CAN address 1..32."""
    count = MAX_INSTANCES[NODE_TYPE_SOLAR]
    if not 0 <= instance < count:
        raise ValueError(f"Solar instance must be between 0 and {count - 1}.")
    return instance + 1


def main_bus_node_ids(global_node_id: int) -> NodeCanIds:
    if not 0x01 <= global_node_id <= 0x47:
        raise ValueError("Main-bus GlobalNodeID must be in the assigned range 0x01..0x47.")
    return NodeCanIds(
        emergency_fault=global_node_id,
        control=0x120 + global_node_id,
        heartbeat=0x600 + global_node_id,
        identity=0x700 + global_node_id,
        acknowledgement=0x760 + global_node_id - 1,
    )


def solar_segment_node_ids(instance: int) -> NodeCanIds:
    address = solar_local_address(instance)
    return NodeCanIds(
        emergency_fault=None,
        control=0x120 + address,
        heartbeat=0x600 + address,
        identity=0x700 + address,
        acknowledgement=0x740 + address,
    )


def solar_gateway_can_ids(instance: int) -> SolarGatewayCanIds:
    count = MAX_INSTANCES[NODE_TYPE_SOLAR_GATEWAY]
    if not 0 <= instance < count:
        raise ValueError(f"Solar gateway instance must be between 0 and {count - 1}.")
    global_node_id = main_bus_global_node_id(NODE_TYPE_SOLAR_GATEWAY, instance)
    return SolarGatewayCanIds(
        global_node_id=global_node_id,
        summary_primary=0x430 + instance,
        summary_secondary=0x440 + instance,
        child_detail=0x450 + instance,
        child_command=0x1A0 + instance,
        child_acknowledgement=0x1B0 + instance,
    )
