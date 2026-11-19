# Microgrid CANbus Guide: How Your System Talks

**A Plain-English Guide to How the Coordinator and Power Modules Communicate**

---

> **Implementation status:** This guide mostly describes intended behavior, not implemented firmware. The Coordinator currently sends the `100 ms` coordinator heartbeat and parses the proposed Battery 0 identity, heartbeat, and telemetry frames. It does not yet send Battery commands, implement full production discovery/control, or simulate power flow. Some example frame assignments and workflows remain unapproved. Use the [Coordinator CAN implementation guide](Coordinator_CAN_Implementation.md) and protocol draft for development.

## 1. What Is CANbus (and Why Are We Using It)?

Think of **CANbus** (Controller Area Network) as a shared group text or walkie-talkie channel over two copper wires (`CANH` and `CANL`). Every module in your microgrid—the Coordinator screen, the Battery modules, the Boost converters, the Solar MPPTs, the Generator, and the AC Inverters—is connected to those same two wires.

### Why CANbus Works So Well for a Microgrid
* **Everyone Hears Everything:** When a module puts a message on the wire, every other module sees it at the exact same time. There is no central router forwarding mail one by one.
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
| **`0x001`–`0x01F`** | **1 (Highest)**   | **The Fire Alarm**         | Instant broadcast when a module trips a hardware safety limit.            | Immediately when fault happens                         |
| **`0x100`–`0x11F`** | **2 (Very High)** | **The Conductor's Pulse**  | Coordinator says "I'm alive, here is the overall system mode."            | Every **100 ms** (10x/sec)                             |
| **`0x120`–`0x1FF`** | **3 (High)**      | **Marching Orders**        | Coordinator tells specific modules to turn on, reset, or share current.   | Every **20 ms** (50x/sec) for active converters        |
| **`0x200`–`0x5FF`** | **4 (Medium)**    | **The Gauges (Telemetry)** | Modules report volts, amps, watts, RPM, temperatures, and battery %.      | Fast: **50 ms** (20x/sec)<br>Slow: **250 ms** (4x/sec) |
| **`0x600`–`0x6FF`** | **5 (Low)**       | **Roll Call (Heartbeats)** | Every module says "I'm still here and my state is Ready/Running."         | Every **100 ms** (10x/sec)                             |
| **`0x700`–`0x7FF`** | **6 (Lowest)**    | **ID Badges & Receipts**   | Modules share their permanent Serial Number, and send "Got it!" receipts. | Every **1000 ms** (1x/sec) or when asked               |

---

## 4. Walk-Through: What Happens During Real Events?

Here is a play-by-play look at the exact CAN messages flying back and forth during five everyday scenarios.

### Event 1: Cold "Black Start" (Waking Up from Completely Dead)
Imagine the grid is down, it's pitch black outside, and the whole house is off. You walk up and press the start button on a Battery module.

1. **Lifeline Wakes Up:** The Battery module turns on its internal `30V 50kHz` auxiliary power supply. Suddenly, the Coordinator screen and every STM32 board in the system get power and boot up—even though the main `96V` and `380V` buses are still at `0 Volts`.
2. **Roll Call (`0x600+` & `0x700+`):** Every module starts sending a `NODE_HEARTBEAT` every **100 ms** ("I'm Battery #1, I'm Ready!") and a `NODE_IDENTITY` badge every **1000 ms** ("My permanent serial number is `0x1048A92B`").
3. **Safety Check:** The Coordinator checks its saved checklist:
   * *Are all "Required" modules present?* Yes.
   * *Did two modules accidentally get programmed with the same ID number?* The Coordinator checks the serial numbers. No duplicates!
4. **Step-by-Step Energizing (`0x120+` & `0x740+`):** You tap **Start System** on the touchscreen. The Coordinator turns things on in strict upstream-to-downstream order:
   * **Step A (96V Bus):** Coordinator sends `CMD_NODE_CONTROL` (`0x120+`) to the Battery module: *"Precharge the 96V bus!"* The Battery closes a small relay through a resistor (`Q_PRE`) to gently fill the `96V` capacitors without a giant spark. Once the voltage matches within `1.5V`, it slams shut the big main contactor (`Q_ISOL`) and replies with a `CMD_ACK` (`0x740+`) receipt: *"Done, 96V bus is live!"*
   * **Step B (380V Bus):** Next, the Coordinator tells the Boost converter: *"Precharge and step up to 380V!"* The Boost converter smoothly ramps the `380V` bus up and sends back its `CMD_ACK` receipt.
   * **Step C (House AC Power):** Finally, the Coordinator tells the AC Inverter modules: *"Turn on 120V/240V output!"* One by one, the inverters close their output breakers, and your lights come on.

---

### Event 2: Big Appliance Surge (Current Balancing Tokens in Action)
Suppose you have **two 10kVA Boost converters** working together to feed the `380V` bus, and your central air conditioner compressor kicks on.

