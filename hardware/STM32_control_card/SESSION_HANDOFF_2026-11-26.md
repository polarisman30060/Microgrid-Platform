# STM32 Control Card — PCB Layout & Routing Session Handoff (2026-11-26)

## 1. Role of This Board in the Microgrid Platform

The **Universal STM32 Control Card** (`hardware/STM32_control_card/`) is a reusable **4-layer mixed-signal daughter card** (`STM32G474QET6`, LQFP-128) designed to plug downward onto every accessible power-electronics host board in the Microgrid Platform:

- **Supported Host Power Modules:** `10 kW` 2-Phase Interleaved FB-LLC Boost (`96 V → ±200 V / 400 VDC`), `2.5 kW` Isolated Bidirectional DAB Battery Module, Half-Bridge Inverter (`120/240 VAC`), Generator Interface, and Two-Stage Isolated Utility-Tie (rooftop Solar MPPT uses a separate compact controller).
- **Power Partition:** Receives isolated `24.0 VDC` (`AUX_LOGIC_24V`, `21.6–28.0 V` operating, always-on lifeline that stays live during E-Stop/faults) via `J1` pins 1–4. Protected by `MF-R050` PTC + `B360A-13-F` reverse-polarity diode + `SMBJ30A` TVS (`V_C max = 48.4 V`), stepped down by `LM2576HVT-5.0/NOPB` (`60 V` TO-220-5 buck + `Bourns SRR1280-331K` `330 µH` inductor) to `+5V_CTRL`, regulated to `+3V3_DIG` by `TLV76133DCYR` (`SOT-223`, Tab Pin 4 = `3V3_DIG` output — **not** GND), and ferrite-filtered (`BLM21AG601SN1D`) to `+3V3_ANA` (`VDDA`/`VREF+`).
- **5 kVrms Reinforced Isolated CAN Bus:** Classical CAN 2.0B (`500 kbps`, 11-bit IDs) using `TI ISO1042DWV` (`8-SOICW`, `10.90 mm` pad span) powered by stocked `Analog Devices ADuM6020-5BRIZ` (`16-SOICW`, `9.50 mm` pad span, regulated `5.0 V ±5%` down to `0 mA` idle), with `NUP2105LT1G` ESD, selectable split termination (`R16`/`R17`/`C28`/`H2`), and `Phoenix 1729131` 3-pin `5.08 mm` screw terminal (`J_CAN`). Requires a strict copper-free galvanic moat across **all 4 layers**.
- **Human Interface & Bench Usability:**
  - Directly mounts a **Waveshare `1.3"` SPI IPS LCD + 8-input keypad HAT** (`ST7789`, `240×240`) on the **top side** via `J4` (`2×20` male header) + **4× M2.5 (`2.75 mm` drill) standoffs** (`58.0 × 23.0 mm` RPi Zero hole pattern; HAT spans `X = 14.5..79.5 mm`, `Y = 25.3..55.5 mm`, `11 mm` tall — only flat SMD parts under the HAT).
  - **Bottom-side host headers `J1` (Analog/24V Power) and `J2` (HRTIM1 PWM/Fault/Sync)** (`SBH11-PBPC-D20-ST-BK` keyed shrouded `2×20` headers) are locked on an exact **`78.740 mm` (`31 × 2.54 mm`) stride, `ΔY = 0.000 mm`** so they mate stress-free into host `SFH11` sockets. `J3` (`2×12` expansion) shares the exact `2.54 mm` grid.
  - **Hand-Solderability & Silkscreen Requirement:** The board will be **hand-soldered** (`0805` passives, wide-body `1.27 mm` SOICs, `TO-220-5` buck). All components require generous iron-tip clearance (`≥ 1.2 mm` physical body air gap) and **100% intact, upright, non-overlapping silkscreen designators**.
  - **Serviceability with HAT Mounted:** Reset button (`SW1`), status LEDs (`LED1`/`LED2`), `BOOT0` jumper, and `J5` (`1×6` SWD debug header) must remain accessible/visible even while the Waveshare LCD HAT is installed.

---

## 2. What Was Completed & Saved Today (2026-11-26)

### A. Schematic & Documentation (Verified)
- All 4 sheets in `hardware/STM32_control_card/Schematic/STM32_Control_Card/STM32_Control_Card.eprj3` (`83` components, `232` header/MCU pins audited) pass **live EasyEDA Pro Schematic ERC with `0` errors (`[]`)** and match `STM32_Control_Card_BOM.md` 1-for-1 (`$75.52` Qty 1 / `$69.20*` Qty 10).
- Boost Converter documentation package (`hardware/boost_converter/`) and project punchlist (`docs/Project_Punchlist.md`) were previously synced and pushed to `origin/main`.

