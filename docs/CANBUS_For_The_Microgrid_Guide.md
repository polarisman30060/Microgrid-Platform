# Microgrid CANbus Guide: How Your System Talks

**A Plain-English Guide to How the Coordinator and Power Modules Communicate**

---

> **Implementation status:** This guide describes intended behavior, not complete firmware. The Coordinator currently sends the `100 ms` heartbeat and parses Battery 0 identity, heartbeat, and telemetry. It does not yet send node commands or decode Boost frames. The approved working registry removes main-bus ID collisions; Solar gateway payloads and physical gateway implementation remain open. No simulator controls power hardware. Use the [Coordinator CAN implementation guide](Coordinator_CAN_Implementation.md) and protocol definitions for current status.

## 1. What Is CANbus (and Why Are We Using It)?

Think of **CANbus** (Controller Area Network) as a shared group text or walkie-talkie channel over two copper wires (`CANH` and `CANL`). Main-bus modules share the Coordinator's CAN segment. Solar MPPT children instead use isolated local segments connected through Solar gateways.

### Why CANbus Works So Well for a Microgrid
* **Everyone on the Same Segment Hears Everything:** Nodes on one CAN segment receive the same frames. Solar MPPT children sit behind isolated gateway-local segments; their frames are filtered/aggregated upstream, not transparently repeated onto the main bus.
* **Built-In Priority (No Talking Over Each Other):** Every CAN message starts with a **Message ID number** (`0x001` up to `0x7FF`). **Lower numbers always win.** If a Boost converter tries to send a routine status update (`0x300`) at the exact same microsecond a Battery module screams "EMERGENCY SHORT CIRCUIT!" (`0x001`), the routine message automatically steps aside so the emergency message gets through immediately.
* **Tough Against Electrical Noise:** Your microgrid switches high voltages (`96VDC` and `380VDC`) and high currents. CANbus sends signals as the *difference* between two twisted wires, which cancels out electrical interference, and every board uses a **galvanic isolator** so high voltage can never zap the logic chips.
* **Classic CAN 2.0B (The Shared Language):** Your Waveshare ESP32-S3 Coordinator screen only speaks **Classic CAN 2.0B** (which sends up to **8 bytes** of data per message at **500,000 bits per second**). Even though your STM32G474 module brains are capable of a newer dialect called "CAN FD," we lock them into **Classic CAN mode** so the Coordinator understands every single word.

---

## 2. Meet the Crew (Who Is on the Bus?)

1. **The Coordinator (Waveshare ESP32-S3 Touchscreen):** The "Orchestra Conductor." It watches all the gauges, shows you what is happening on the 7-inch screen and web page, decides when to start the generator or charge batteries, and tells parallel modules how to share work. **Crucial Rule:** The Coordinator *never* does split-second electrical safety—it is too slow for that.
2. **The Module Brains (STM32G474 Microcontrollers):** The "Bodyguards." Every power module has its own fast STM32 chip. These chips run the high-speed power switching (`40,000` times a second) and watch for danger. If a wire shorts out, the STM32 shuts the power off in **less than 1 microsecond** without asking the Coordinator for permission.
3. **The 30V 50kHz Lifeline (The Backup Power Cord):** Even when the main `96V` and `380V` power buses are completely turned off, the Battery modules send a safe `30V 50kHz` power signal over a dedicated wire so every STM32 brain and the Coordinator screen stay awake and able to talk.

---

## 3. The Six Types of Messages on the Wire

To keep the conversation organized, we divide all CAN messages into priority lanes:

