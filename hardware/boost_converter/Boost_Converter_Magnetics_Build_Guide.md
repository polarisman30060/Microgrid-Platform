# 10 kW 2-Phase FB-LLC Boost — DIY Transformer (`T1`/`T2`) & Resonant Inductor (`L_r`) Build Guide

Status: Rev 0.1 Workshop Construction Guide (2026-11-22) — Verified physical stackup for hand-winding the `45.0 kHz` `2 × 5 kW` Boost FB-LLC magnetics using standard **`26 AWG` heavy-build (`MW35-C` `200 °C`) magnet wire**, **`TDK U93/76/30` (`N87`)** transformer cores, and **`TDK E55/28/21` (`N87`)** resonant inductor cores.

---

## 1. Why This Winding Is Unusually Easy to Hand-Build

Using the giant **`TDK U93/76/30` (`U-U` core pair)** at **`45.0 kHz`** (`6 : 13 : 13` turns) hits a rare mechanical sweet spot:
- **Both Primary (`6 T`) AND Bifilar Secondary (`13 T + 13 T`) Fit in ONE SINGLE LAYER EACH** inside the `96 mm` tall window (leaving `8 mm` top and bottom insulation margins and **`25.4 mm` of open air space** across the `37 mm` window width!).
- **Zero Layer-to-Layer Proximity Penalty:** Single-layer windings + `26 AWG` strands (`r = 0.202 mm < 0.311 mm` skin depth at `45 kHz`) keep `R_ac/R_dc ≈ 1.15–1.20`.
- **Ice-Cold `99.6%` Transformer Efficiency (`~19.5 W` Total Loss at `5,000 W`!):**
  - Core loss (`106 mT` @ `45 kHz`): `~6.0 W`
  - Primary copper loss (`64.3 A RMS`, `1.26 mΩ` DC @ `100 °C`): `~6.3 W`
  - Secondary copper loss (`13.9 A RMS`/half, `15.5 mΩ` DC/half): `~7.2 W`
- **One `2 lb` (`~0.9 kg`) spool of `26 AWG` `200 °C` magnet wire (`MW35-C` / polyester-imide + polyamide-imide)** winds **BOTH** `5 kW` transformers (`~0.76 lb` each) AND **BOTH** `L_r` inductors!

---

## 2. Materials & Workshop Tools Needed (For `1×` `10 kW` Module = `2×` `T1/T2` + `2×` `L_r1/L_r2`)

| Item | Qty | Specification |
| :--- | :---: | :--- |
| **Main Transformer Cores (`T1`, `T2`)** | `4` halves (`2` pairs) | **TDK/Epcos `B67345B0001X087` (`U93/76/30`, `N87` U-half!)** — leg `28 × 30 mm`, U-U window `37 × 96 mm` *(Alt: Ferroxcube `U93/76/30-3C94`)* |
| **External `L_r` Cores (`L_r1`, `L_r2`)** | `4` halves (`2` pairs) | **TDK/Epcos `B66335G0000X187` (`E55/28/21`, `N87`, `A_e = 353 mm²`)** — will not saturate even at `> 250 A` fault spike! |
| **Magnet Wire** | `2 lb` spool (~`800 m`) | **`26 AWG` Heavy-Build `200 °C` (`NEMA MW35-C`)** magnet wire (solder-strippable or chemical/abrasive strip) |
| **Bobbin & Clamping Stock** | `1` sheet | **`3.0 mm` & `6.0 mm` `G10/FR4` fiberglass sheet** + `1.5 mm` FR4/Nomex gap shim stock |
| **Reinforced Insulation** | `1` roll each | **`0.5 mm` (`20 mil`) Nomex 410** paper + **`2 mil` (`50 µm`) Kapton tape** (`25 mm` & `50 mm` widths) + **`8 mm` Nomex margin tape** |
| **Secondary Lead Sleeving** | `2 m` | **`4 kV`-rated PTFE (Teflon) or silicone/fiberglass sleeving** (`4–5 mm` ID) for `SEC_A`, `CT`, `SEC_B` exits |
| **Terminations & Clamping** | Lot | M4/M6 heavy copper crimp ring lugs (`16 mm²` / `6 AWG` for Pri, `4 mm²` / `12 AWG` for Sec), **non-magnetic stainless M6 threaded rod** (outside core!), high-temp epoxy (`3M DP460` / `JB Weld`), `95 °C` `NC` thermal snap discs |
| **Test Tools** | — | Handheld **LCR meter** capable of `10 kHz` or `100 kHz` (e.g., `DE-5000` / `Hantek`), solder pot (or `400 °C` iron + flux) for tinning `26 AWG` bundles, cordless drill for rope twisting |