### B. 4-Layer PCB Floorplan Saved Live to `PCB1.epcb2`
Updated via `C:\Projects\easyeda-ai-designer\projects\microgrid_stm32_control_card\step2_plan_and_place.py`:
- **Board Outline & Stackup:** `100.0 × 100.0 mm` (`r = 2.0 mm` corners, still within JLCPCB's `≤ 100 × 100 mm` promotional tier), 4-layer `4L_SIG_GND_PWR_SIG_3313` (`Top`, `Inner1_GND`, `Inner2_PWR`, `Bottom`), `4× M2.5` (`2.75 mm`) HAT mounting holes at `(18.0, 52.0)`, `(76.0, 52.0)`, `(18.0, 29.0)`, `(76.0, 29.0)`.
- **Verified Locked Mechanics:**
  - `J1` `(7.62, 33.02)` & `J2` `(86.36, 33.02)` (Bottom Layer 2, `rot=90°`): **`dX = +78.740 mm (+31.0000 × 2.54 mm), dY = 0.000 mm`**.
  - `J3` `(48.26, 5.08)` (Top Layer 1, `rot=0°`), `J4` `(47.00, 52.00)` (Top Layer 1, `rot=0°`), `U1` `(47.00, 32.50)` (Top Layer 1, `rot=270°` — AIN left, EXP bottom, PWM right, CAN/LCD top).
- **Hand-Solderable Spacing & Serviceability Improvements (Per User Feedback):**
  - Expanded from crowded `95 × 95 mm` (44 courtyard overlaps) to **`100 × 100 mm` with `0` pad/body overlaps** across all 83 components (verified live against exact `0805`/THT pad spans + physical bodies).
  - Moved **`SW1` + `C13` + `LED1`/`R18` + `LED2`/`R15`** to **Bottom-Left** (`X = 16.5..27.0, Y = 5.5..18.25`) and **`J5` (SWD) + `BOOT0` + `R1`** to **Bottom-Right** (`X = 73.5, Y = 5.5..19.25`) — **100% outside the Waveshare LCD HAT** for easy bench access and clear top spacing between the SMPS (`L2`/`C35`/`U7`) and the `5 kVrms` CAN barrier (`U2`/`U8`).
  - Dual crystal wells (`U5 + C31/C32` LSE, `U4 + C29/C30` HSE) spaced `≥ 1.5 mm` apart for comfortable hand-soldering while staying within `~7 mm` of `U1.8/9` & `U1.19/20`.
  - Keypad `0805` RC bank (`R6..R13`, `C14..C21`) spaced at `3.3 mm` uniform pitch (`1.7 mm` air gap) above `J4`.
  - `U2` (`10.90 mm` pad-to-pad) & `U8` (`9.50 mm` pad-to-pad) straddle the `5 kVrms` CAN moat (`X = 75.5..82.0 mm, Y ≥ 62.5 mm`).

### C. `easyeda-ai-designer` Tooling Improvements (Committed to `easyeda-ai-designer`)
- Added `projects/microgrid_stm32_control_card/` (`step1_open_and_inspect.py`, `step2_plan_and_place.py`, `step3_route_and_verify.py`, `pcb_design_brief.json`, `BOARD_PLAN.md`, `board_plan.json`, `pcb_pads.json`, `reports/schematic_erc.json`, `screenshots/01_pcb_placed.png`).
- Upgraded `src/pcb_router.py`:
  - Added `200 mil` (`5.08 mm`) spatial hash buckets (`_pad_buckets`, `_hole_buckets`, `_via_buckets`) to `is_via_safe()` for dramatically faster via validation on large 80+ component boards.
  - Tightened `_build_coarse_guide_mask()` solid-tile threshold (`min_occ >= tile_area // 2`) so coarse guides avoid dense via/component walls.

---

## 3. Current Routing Status & Known Issues to Fix Next Session

In `C:\Projects\easyeda-ai-designer\projects\microgrid_stm32_control_card\step3_route_and_verify.py`, the following stages already pass cleanly offline:
- **Dual Crystal Shield Wells:** `4/4` `CRYSTAL_OSC` nets routed on Layer 1 with **0 vias** + stitched `GND_CTRL` guard rings.
- **SMPS Hot Loop:** `BUCK_SW` routed `1/1` on Layer 1 with **0 vias** + `L2` inductor magnetic flux keepout.
- **SMD Plane Vias:** `51/51` (`100%`) `GND_CTRL` SMD vias into `Inner1_GND` (`L15`), `27/27` (`100%`) `3V3_DIG` SMD vias into `Inner2_PWR` (`L16`), and `7/10` `GND_CAN_ISO` vias (remaining 3 tie via Top `GND_CAN_ISO` pour).
- **Tier P1 Power & CAN Diff Pair (`+24V_IN`, `$4N2`, `24V_FUSED_REVPOL`, `5V_CTRL`, `CANH`, `CANL`):** `6/6` routed in `2.3 s`, `0` CAN moat intrusions.

**What still needs tuning before running `--fast` live in EasyEDA Pro:**
1. **Visual Check of Latest `01_pcb_placed.png` & Silkscreen Cleanup:**
   - Visually inspect the newly saved `screenshots/01_pcb_placed.png` (with `SW1`/`LEDs`/`J5`/`BOOT0` at bottom) and verify every designator (`U2`, `U8`, `U4`, `U5`, `R6..R13`, `C14..C21`, `C1..C12`) sits cleanly off pads and is easily legible for hand-soldering. Adjust designator positions via EasyEDA Pro API if any text overlaps a pad.
   - Confirm with user whether `100.0 × 100.0 mm` fits the target host power boards / enclosure envelope and confirm real Waveshare HAT caliper measurements (`58.0 × 23.0 mm` holes & `J4` offset).
2. **Finish `step3_route_and_verify.py` Solver Tuning (`Tier P2/P3/P4`):**
   - `Tier P3/P4` still had ~11–35 nets failing on Pass 1 because long cross-`U1` nets (e.g., `AIN_15` from `U1.70` on right to `J1.23` on left; `ENG_CAP2` from `U1.16` on left to `J2.29` on right) ran out of the `6500` corridor expansion budget inside `_build_coarse_guide_mask()` when detouring around `U1_BODY`.
   - **Recommended quick fix:**
     - In `src/pcb_router.py` (`route_two_points()`), give the coarse-guided A* pass a generous `max(self.max_expansions_corridor * 2, 16000)` budget (and run coarse guide *before* full-board search on spans `> 15 mm`, even on `grid_mm = 0.20`).
     - Re-run `uv run python projects/microgrid_stm32_control_card/step3_route_and_verify.py --dry-run` until **`failed = 0`, `xtal_vias = 0`, `CAN_moat_intrusions = 0`** in `< 60 s`.
3. **Live Emit, 4-Layer Pours, DRC Sign-Off & Visual Verification:**
   - Open `STM32_Control_Card.eprj3` via `step1_open_and_inspect.py` (refresh `.session.json` if EasyEDA Pro was closed).
   - Run `step3_route_and_verify.py --fast` live to emit all traces, tented Multi-Layer via pads (`padNumber='VIA'`), notched 4-layer pours (`Inner1_GND`, `Inner2_PWR` `3V3_DIG`/`5V_CAN_ISO`, `Bottom`, `Top` + `U7` `3V3_DIG` thermal island), rebuild copper, run live `JLCPCB 4L` DRC + `getRatlines()`, and capture `02_pcb_4layer_top.png` through `05_pcb_4layer_inner2_pwr.png`.
   - If any DRC or ratline items remain, add a surgical `step4_drc_polish.py` until live EasyEDA Pro reports **0 DRC errors and 0 unrouted ratlines**.
   - Visually inspect all 4 copper layer PNGs + Top silkscreen, verify 1:1 paper printout against physical Waveshare HAT & `SBH11`/`SFH11` headers, and update `STM32_Control_Card_Mechanical_and_Power.md` & `docs/Project_Punchlist.md`.

---

## 4. Key Files & Repositories

| Repository / Path | Purpose |
| --- | --- |
| `c:\Projects\Microgrid-Platform\hardware\STM32_control_card\Schematic\STM32_Control_Card\STM32_Control_Card.eprj3` | Live EasyEDA Pro project (4 ERC-clean sheets + updated `100×100 mm` 4L placed `pcb/PCB1.epcb2`) |
| `c:\Projects\Microgrid-Platform\hardware\STM32_control_card\STM32_Control_Card_Architecture.md` | System role, power partition, HAT & CAN requirements |
| `c:\Projects\Microgrid-Platform\hardware\STM32_control_card\STM32_Control_Card_Mechanical_and_Power.md` | Power tree, `5 kVrms` CAN isolation, headers, hand-soldering assembly order |
| `c:\Projects\Microgrid-Platform\hardware\STM32_control_card\STM32_Control_Card_BOM.md` | 83-part reconciled BOM with DigiKey pricing |
| `C:\Projects\easyeda-ai-designer\projects\microgrid_stm32_control_card\step2_plan_and_place.py` | Reproducible `100×100 mm` hand-solderable placement script |
| `C:\Projects\easyeda-ai-designer\projects\microgrid_stm32_control_card\step3_route_and_verify.py` | 4-layer router, notched CAN pours, live DRC & screenshot script |
| `C:\Projects\easyeda-ai-designer\projects\microgrid_stm32_control_card\screenshots\01_pcb_placed.png` | Latest live post-placement canvas capture |
| `C:\Projects\easyeda-ai-designer\src\pcb_router.py` | Core multi-layer router (spatial-hashed `is_via_safe()` + coarse-guide updates) |