1. **Instant Local Reaction (Microseconds):** Before the Coordinator even knows the AC turned on, the big capacitor banks on the `380V` bus absorb the first split-second of shock, and both Boost converters' local STM32 chips instantly push more current to hold `380V`.
2. **Telemetry Arrives (`0x300` & `0x301`):** Within **50 ms**, both Boost converters report their numbers on the CAN bus. The Coordinator notices Boost #1 is carrying `60 Amps` while Boost #2 is only carrying `40 Amps` (total `100 Amps`).
3. **Current Balancing Tokens (`0x120+` every 20 ms):** Fifty times every second (**every 20 ms**), the Coordinator sends a tiny setpoint message called a **Current Balancing Token** to each Boost converter:
   * To Boost #1: *"Your target share is `50.0 Amps`."*
   * To Boost #2: *"Your target share is `50.0 Amps`."*
4. **Smooth Sharing:** Both converters nudge their current loops so they split the load `50/50`, preventing one converter from overheating while the other loafs.

---

### Event 3: A Short Circuit in a House Outlet (Hardware Safety vs. Coordinator)
Suppose someone plugs in a faulty space heater with a dead short circuit inside it.

1. **Sub-Microsecond Trip (No CAN Needed):** The AC Inverter's STM32 chip sees the current spike past `40 Amps` in less than **1 microsecond**. Its hardware fault pin (`HRTIM_FLT`) immediately shuts off the switching transistors and pops open the physical air-gap relay. That single circuit goes dead, while the rest of the house keeps running normally.
2. **The Fire Alarm (`0x001+`):** Immediately after tripping, the Inverter broadcasts an `EMERGENCY_FAULT` (`0x006`) at highest priority:
   * *"I am Inverter #2 (`GlobalNodeID`), I am Locked Out, Fault Bit 0 (Hardware Overcurrent) tripped at `118.4V` and `44.2A`!"*
3. **Coordinator Logs & Alerts:** The Coordinator hears `0x006`, logs the exact voltage/amp snapshot to its MicroSD card, and pops up a red **Breaker Tripped** alert on your 7-inch screen and phone web page.
4. **Remote Reset (`0x120+` & `0x740+`):** Once you unplug the broken heater, you tap **Reset Breaker** on the web page. The Coordinator sends `Command_Action = 0x05 (Reset Breaker)` to Inverter #2. Inverter #2 checks that the short is gone, closes its relay, and sends back `CMD_ACK` (`Ack_Status = Accepted`).

---

### Event 4: The Coordinator Crashes or Unplugs (`DEGRADED_AUTONOMOUS` Mode)
What happens if you accidentally yank the CAN cable out of the Waveshare touchscreen, or reboot it to update its web page while your house is running?

1. **The 500 ms Watchdog:** Normally, the Coordinator sends `COORD_HEARTBEAT` (`0x100`) every **100 ms**. Every module runs a countdown timer. If **500 ms** (half a second, or 5 missed beats) goes by without hearing from the Coordinator, every module realizes: *"The boss went offline!"*
2. **No Blackout! Safe Autopilot:** Instead of shutting off your house, every module switches into **`DEGRADED_AUTONOMOUS`** mode using pre-agreed rules:
   * **Battery Modules:** Keep holding the `96V` bus steady using their last known safe limits.
   * **Solar MPPTs:** Keep harvesting sunshine, automatically tapering back if the `96V` bus approaches `100.8V`.
   * **Boost Converters (Primary vs. Secondary):** Without the Coordinator's `20 ms` balancing tokens, two parallel Boost converters might fight over who controls the `380V` bus. So **Boost #0 (Designated Primary)** keeps regulating `380V` on its own, while **Boost #1 (Secondary)** politely steps into Standby. Your house stays powered!
   * **Generator & Grid Charger:** Since nobody is supervising them, they safely ramp down and disconnect so they don't overcharge the batteries.
3. **Seamless Recovery:** As soon as you plug the CAN cable back in and `COORD_HEARTBEAT` (`0x100`) reappears, the Coordinator brings Secondary Boost #1 back online and resumes normal balanced operation.

---

### Event 5: Adding a New Roof Solar Panel (Remote Provisioning)
Because per-panel Solar MPPT boxes live bolted underneath solar panels on a hot roof, they don't have screens or buttons.

1. **Factory Default:** You bolt a brand-new Solar MPPT onto the roof and plug in its `30V 50kHz` and CAN wires. It boots up with a default unconfigured ID and starts broadcasting its permanent factory serial number on `NODE_IDENTITY` (`0x700+`).
2. **Web UI Setup:** You open the Coordinator's web page on your phone, see *"New Unconfigured Solar MPPT Detected (Serial `0x8821C004`)"*, and type in *"Assign as Solar Panel #4"*.
3. **Over-the-Wire Assignment (`0x780`):** The Coordinator broadcasts `CMD_REMOTE_PROVISION` (`0x780`) containing serial number `0x8821C004` and new ID `4`. Only that specific rooftop box accepts the command, saves ID `4` into its permanent flash memory, and starts reporting solar watts on `0x404`!

---

## 5. Golden Rules to Remember

* **100 ms Heartbeats / 500 ms Timeout:** Everyone checks in 10 times a second; half a second of silence means someone is offline.
* **20 ms Balancing Tokens:** Parallel converters get fresh load-sharing numbers 50 times a second.
* **50 ms Fast Gauges / 250 ms Slow Gauges:** Volts and amps update 20 times a second; temperatures and cell voltages update 4 times a second.
* **Safety Never Waits for CAN:** Hardware comparators and local STM32 chips handle microsecond emergencies; CANbus handles coordination, balancing, and visibility.