---

## 3. Step-by-Step Construction of Main Transformer (`T1` & `T2`)

### Step 1 — Build the `3.0 mm` G10 Bobbin Sleeve (Gap-Fringing Standoff!)
*Why `3.0 mm` thick?* In a `U-U` core pair, the **`1.52 mm` air gap** sits right at the vertical midpoint of the `96 mm` leg (`48 mm` up). Keeping the copper **`≥ 3.0 mm` radially away from the ferrite face** prevents gap fringing flux from inducing eddy-current hotspots in the inner primary turns (and still leaves `25.4 mm` of free space in the `37 mm` window!).
1. Cut four strips of **`3.0 mm` `G10/FR4`** to form a snug rectangular tube (`28.5 × 30.5 mm` ID, `34.5 × 36.5 mm` OD, **`94 mm` tall**) that slides cleanly over Leg 1 of the U-U core pair.
2. Glue two **`1.6 mm` `G10`** end flanges (`~62 × 62 mm`, centered on Leg 1) at the top and bottom of the tube with high-temp epoxy. Inside flange-to-flange height = `~91 mm`.
3. Wrap **`8 mm` wide Nomex/Kapton margin strips** tightly against both the top and bottom flanges, leaving an exact **`78–80 mm`** winding zone in the middle.

### Step 2 — Make & Wind the Single-Layer Primary (`6 Turns`, `120 × 26 AWG`)
1. Cut **`120` strands of `26 AWG`** to **`1.50 m` length** each (6 turns × `~0.15 m` MLT = `0.90 m` + `0.60 m` for twisting take-up and busbar exit leads).
2. Divide into **`4` equal ropes of `30` strands** each. Clamp one end of each 30-strand rope in a vise, chuck the other end in a cordless drill, and twist gently (**`~1 twist per 25–30 mm`**—just enough to hold round and transpose strands; do not over-twist!).
3. Lay all `4` ropes **flat side-by-side** (`~12.9 mm` wide × `~3.4 mm` thick) and spirally half-lap wrap them with **`1` layer of thin Kapton tape** to form one neat flat ribbon.
4. Leaving `150 mm` of tail on the **LEFT (Primary `96 V`) side**, wind **`6` tight, flat turns side-by-side** down the bobbin (`6 × 12.9 mm = 77.4 mm`, fitting cleanly inside the `80 mm` zone in **one single layer!**), bringing the finish tail out the **LEFT side** as well. Secure tightly with Kapton tape.

### Step 3 — Apply the Reinforced `≥ 4 kVrms` Isolation Barrier
1. Fill the `8 mm` top and bottom margin channels flush with the primary thickness using stacked `8 mm` Nomex/Kapton margin strips so the secondary cannot slip down into the margin zone.
2. Wrap **`1` full turn (with `15 mm` overlap) of `0.5 mm` (`20 mil`) Nomex 410 paper** spanning the **full `91 mm` flange-to-flange width**.
3. Overwrap **`3` full overlapping layers of `2 mil` Kapton tape** spanning the full `91 mm` width. *(Combined dielectric strength `> 15 kV`, physical creepage `> 12 mm`!)*

### Step 4 — Make & Wind the Single-Layer Bifilar Secondary (`13 T + 13 T`, `25 × 26 AWG` each)
1. Cut **`2` identical ropes of `25 × 26 AWG`**, each **`3.20 m` long** (`13` turns × `0.175 m` MLT = `2.28 m` + generous lead tails). Twist each 25-strand rope gently (`~1 twist per 25 mm`) and label them **`SEC1` (Red tape)** and **`SEC2` (Black tape)** so you never mix up phase dots!
2. Slip **`4 kV` PTFE / silicone-fiberglass sleeving** over both start tails (`SEC1_START`, `SEC2_START`) so the insulation begins well inside the Kapton wrap.
3. Bringing both start tails out the **RIGHT (Secondary `±200 V`) side**, wind **`SEC1` and `SEC2` side-by-side together (bifilar)** for **`13` neat turns** (`26` rope widths × `2.9 mm` ≈ `76 mm` — fits in **one single layer!**).
4. Bring both finish tails (`SEC1_END`, `SEC2_END`) out the **RIGHT side** inside `4 kV` sleeving.
5. **Center-Tap Wiring (Verify Polarity!):**
   - `SEC1_START` → **`SEC_A`** (to `D1`/`D3` SOT-227 bridge)
   - Join **`SEC1_END` + `SEC2_START`** together cleanly in ONE crimped M4 lug → **`CT` (`HV_MID = 0 V = N = PE`)**!
   - `SEC2_END` → **`SEC_B`** (to `D2`/`D4` SOT-227 bridge)
   - *(Sanity check: Because `SEC1` and `SEC2` are wound in the exact same direction, connecting `END` of `SEC1` to `START` of `SEC2` puts them series-aiding so `V(SEC_A – SEC_B) = 2 × V(SEC_A – CT) = 400 V`!)*