| ID Range            | Priority          | Nickname                   | What It Does                                                              | How Often?                                             |
| :------------------ | :---------------- | :------------------------- | :------------------------------------------------------------------------ | :----------------------------------------------------- |
| **`0x001`–`0x047`** | **1 (Highest)**   | **The Fire Alarm**         | Unique main-bus emergency ID equals the sending node's GlobalNodeID.      | Immediately when fault happens                         |
| **`0x100`–`0x11F`** | **2 (Very High)** | **The Conductor's Pulse**  | Coordinator says "I'm alive, here is the overall system mode."            | Every **100 ms** (10x/sec)                             |
| **`0x120`–`0x167`** | **3 (High)**      | **Marching Orders**        | Coordinator addresses a main-bus module using its registry ID.            | On change/request; parallel Boost/Inverter sharing uses **20 ms** |
| **`0x1A0`–`0x1BF`** | **3 (High)**      | **Solar Gateway Routing**  | Reserved child-command and reply IDs; payload is still being designed.    | Not implemented                                        |
| **`0x200`–`0x5FF`** | **4 (Medium)**    | **The Gauges (Telemetry)** | Modules report volts, amps, watts, RPM, temperatures, and battery %.      | Type-specific; Solar child traffic stays local        |
| **`0x600`–`0x6FF`** | **5 (Low)**       | **Roll Call (Heartbeats)** | Modules report presence and local state.                                  | **100 ms** to **1 s**, depending on node type          |
| **`0x700`–`0x7FF`** | **6 (Lowest)**    | **ID Badges & Receipts**   | Identity at startup/on request; ACK for state-changing commands.          | Event/on request                                       |

---

## 4. Walk-Through: What Happens During Real Events?

Here is a play-by-play look at the exact CAN messages flying back and forth during five everyday scenarios.

### Event 1: Cold "Black Start" (Waking Up from Completely Dead)
Imagine the grid is down, it's pitch black outside, and the whole house is off. You walk up and press the start button on a Battery module.

1. **Lifeline Wakes Up:** The Battery module turns on its internal `30V 50kHz` auxiliary power supply. Suddenly, the Coordinator screen and every STM32 board in the system get power and boot up—even though the main `96V` and `380V` buses are still at `0 Volts`.
2. **Roll Call (`0x600+` & `0x700+`):** Nodes send identity at startup/on request. Heartbeats are type-specific: Boost/Solar child `100 ms`, Battery/Inverter `250 ms`, and Generator/Grid/Display `1 s`.
3. **Safety Check:** The Coordinator checks its saved checklist:
   * *Are all "Required" modules present?* Yes.
   * *Did two modules accidentally get programmed with the same ID number?* The Coordinator checks the serial numbers. No duplicates!
4. **Step-by-Step Energizing (`0x120+` & `0x740+`):** You tap **Start System** on the touchscreen. The Coordinator turns things on in strict upstream-to-downstream order:
   * **Step A (96V Bus):** Coordinator sends Battery 0 `CMD_NODE_CONTROL` (`0x121`); Battery 0 replies to a state-changing command on ACK ID `0x760`.
   * **Step B (380V Bus):** Next, the Coordinator tells Boost 0 on `0x126`; Boost 0 replies on ACK ID `0x765`.
   * **Step C (House AC Power):** Finally, the Coordinator tells the AC Inverter modules: *"Turn on 120V/240V output!"* One by one, the inverters close their output breakers, and your lights come on.

---

### Event 2: Big Appliance Surge (Current Balancing Tokens in Action)
Suppose you have **two 10kVA Boost converters** working together to feed the `380V` bus, and your central air conditioner compressor kicks on.

1. **Instant Local Reaction (Microseconds):** Before the Coordinator even knows the AC turned on, the big capacitor banks on the `380V` bus absorb the first split-second of shock, and both Boost converters' local STM32 chips instantly push more current to hold `380V`.
2. **Telemetry Arrives (`0x300` & `0x301`):** Within **50 ms**, both Boost converters report their numbers on the main bus. The Coordinator notices Boost #1 is carrying `60 Amps` while Boost #2 is only carrying `40 Amps` (total `100 Amps`).
3. **Current Balancing Tokens (`0x120+` only when needed):** If active parallel balancing requires it, the Coordinator sends updates at a provisional **20 ms** cadence:
   * To Boost #1: *"Your target share is `50.0 Amps`."*
   * To Boost #2: *"Your target share is `50.0 Amps`."*
4. **Smooth Sharing:** Both converters nudge their current loops so they split the load `50/50`, preventing one converter from overheating while the other loafs.

---

### Event 3: A Short Circuit in a House Outlet (Hardware Safety vs. Coordinator)
Suppose someone plugs in a faulty space heater with a dead short circuit inside it.