6. Overwrap the secondary with `2` layers of Kapton tape + `1` layer of Nomex, and clamp the `95 °C` `NC` thermal snap disc firmly onto the outer wrap.

### Step 5 — Tin, Lug & Clamp the `U93/76/30` Core Pair
1. **Stripping & Tinning `26 AWG` Bundles:** Dip each bundle end into a `400 °C` solder pot (for solder-strippable wire) or strip enamel cleanly, twist tightly, tin thoroughly so **every single strand** is electrically bonded (missed strands raise AC resistance!), and crimp + solder into heavy M4/M6 copper ring lugs.
2. Slide the wound bobbin onto Leg 1 of the two **`TDK U93/76/30`** halves.
3. Place **`1.52 mm`** of flat FR4/Nomex spacer stock across **BOTH mating legs** (Leg 1 and Leg 2) of the U-U core pair.
4. Clamp the U-U core externally using two **`6 mm` `G10/FR4`** clamping plates across the top and bottom yokes with **four stainless M6 threaded rods positioned OUTSIDE the ferrite core** (never pass a metal bolt through the center window!).

---

## 4. External Resonant Inductor (`L_r1`, `L_r2`) & "Measure-and-Shim" Calibration

Do **NOT** guess your resonant inductor gap before building `T1` and `T2`—use this exact 10-minute LCR calibration so hand-winding and busbar parasitics cancel out to `< 1.5%`:

1. **Measure `L_m` (Open-Circuit Inductance):** With all secondary leads **OPEN**, measure primary inductance of `T1` and `T2` on your LCR meter (`10 kHz` or `100 kHz`). Tweak the `~1.52 mm` U93 spacer thickness with thin `0.1 mm` Kapton/paper shims until **both `T1` and `T2` read `L_m = 12.50 µH ± 2%` (`12.25 – 12.75 µH`)**!
2. **Verify Polarity & Turns Ratio:** Apply `10 Vpp` (`45 kHz` sine or square wave from a signal generator) to Primary; confirm on a scope:
   - `V(SEC_A – CT) = 21.7 Vpp` (`13/6 × 10 V`)
   - `V(CT – SEC_B) = 21.7 Vpp` (same phase!)
   - `V(SEC_A – SEC_B) = 43.3 Vpp` (if it reads `0 V`, you wired `SEC1_END` to `SEC2_END` instead of `SEC2_START`—swap one secondary half!).
3. **Measure Built-In Loop Leakage (`L_lk + L_busbar`):** Bolt `T1` onto your primary busbar assembly (bypassing `C_r` temporarily with a short copper jumper), **dead-short `SEC_A`, `CT`, and `SEC_B` together** with a short clip, and measure inductance at the `Q1–Q4` bridge terminals. Expect **`~0.8 – 1.3 µH`**.
4. **Wind & Shim `L_r1` / `L_r2` (`TDK E55/28/21`, `N87`):**
   - Needed external inductance: `L_r,ext = 2.08 µH − L_measured_loop` (typically `~0.8 – 1.2 µH`).
   - Wind **`3 turns`** of the exact same `120 × 26 AWG` primary flat ribbon onto an `E55/28/21` bobbin (at `140 A` peak, `B_pk = 159 mT` — impossible to saturate!).
   - Place FR4/Nomex spacers across all 3 legs of the `E55` core (`~1.2–2.0 mm` shim thickness) while wired in series with `T1` (secondary still shorted!) and trim shim thickness until the **TOTAL loop inductance at the `Q1–Q4` bridge reads `2.08 µH ± 1.5%` (`2.05 – 2.11 µH`)**!
   - Repeat for Phase 2 (`T2` + `L_r2`) until **`L_r1 == L_r2`** within `±1.5%`.
5. **Hi-Pot / Megger Check Before Power-Up:** Short all primary leads together and short all secondary leads together; test insulation resistance with a `1,000 V` (or `2,500 V`) insulation tester (`Megger`) between Primary and Secondary, and between both windings and the ferrite core (`> 100 MΩ` required!).