1. **Sub-Microsecond Trip (No CAN Needed):** The AC Inverter's STM32 chip sees the current spike past `40 Amps` in less than **1 microsecond**. Its hardware fault pin (`HRTIM_FLT`) immediately shuts off the switching transistors and pops open the physical air-gap relay. That single circuit goes dead, while the rest of the house keeps running normally.
2. **The Fire Alarm (`GlobalNodeID`):** Immediately after tripping, Inverter instance 2 (registry `GlobalNodeID 0x11`) broadcasts `EMERGENCY_FAULT` on `0x011` at highest priority:
   * *"I am Inverter #2 (`GlobalNodeID 0x11`), I am Locked Out, Fault Bit 0 (Hardware Overcurrent) tripped at `118.4V` and `44.2A`!"*
3. **Coordinator Logs & Alerts:** The Coordinator hears `0x006`, logs the exact voltage/amp snapshot to its MicroSD card, and pops up a red **Breaker Tripped** alert on your 7-inch screen and phone web page.
4. **Remote Reset (`0x120+` & registry ACK):** Once you unplug the broken heater, you tap **Reset Breaker** on the web page. The Coordinator sends `Command_Action = 0x05 (Reset Breaker)` to Inverter #2 on `0x131`. The node replies on its assigned main-bus ACK ID.

---

### Event 4: The Coordinator Crashes or Unplugs (`DEGRADED_AUTONOMOUS` Mode)
What happens if you accidentally yank the CAN cable out of the Waveshare touchscreen, or reboot it to update its web page while your house is running?

1. **The 500 ms Coordinator Watchdog:** Normally, the Coordinator sends `COORD_HEARTBEAT` (`0x100`) every **100 ms**. Each module uses its local `500 ms` Coordinator watchdog to decide when the Coordinator is absent. This is separate from the Coordinator's type-specific missing-node deadlines.
2. **No Blackout! Safe Autopilot:** Instead of shutting off your house, every module switches into **`DEGRADED_AUTONOMOUS`** mode using pre-agreed rules:
   * **Battery Modules:** Keep holding the `96V` bus steady using their last known safe limits.
   * **Solar MPPTs:** Keep harvesting sunshine, automatically tapering back if the `96V` bus approaches `100.8V`.
   * **Boost Converters (Primary vs. Secondary):** Without the Coordinator's `20 ms` balancing tokens, two parallel Boost converters might fight over who controls the `380V` bus. So **Boost #0 (Designated Primary)** keeps regulating `380V` on its own, while **Boost #1 (Secondary)** politely steps into Standby. Your house stays powered!
   * **Generator & Grid Charger:** Since nobody is supervising them, they safely ramp down and disconnect so they don't overcharge the batteries.
3. **Seamless Recovery:** As soon as you plug the CAN cable back in and `COORD_HEARTBEAT` (`0x100`) reappears, the Coordinator brings Secondary Boost #1 back online and resumes normal balanced operation.

---

### Event 5: Adding a New Roof Solar Panel (Gateway Provisioning Concept)
Per-panel Solar MPPT boxes are intended to connect to a local segment behind a Solar gateway. The physical gateway and provisioning payload still need design.

1. **Capacity:** The working registry allows 16 gateways with 32 Solar child slots each, enough for the 500-node target. Solar child IDs are local to the gateway segment, not main-bus addresses.
2. **Web UI Setup:** You open the Coordinator's web page on your phone, see *"New Unconfigured Solar MPPT Detected (Serial `0x8821C004`)"*, and type in *"Assign as Solar Panel #4"*.
3. **Provisioning:** `0x7C0` remains reserved for provisioning, but how the Coordinator reaches the correct gateway/child and assigns a local slot is not yet defined. Solar child traffic must not be transparently bridged onto the main bus.

---

## 5. Golden Rules to Remember

* **Type-Specific Heartbeats:** Boost/Solar child `100 ms`, Battery/Inverter `250 ms`, Generator/Grid/Display `1 s`; missing-node deadlines are `500 ms` for Boost/Solar gateway, `1 s` for Battery/Inverter, and `3 s` for Generator/Grid/Display.
* **Conditional Balancing:** Active parallel Boost and Inverter modules require `20 ms` current-sharing updates. Non-paralleled Inverters do not need that routine cadence; Inverter sharing details and bus capacity remain to be validated.
* **Gateway-Local Solar:** Solar children report locally; gateways aggregate and limit upstream traffic.
* **Safety Never Waits for CAN:** Hardware comparators and local STM32 chips handle microsecond emergencies; CANbus handles coordination, balancing, and visibility.
