# Tiny GPS Logger — design record

Process: `power-pcb-design` skill (`~/.codex/skills/power-pcb-design`), adopted on 2026-11-05 after schematic drafting had started. Requirements and decisions are in [SPEC.md](SPEC.md).

## Gate log

| Step | Date | Decision | Notes |
|---|---|---|---|
| 1 Requirements | 2026-11-05 | Approved | Retroactive approval by the user |
| 2 Architecture and limits | 2026-11-05 | Approved | Retroactive approval by the user |
| 3 Circuit and models | 2026-11-05 | Approved | Hand calculations only, no simulation |
| 4 Schematic and footprints | 2026-11-05 | Approved | Open audit items stay open and must close before ordering |
| 5 Design rules and stack-up | 2026-11-05 | Approved | 1.6 mm stack accepted |
| 6 Placement | 2026-11-05 | Gate waived; revised at the user's request | User: "решай сам, я все одобряю" (decide yourself, I approve everything). The user then asked to move the microSD slot to the bottom and to spread the top side |
| 7–8 Routing, verification | 2026-11-05 | Gates waived on the same basis | Agent chose the routing mode; decisions and evidence are below |
| 9 Delivery | 2026-11-05 | Released locally at the user's request | Files are in `release/rev1`; prototype only. Not committed, shared or ordered. Ordering always needs explicit permission |
| 9 Delivery, repeated | 2026-11-07 | Released locally at the user's request | `release/rev1` exported again after the revision of 2026-11-07. Prototype only. Not committed, shared or ordered |
| Small revision: J3 and L1 clearance | 2026-11-07 | Approved by the user | User proposed moving R12 and D1 down, J3 down and rotating L1, then asked to move D3 away from J5 and to spread C16, C17, R13 and D3, then C5, C14 and C8 to the left. Details in "Revision of 2026-11-07" |
| Small revision: shared passives | 2026-11-09 | Approved by the user | Passives aligned with the user's other boards. The user chose to convert the values without a match as well, and to keep D3 as Hottech BAT54WS. Details in "Revision of 2026-11-09" |

Steps 1–4 were run before this process was adopted, so their gates were not held in order. They need retroactive approval.

## Tool inventory

| Tool | Runs on | Version | Note |
|---|---|---|---|
| EasyEDA Pro | User's computer | 3.2.209 | Driven live through the `easyeda-api` bridge, so there is no version mismatch |
| Circuit simulator | — | None selected | No simulation has been run |
| Thermal or field solver | — | None selected | |
| Impedance calculation | Agent workspace | Python script, analytical CPWG | JLC calculator not run |

## Architecture

There are no switching converters on this board. Power goes through a linear charger, a diode and P-FET power path, and an LDO, so the skill's commutation cells and gate loops do not apply. No insulation boundary exists: all nets are at most 5.5 V.

The relevant stress cases are the USB input, startup inrush, the 50 Ω RF line, and SD/USB edges coupling into GNSS.

## Electrical net table

Evidence: DS = data sheet, CALC = hand calculation. Nothing has been simulated or measured.

### Nodes

| Node | Ref | Working | Peak | Source | Constraint |
|---|---|---|---|---|---|
| VBUS | GND | 5 V | 5.5 V assumed | USB nominal; transients unresolved | ESD at the connector |
| VBAT, VBAT_SW | GND | 3.0–4.2 V | 4.2 V | Li-Po, MCP73831 (DS) | Must never connect to VBUS except through the charger |
| VSYS | GND | 3.0–5.0 V | 5.5 V assumed | CALC: VBUS minus diode drop | LDO VIN max 5.5 V (DS) |
| 3V3 | GND | 3.3 V | 3.6 V limit | MAX-M10S (DS) | Ramp 25–35000 µs/V at V_IO (DS) |
| V_BCKP | GND | ≈3.0 V | 3.3 V | Cell charge window (Seiko) | Below 3.6 V (DS) |
| RF_IN | GND | RF | +5.5 V DC abs. max. (DS) | | 50 Ω, ground on layer 2 |

### Branches

| Branch | Average | Peak | Evidence | Constraint |
|---|---|---|---|---|
| Battery → SW1 → Q1 → LDO → 3V3 | 15–20 mA | 200–250 mA | CALC; SD share assumed | SW1 300 mA, J1 1 A (library data); LP502030-PCM discharge rating not checked |
| VBUS → D4 → VSYS | 15–20 mA | 200–250 mA | CALC | D4 1 A (library data) |
| 3V3 → U1 | 12 mA | 100 mA inrush | DS | Under 0.2 Ω in series (u-blox) |
| 3V3 → J4 | 1–5 mA | About 100–200 mA, unverified | Assumed | C19, C20 at the slot |
| VBUS → U4 → VBAT | 50 mA | About 55 mA | CALC: 1000 V / 20 kΩ (DS) | 0.25C at 200 mAh |
| Fault | — | Unresolved | — | No fuse. Short on 3V3 is limited by the LDO, 610 mA minimum (DS). Short at VBAT depends on the battery's protection |

### Sensitive nets

| Net | Function | Aggressors | Constraint |
|---|---|---|---|
| RF_IN | GNSS input, −130 to −167 dBm | SD_SCK, USB, SPI | Short line; keep digital nets away |
| VBAT_SENSE | ADC | Switching digital nets | Filter at the MCU pin |
| V_BCKP | RTC and BBR supply | — | Low resistance to the cell (u-blox) |

## Step 4 audit

### ERC and netlist

- ERC on all three pages: 0 errors, 0 warnings. An earlier BT1 warning came from a renamed part and was fixed.
- Netlist compared with the design table: 58 parts, 0 mismatches, no single-pin nets.
- One ERC run reported 22 warnings during an earlier rebuild. They did not recur, and their cause is unknown.
- A netlist check confirms only the intended design. The VBUS-to-battery path found in review was intended wiring and passed it.

### Symbol to footprint

Pad numbers were compared with symbol pins for 20 part types, covering every IC, connector, diode, LED and the backup cell. All match. Passives of the same size share the footprints checked.

| Check against the manufacturer's data sheet | Status |
|---|---|
| U1 MAX-M10S pins | Checked, data sheet R08 |
| U3 RT9080 pins | Checked, DS9080-09 |
| U2 STM32L412 pins and alternate functions | Checked, DS12469 Rev 9 |
| U4 MCP73831 | Checked, Microchip data sheet |
| U5 USBLC6 | Checked, ST Doc ID 11265 Rev 5, SOT23-6 |
| Q1 AO3401A | Checked, AOS data sheet Rev 3.1: 1 gate, 2 source, 3 drain |
| D4 B5819WS | Checked, Yangjie S-S1680 Rev 1.3: the line marks the cathode; footprint band at pad 1, wired to VSYS. Order code changed from C179554, which LCSC no longer lists, to C699105 |
| SW1 common pin | Checked, Alps SSSS811101: terminal 2 is common and the asymmetric pads are correct. Terminals 1 and 2 are wired, so ON should be the actuator toward terminal 1; confirm with a meter |
| J4 CD pin behaviour | **Unresolved:** the drawing shows a Cd pad but not its switching sense. Read it on the first board; SD_DET is an input with a pull-up, so firmware can adapt |
| BT1 tab pads | Checked, Seiko MS621FE FL11E drawing: tabs 0.5 mm wide, 1.5 mm apart; plus tab on pad 1 |
| D1, D2, D3 polarity | Checked: KENTO and Hottech data sheets, library pin names and footprint marks agree with the nets |
| Land patterns, drills, pad sizes | See "Land patterns against manufacturer drawings" in step 8 |

### 3D and mechanical

Not done: no 3D model, envelope or height check.

### Component links

Checked 2026-11-05; rows changed by the passives revision of 2026-11-09 are marked "not opened". An LCSC page returning HTTP 200 does not prove that its content matches the part. Manufacturer product pages are unresolved.

| Refs | Part | Order page | Order page check | Data sheet | Product page |
|---|---|---|---|---|---|
| BT1 | MS621FE-FL11E | [C80909](https://www.lcsc.com/product-detail/C80909.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/MS621FE-FL11E/82059.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| C1, C4, C16, C12 | CL10A105KB8NNNC | [C15849](https://www.lcsc.com/product-detail/C15849.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/CL10A105KB8NNNC/16531.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| C3 | CL21A476MQYNNNE | [C16780](https://www.lcsc.com/product-detail/C16780.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/CL21A476MQYNNNE/17464.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| C5, C15, C17, C8, C9, C10, C13, C14, C20 | CL10B104KB8NNNC | [C1591](https://www.lcsc.com/product-detail/C1591.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/CL10B104KB8NNNC/1943.html) — unchecked | unresolved |
| C6, C7, C2, C11 | CL10A475KO8NNNC | [C19666](https://www.lcsc.com/product-detail/C19666.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/CL10A475KO8NNNC/20375.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| C18 | CL10B103KB8NNNC | [C1589](https://www.lcsc.com/product-detail/C1589.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/CL10B103KB8NNNC/1941.html) — unchecked | unresolved |
| C19 | CL10A106KP8NNNC | [C19702](https://www.lcsc.com/product-detail/C19702.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/CL10A106KP8NNNC/20411.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| D1 | KT-0603G | [C12624](https://www.lcsc.com/product-detail/C12624.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/KT-0603G/13258.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| D2 | KT-0603R | [C2286](https://www.lcsc.com/product-detail/C2286.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/KT-0603R/2659.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| D3 | BAT54WS | [C191198](https://www.lcsc.com/product-detail/C191198.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/BAT54WS/214375.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| J1 | BX-SH1.0-2PWT | [C18077908](https://www.lcsc.com/product-detail/C18077908.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/BX-SH1.0-2PWT/19218309.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| J2 | TYPE-C-31-M-12 | [C165948](https://www.lcsc.com/product-detail/C165948.html) | opens (HTTP 200); content not compared | [drawing](https://www.lcsc.com/datasheet/C165948.pdf) — land pattern compared, see step 8 | unresolved |
| J3 | U.FL-R-SMT-1(80) | [C88374](https://www.lcsc.com/product-detail/C88374.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/U.FL-R-SMT-1(80)/89553.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| J4 | TF PUSH | [C393941](https://www.lcsc.com/product-detail/C393941.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/TF%2520PUSH/372553.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| J5 | MTP315-1107S1 | [C5142369](https://www.lcsc.com/product-detail/C5142369.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/MTP315-1107S1/5758673.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| L1 | LQG15HS27NJ02D | [C12669](https://www.lcsc.com/product-detail/C12669.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/LQG15HS27NJ02D/13305.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| R2, R3 | RC0603FR-07220KL | [C123420](https://www.lcsc.com/product-detail/C123420.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://atta.szlcsc.com/upload/public/pdf/source/20180821/C275644_0D56F95EFF785E7D9A1FEF436B37DA78.pdf) — unchecked | unresolved |
| R4, R5 | RC0603FR-075K1L | [C105580](https://www.lcsc.com/product-detail/C105580.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-075K1L/106795.html) — unchecked | unresolved |
| R8, R9, R10 | RC0603FR-0710KL | [C98220](https://www.lcsc.com/product-detail/C98220.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/RC0603FR-0710KL/99429.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| R7, R13, R12 | RC0603FR-071KL | [C22548](https://www.lcsc.com/product-detail/C22548.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-071KL/23274.html) — unchecked | unresolved |
| R1 | RC0603FR-07100KL | [C14675](https://www.lcsc.com/product-detail/C14675.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-07100KL/15343.html) — unchecked | unresolved |
| R6 | RC0603FR-0720KL | [C105575](https://www.lcsc.com/product-detail/C105575.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-0720KL/106790.html) — unchecked | unresolved |
| R11 | RC0603FR-0733RL | [C108661](https://www.lcsc.com/product-detail/C108661.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-0733RL/109880.html) — unchecked | unresolved |
| R14 | RC0603FR-0710RL | [C109318](https://www.lcsc.com/product-detail/C109318.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-0710RL/110539.html) — unchecked | unresolved |
| R15, R16, R17, R18, R19, R20 | RC0603FR-0747KL | [C105579](https://www.lcsc.com/product-detail/C105579.html) | not opened; MPN, package and footprint read from the EasyEDA library on 2026-11-09 | [link](https://item.szlcsc.com/datasheet/RC0603FR-0747KL/106794.html) — unchecked | unresolved |
| SW1 | SSSS811101 | [C109335](https://www.lcsc.com/product-detail/C109335.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/SSSS811101/110556.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| U1 | MAX-M10S-00B | [C4153167](https://www.lcsc.com/product-detail/C4153167.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/MAX-M10S-00B/4722633.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| U2 | STM32L412KBU6 | [C529439](https://www.lcsc.com/product-detail/C529439.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/STM32L412KBU6/549667.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| U3 | RT9080-33GJ5 | [C841192](https://www.lcsc.com/product-detail/C841192.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/RT9080-33GJ5/897967.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| U4 | MCP73831T-2ACI/OT | [C424093](https://www.lcsc.com/product-detail/C424093.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/MCP73831T-2ACI%252FOT/417116.html) — blocked for scripts (HTTP 405), unchecked | unresolved |
| U5 | USBLC6-2SC6 | [C7519](https://www.lcsc.com/product-detail/C7519.html) | opens (HTTP 200); content not compared | [link](https://item.szlcsc.com/datasheet/USBLC6-2SC6/7996.html) — blocked for scripts (HTTP 405), unchecked | unresolved |

## Programming

| Method | Connection | Needs | Status |
|---|---|---|---|
| SWD (primary) | ST-Link to J5: pin 1 3V3 (target voltage sense), 2 SWDIO, 3 SWCLK, 4 NRST, 5 GND | USB or battery power | Pins checked, DS12469 |
| USB DFU (secondary) | USB-C. A blank chip enters the bootloader by itself; later, hold BOOT0 (J5 pin 6) at 3V3 during reset | USB power is enough | Checked against AN2606 Rev 71, see "System bootloader" in step 8. PA10 is floating, so DFU may need a second reset |

## Step 5: design rules and stack-up

### Stack-up

Proposed: JLCPCB JLC04161H-3313, 1.6 mm, 4 layers. Values are from the JLC impedance page, checked 2026-11-05.

| Layer | Use | Copper | Dielectric below |
|---|---|---|---|
| L1 Top | Components, RF, signals | 35 µm | 3313 prepreg, 0.0994 mm, εr 4.1 |
| L2 | Solid GND | 15.2 µm | Core, 1.265 mm, εr 4.6 |
| L3 | 3V3 and power, low-speed signals | 15.2 µm | 3313 prepreg, 0.0994 mm |
| L4 Bottom | Signals, GND pour | 35 µm | — |

The core thickness is taken from the standard 1.6 mm stack and needs checking against the JLC calculator. A 1.0 mm board is an alternative, but JLC's page does not publish that stack.

### Rules

Implemented in EasyEDA on PCB1. The configuration is also saved as `TinyGPS_4L`.

| Item | Value | Basis | Type |
|---|---|---|---|
| Insulation | No safety spacing | All nets ≤ 5.5 V; functional insulation only. No product standard selected | Decision |
| Copper clearance, default | 0.15 mm | Above JLC 0.09 mm, margin for hand soldering | Heuristic |
| Via to track | 0.20 mm | JLC capabilities page | Fabricator limit |
| Copper to board outline | 0.30 mm | JLC ≥ 0.2 mm plus margin | Fabricator limit |
| Default track | 0.15 mm (changed from 0.20 mm in step 7 to escape the QFN) | JLC minimum 0.09 mm, margin | Heuristic |
| POWER track | 0.40 mm default, min 0.30 mm; routed at 0.35 mm | Conductor table below; 0.35 mm meets the 0.34 mm inner-layer estimate | Calculated |
| RF track (RF_IN, ANT_BIAS, VCC_RF) | 0.20 mm, range 0.19–0.21 mm | CPWG calculation below | Calculated |
| RF clearance | 0.20 mm | CPWG gap | Calculated |
| Via | 0.30 mm drill, 0.60 mm pad | JLC annular ring 0.15 mm minimum | Fabricator limit |
| USB pairs | Default track; pair objects USB_MCU and USB_CONN; length tolerance 5 mm (changed from 0.254 mm in step 8) | USB FS, 12 Mbit/s: 5 mm is about 33 ps against edges of several ns; impedance not controlled | Heuristic |

### 50 Ω line

Conductor-backed coplanar formula, zero-thickness approximation, no solder mask. Evidence: calculation only.

| Gap | Width for 50 Ω | Effective εr | ±0.02 mm width |
|---|---|---|---|
| 0.15 mm | 0.200 mm | 3.09 | 53.3 / 47.1 Ω |
| 0.20 mm | 0.206 mm | 3.16 | 53.2 / 47.2 Ω |
| 0.25 mm | 0.210 mm | 3.21 | 53.2 / 47.2 Ω |

Chosen: 0.20 mm width and 0.20 mm gap. Confirm in the JLC impedance calculator before ordering, because copper thickness and solder mask shift the result. Controlled impedance is not ordered by default.

### Conductors

IPC-2221 is used as a preliminary estimate, with a 10 °C rise. Design current is 0.3 A: the 250 mA peak with 20% margin. Resistance is calculated for 35 µm copper at 25 °C.

| Branch | Layer | Minimum by estimate | Chosen | 30 mm drop at 0.25 A |
|---|---|---|---|---|
| POWER nets | Outer | 0.06 mm | 0.40 mm | 9 mV, 37 mΩ |
| POWER nets | Inner, 15.2 µm | 0.34 mm | 0.40 mm minimum; check final width | Not calculated |
| Charge path, 55 mA | Outer | Under 0.01 mm | 0.40 mm (POWER class) | — |

The 37 mΩ result satisfies the 0.2 Ω u-blox series limit. Check VCC and V_IO on the final copper, including vias.

### Traceability

| Constraint | Implementation | Verification |
|---|---|---|
| RF_IN 50 Ω, ground on L2 | RF track and clearance rules; L2 solid | JLC calculator; L2 inspection |
| Ground under U1 on L1 and L2, no signals | Placement and routing note; no CAD keep-out until U1 is placed | DRC, inspection |
| Under 0.2 Ω to U1 VCC/V_IO | POWER rule; no series parts | Calculation on final copper |
| Keep SD_SCK and USB away from RF | Placement note | Inspection |
| Filter at VBAT_SENSE pin | Placement note: C5 at U2 PA0 | Inspection |
| Low resistance V_BCKP | Placement note: BT1 and C17 at U1 | Inspection |
| Polarity of J1 | Silkscreen note | Multimeter before connecting |

### DRC state at the end of step 5

Two clearance errors were unplaced parts lying on top of each other (U1 over R12). 190 connection errors were unrouted nets. Both were expected before placement and routing. See step 8 for the current state.

### Not done in this step

- The physical stack-up could not be read through the API (it returned null), so JLC04161H-3313 is a design assumption until selected when ordering.
- Keep-out regions need placed parts and a board outline.

## Step 6: placement

Board: 46 × 28 mm, corners 1 mm radius (a 2 mm radius clipped SW1 pad 7). 42 × 26 mm was rejected at 90% footprint density.

| Decision | Reason | Evidence |
|---|---|---|
| J4 moved to the bottom side at the same position, rotation 90 | User request. It cleared the top side and let the crowded parts near U2 move away | Pad positions read back from CAD; nets unchanged |
| Top parts spread with a 0.7 mm body gap and a 1.3 mm ring around U2 | User request; R2 stood 0.5 mm from the U2 pads and sealed pin 15 | Annealing plus hand edits; 0 footprint overlaps against the 0.15 mm check, except the L1 row in the open items |
| Decoupling held within 3.5–5 mm of its pin | Spreading alone pushed C10 to 12.5 mm | C15 2.0, C8 2.8, C9 3.3, C10 3.3, C16 4.4, C12 4.7, C13 4.9 mm |
| J5 moved from (24.5, 14) to (33.8, 12.1) | Its 1.27 mm pitch through-hole row was a wall across all layers between USB and U2 | Routing converged only after this move |
| R11 placed near J4, MCU-side pad facing U2 | The first placement pointed that pad into the area with no signal vias | Router log |
| R15, R17 and R2 moved to the free top-left area, R3 moved to the old R2 position | Designators were landing on neighbouring parts. The area was 91% free; the band beside U2 was 42% free. VBAT_SW, the SD pull-up stubs and the 3V3 branch were reworked locally; one GND via at (16.9, 26.65) was removed | DRC, pad-net check and resistance repeated, see step 8 |
| BT1 moved 0.6 mm toward the board edge, to (39.5, 9.4) | Its tab outline stood 0.05 mm from the J5 outline, and the tab could reach the J5 housing at worst-case tolerance. The tracks still land on the longer tab pads, so no rerouting was needed | Silkscreen gap now about 0.4 mm; DRC and pad-net check repeated |
| R18, R19, R20 stay beside J4 | With no signal vias allowed under the J4 shell, the top-left area is reachable only on Top from above. LDO_EN, VSYS and 3V3 block the way, so moving these parts would add vias and crossings | Track map of the area |

R19 and R15 swapped places, and R20 moved, because the slot pin order is mirrored on the bottom side.

## Step 7: routing

Mode: custom rip-up router written for this board, 0.05 mm grid, layers Top, L3 and L4. The EasyEDA autorouter was tried first and dropped: it left top tracks under U1, power tracks at 0.15–0.2 mm and a wall along the U2 pins. Its copper was removed.

Layer use: Top 392 mm, L3 126 mm, L4 105 mm. L2 carries no tracks.

Ground: 45 vias, of which 4 are in the U2 pad. GND pours on all four layers (GND_TOP, GND_L2, GND_L3, GND_BOT).

| Named exception | Size | Reason |
|---|---|---|
| 3V3 at 0.15 mm | 1.0 mm in total | Fine-pitch and via clearance; DRC-flagged tracks were narrowed |
| VBUS at 0.15 mm | 0.9 mm in total | Same, near R4 and a ground via |
| 3V3 passes through J5 pin 1 | Top to L3 | Plated hole, present with or without the header fitted |
| Seven signal vias under the J4 outline | x = 13.6–13.7 mm, plus LDO_EN at (8.3, 10.95) | Tented; check against the slot underside before ordering |
| No stitching vias along RF_IN | 2.7 mm line since the revision of 2026-11-07 (was 1.8 mm) | Far shorter than λ/20 (about 9.5 mm); no room within pad-to-via rules. Nearest ground vias are 1.1 and 1.3 mm away |

API note: a track created through the API links only to a via that already exists. Create vias first, or DRC reports those nets as disconnected.

## Step 8: verification

All values were read back from the EasyEDA document, not from the router model.

| Check | Result | Limit | Evidence |
|---|---|---|---|
| DRC | 0 errors, 0 unrouted | 0 | EasyEDA DRC, TinyGPS_4L rules, after the pour rebuild |
| Pad nets against the design table | 212 pads, 0 mismatches | 0 | Script |
| Minimum track | 0.15 mm | 0.15 mm | Script |
| RF_IN, ANT_BIAS, VCC_RF width | 0.20 mm | 0.19–0.21 mm | Script |
| Top routing under U1 | None | None | Script |
| Vias under U1 | None | None | SAFEBOOT_N via at (40.6, 26.6) is beyond the pad row |
| 3V3, C3 to U1 pins 7 and 8 | 78–80 mΩ | 200 mΩ (u-blox) | Calculated from final copper; 35 µm outer, 15.2 µm inner, 2.5 mΩ per via |
| 3V3, C3 to U2 pins 17, 1, 5 | 35, 55, 51 mΩ | — | Same |
| V_BCKP, BT1 to U1 pin 6 | 92 mΩ | Low (u-blox) | Same |
| USB_DP / USB_DN | 7.4 / 8.4 mm | 5 mm mismatch | Script |
| USB_CONN_DP / USB_CONN_DN | 4.6 / 8.9 mm | 5 mm mismatch | Script |

Not verified: 3D fit, assembled heights, signal integrity by simulation or measurement, and the JLC impedance calculation.

Silkscreen: 51 designators are 1.0 mm high and 6 are 0.8 mm (C14, C16, C17, R5, R13, R16), all with a 0.15 mm stroke. The 0.8 mm ones are under the JLC recommendation because the space there is too tight, and may print less cleanly. Label boxes read back from EasyEDA show no overlap with footprint outlines, pads, vias or other labels. The J4 designator is on the bottom side and was not moved. The outlines come from the footprint silkscreen bounding boxes, so a label can still sit close to a part.

### J2 land pattern against the manufacturer drawing

Source: [HRO TYPE-C-31-M-12 drawing, C165948](https://www.lcsc.com/datasheet/C165948.pdf), dated 2020-12-08. Board values come from the footprint and the generated drill and mask files.

| Feature | Drawing | Board | Result |
|---|---|---|---|
| Signal pads | 0.30 mm wide, ±1.25, ±1.75 and 0.5 mm pitch | Same | Match |
| Power and ground pads | 0.60 mm wide, centres ±2.40, ±3.20 | Same | Match |
| Pad length | 1.14 mm | 1.30 mm, ending 0.28 mm further out | Differs; the 0.4 mm exposed tail lands fully on the pad, and the longer toe helps hand soldering |
| Pegs | 2 × Ø0.60 mm at ±2.89 mm, for Ø0.50 mm pegs | 2 × Ø0.60 mm at ±2.90 mm, NPTH | Match |
| Slot centres | ±4.325 mm, 4.18 mm apart, 0.50 mm behind the pegs | Same | Match |
| Rear slot, for the 1.10 mm tab | 0.6 × 1.7 mm, pad 0.9 × 2.0 mm | 0.8 × 1.5 mm, pad 1.2 × 2.0 mm | Differs; 0.40 mm length clearance instead of 0.60 mm |
| Front slot, for the 0.80 mm tab | 0.6 × 1.4 mm, pad 0.9 × 1.7 mm | 0.8 × 1.2 mm, pad 1.2 × 1.8 mm | Differs; 0.40 mm length clearance instead of 0.60 mm |
| Peg line to board edge | 5.79 mm | 5.81 mm | Match |
| Pin order, component side | A1/B12 to B1/A12 | Same | Match |

Decision: keep the footprint. With drawing tolerance of ±0.10 mm on the tabs and an assumed ±0.15 mm on plated slot length (to be confirmed with the fabricator), the tabs still fit. The drawing does not give tab thickness, so the fit across the 0.8 mm slot width was not calculated; it is wider than the recommended 0.6 mm.

The tabs extend 1.01 mm below the housing and the board is 1.6 mm thick, so they stop about 0.6 mm inside the slot. Nothing shows on the bottom side, and the slots must be soldered from there with enough solder to fill the gap.

The bottom 3D view shows the slots as solid copper ovals, although the plated drill file holds all four. The tabs cannot explain this if the 3D model follows the drawing, so the view is treated as a rendering matter; this was not confirmed.

### Land patterns against manufacturer drawings

Compared on 2026-11-05 with the drawings named below. Passives other than L1 were not compared.

| Part | Source | Result |
|---|---|---|
| U2 | DS12469 Rev 7, figure 54 | Pitch, centre pad and pin order match. Pads end 0.25 mm further out than recommended, which helps hand soldering |
| U4 | DS20001984G, C04-2091A | Pitch matches. Contact spacing 2.31 mm against 2.80 mm, inner gap 1.15 mm against 1.70 mm minimum. Foot still lands on the pad; accepted |
| U5 | Doc ID 11265 Rev 5, figure 22 | Pitch and 2.30 mm spacing match; pads slightly smaller, 0.53 × 1.07 against 0.60 × 1.20 mm |
| U3 | DS9080-05 | Pitch and foot position fit; the document gives no land pattern |
| U1 | UBX-20053088 R05, figure 30 and table 44 | Pitch 1.1 mm and 1.8 mm pad length match. Pads run 0.05 mm further out. All pads are 0.7 mm wide; the manual gives 0.8 mm for the middle pads and 0.7 mm for the corners. Accepted. Via GPS_RESET at (31.45, 16.1) sits inside the 11.1 × 10.1 mm keep-out but outside the module body; accepted |
| J1 | Bossie BX-SH1.0 drawing | Match |
| J2 | HRO drawing | See the section above |
| J3 | Hirose U.FL catalogue | Pads match. The drawing forbids conductive traces between the GND pads, so a Top no-pour region J3_NO_POUR now clears that area |
| J4 | Shouhan TF PUSH drawing, 2020-07-07 | Pads, pin order and hole positions match. Holes are Ø1.2 mm against Ø1.0 mm for Ø0.8 mm pegs, so the part has more play; accepted |
| J5 | Mintron MTP315-11XXS1 | Match, Ø0.7 mm at 1.27 mm pitch |
| SW1 | Alps SSSS811101 | Holes, ground pads and pad spacing match. Terminal pads sit 0.25 mm further out but still cover the terminals; accepted |
| BT1 | Seiko MS621FE FL11E | Match. Height is 2.8 mm maximum |
| Q1 | AOS AO3401A | Pin order matches the SOT-23 pattern; the document gives no land pattern |
| D1, D2 | KENTO KT-0603 | 1.5 mm pad spacing against 1.4 mm; accepted |
| D3 | Hottech BAT54WS | 2.28 mm pad spacing against 2.15 mm; accepted |
| D4 | Yangjie S-S1680 Rev 1.3 | C699105 uses the same library footprint and 3D model as the old code; Vf 0.60 V at 1 A, also the same. The document's land pattern was not compared |
| L1 | Murata JELF243B-0010P-01 | Gap 0.30 mm against 0.4 mm, overall 1.88 mm against 1.4–1.5 mm. Solderable; check for tombstoning |

### System bootloader

Source: AN2606 Rev 71, November 2026, section 76 and table 2, the ST file supplied by the user. The STM32L412 section was compared line by line with Rev 62: only wording changed (bootloader V13.1, pattern 16, pins and clocks are the same). Rev 71 also asks to keep USB D+ and D− at the same level during detection when USB is unused; that does not affect DFU, where a host is attached.

| Requirement | Board | Result |
|---|---|---|
| Entry: BOOT0 pin high, or BOOT0 low with empty flash (pattern 16, default option bytes) | BOOT0 on PH3 with 10 kΩ pull-down, J5 pin 6 | Met |
| USB on PA11 and PA12, no external pull-up | Pins 21 and 22, no pull-up | Met |
| Clock | HSI48 with CRS, no crystal needed | Met |
| VDDUSB at 3.3 V | All supply pins on 3V3 | Met |
| Voltage scaling range 1 for DFU | Reset default; the bootloader does not change it | Met |
| RX pins of unused interfaces at a known level | PA10 (USART1_RX) is floating and the bootloader leaves it without a pull | **Not met** |
| | PA3 (USART2_RX): internal pull-up. PA7 (SPI1_MOSI): R17 | Met |
| I2C1 on PB6/PB7, shared with the GPS UART | Needs a clock on PB6; GPS_RXD is an MCU output in normal use and nothing clocks it here. Not tested | Accepted |
| SPI1 on PA4–PA7, shared with the card | Needs a clock on PA5; the bootloader pulls it down and the card does not drive it. Not tested | Accepted |

PA10 decision pending. A spurious 0x7F on the floating pin could select USART1 and hide DFU until the next reset. SWD on J5 is not affected. A pull-up cannot be added simply: the GPS_TP via at (24.05, 17.55) sits directly below pin 20.

### Peak current

All values are calculated, not measured. Cell and protection resistance is assumed at 0.3 Ω because no LP502030-PCM data was available. SD peak is assumed.

| Item | Value | Limit | Result |
|---|---|---|---|
| Operating peak, GPS plus SD start | 200–250 mA | SW1 300 mA, RT9080 610 mA minimum, J1 1 A | Met; SW1 margin is 17% |
| Board path, J1 + SW1 + Q1 + copper | 202 mΩ, 50 mV at 250 mA | — | J1 30, SW1 70, Q1 85 (at 2.5 V gate drive), copper 17 mΩ |
| RT9080 dropout at 250 mA | About 130 mV | — | Scaled linearly from 0.31 V at 600 mA |
| Battery for regulated 3.3 V at peak | 3.56 V open circuit | — | Below this, 3V3 follows the battery |
| Lowest battery at 250 mA peak | About 2.96 V open circuit, for 3V3 at 2.7 V | GPS and card minimum, 2.7 V | Met down to a nearly empty cell; set the firmware cut-off well above this |
| LDO start, no soft-start | Up to 1.1 A for about 200 µs into 65 µF nominal | SW1 300 mA | **Above the switch rating, briefly.** The contact is already closed; accepted, not tested |
| V_IO ramp at that current | 59 µs/V nominal, about 36 µs/V at 40 µF | At least 25 µs/V | Met |
| USB input, load plus charger | About 305 mA | 500 mA, default USB | Met |
| D4 | 0.6 V at 1 A, 250 mA peak | 1 A | Met |
| RT9080 from USB | About 0.3 W at peak, under 0.05 W average | — | Met; not a thermal limit |
| U4 charger | 0.1 W at 5 V in, 3.0 V battery, 50 mA | — | Met |

The real SD peak, the LP502030-PCM discharge limit and its protection trip current are still unverified.

### Revision of 2026-11-07

J3 and L1 stood too close: the L1 outline overlapped the J3 silkscreen and the copper gap was 0.44 mm. D3 overlapped the J5 outline by 0.05 mm. The outlines of C16 and C17 overlapped by 0.13 mm, C17 and R13 stood 0.02 mm apart, and C16 overlapped C15 by 0.15 mm. C5, C14 and C8 overlapped each other by 0.03 mm.

| Item | Before | After |
|---|---|---|
| J3 | (42.9, 18.2) | (42.9, 17.2), the 1.0 mm limit is the 0.20 mm RF clearance to the GND via at (41.185, 16.289) |
| L1 | (41.85, 20.5), vertical | (42.7, 20.1), rotation 180, horizontal |
| R12 | (42.5, 13.9) | (42.5, 12.4) |
| D1 | (44.6, 12.4) | (44.6, 10.9) |
| C16 | (31.0, 15.0) | (31.3, 15.0) |
| C17 | (33.9, 15.0) | (34.5, 14.95) |
| R13 | (36.9, 14.7) | (37.65, 14.7) |
| D3 | (39.1, 14.2) | (40.0, 14.2) |
| LED_STATUS via | (39.9, 14.65) | (41.1, 14.55), with a 1.2 mm Bottom extension |
| V_BCKP via | (35.35, 15.55) | (35.175, 15.725), on the same L3 diagonal; 0.2 mm below the 3V3 track and clear of the C17 pad by 0.025 mm |
| GND via at C16 | (30.7, 15.7) | (30.3, 15.75) |
| C5 | (20.3, 26.2) | (19.8, 26.2), 0.5 mm further from the U2 PA0 pin |
| C14 | (23.3, 26.2) | (23.05, 26.2) |
| GND via at C5 | (20.0, 26.9) | (19.5, 26.9) |
| GND vias at J3 and D1 | (44.25, 15.6), (44.79, 20.63), (45.35, 11.85) | (44.25, 14.6), (44.79, 19.63), (45.35, 10.35) |
| J3_NO_POUR | y 17.2–19.2 mm | y 16.2–18.2 mm |
| RF_IN, U1 to J3, along the track | 1.84 mm | 2.67 mm, with a bend at the L1 branch |
| RF_IN branch to L1 pad centre | 1.75 mm | 2.02 mm |

Results, read from CAD:

| Check | Before | After |
|---|---|---|
| L1 to J3, copper | 0.44 mm | 0.63 mm |
| L1 to J3, footprint outline | Overlap | 0.25 mm |
| L1 to R14, footprint outline | About 0.2 mm | 1.0 mm |
| D3 to J5, footprint outline | Overlap 0.05 mm | 0.85 mm |
| C15 to C16, footprint outline | Overlap 0.15 mm | 0.15 mm |
| C16 to C17, footprint outline | Overlap 0.13 mm | 0.17 mm |
| C17 to R13, footprint outline | 0.02 mm | 0.17 mm |
| R13 to D3, footprint outline | 0.43 mm | 0.18 mm |
| C5 to C14, footprint outline | Overlap 0.03 mm | 0.22 mm |
| C14 to C8, footprint outline | Overlap 0.03 mm | 0.22 mm |

DRC after rebuilding all four GND pours: 0 errors. Pad nets of J3, L1, R12, D1, D3, C16, C17, R13, C5 and C14 read back unchanged; 58 parts and 93 vias. The 3V3, V_BCKP, BCKP_CHG, LED_STATUS, VBAT_SENSE and NRST tracks around these parts were reworked locally. The resistance and decoupling distance figures in step 8 were not recalculated. The designators of J3 and L1 were moved so they do not land on pads. The RF effect is an estimate only: the longer line and branch are short against the wavelength, and nothing was measured or simulated.

Check before the export of 2026-11-07, after the user moved designators by hand: DRC 0 errors after rebuilding all four pours; the placement file differs from the previous export only for J3, L1, R12, D1, D3, C16, C17, R13, C5 and C14; no designator box lies on a pad or outside the board. The BOM file is byte-identical to the previous export.

Moving a via through the API rounded its drill from 0.30 mm to 11.8 mil (0.29972 mm), which added a drill tool. The seven moved vias were set back to 0.30 mm. The via drill file still lists two tools, both printed as 0.30000 mm.

### Revision of 2026-11-09: shared passives

Goal: the same value and package on this board and on the user's other boards uses the same part, so one order covers them all. Reference BOMs: `serber/marauder-c5-pancake-board` rev1, `serber/solar-power-module-CN3722` rev2 and `serber/lmp91000-afe` v1.0.

| Refs | Value | Old part | New part | LCSC | Source |
|---|---|---|---|---|---|
| C5, C8, C9, C10, C13, C14, C15, C17, C20 | 100 nF X7R 50 V | Yageo CC0603KRX7R9BB104 | Samsung CL10B104KB8NNNC | C1591 | marauder, lmp91000-afe |
| R1 | 100 kΩ 1 % | UNI-ROYAL 0603WAF1003T5E | Yageo RC0603FR-07100KL | C14675 | marauder |
| R4, R5 | 5.1 kΩ 1 % | UNI-ROYAL 0603WAF5101T5E | Yageo RC0603FR-075K1L | C105580 | marauder |
| R7, R12, R13 | 1 kΩ 1 % | UNI-ROYAL 0603WAF1001T5E | Yageo RC0603FR-071KL | C22548 | marauder, lmp91000-afe |
| R2, R3 | 220 kΩ 1 % | UNI-ROYAL 0603WAF2203T5E | Yageo RC0603FR-07220KL | C123420 | same series, no match |
| R6 | 20 kΩ 1 % | UNI-ROYAL 0603WAF2002T5E | Yageo RC0603FR-0720KL | C105575 | same series, no match |
| R11 | 33 Ω 1 % | UNI-ROYAL 0603WAF330JT5E | Yageo RC0603FR-0733RL | C108661 | same series, no match |
| R15–R20 | 47 kΩ 1 % | UNI-ROYAL 0603WAF4702T5E | Yageo RC0603FR-0747KL | C105579 | same series, no match |
| R14 (DNF) | 10 Ω 1 % | UNI-ROYAL 0603WAF100JT5E | Yageo RC0603FR-0710RL | C109318 | same series, no match |
| C18 (DNF) | 10 nF X7R 50 V | FH 0603B103K500NT | Samsung CL10B103KB8NNNC | C1589 | same series, no match |

Already shared, unchanged: C15849 (1 µF), C19702 (10 µF), C98220 (10 kΩ), C12624 and C2286 (LEDs), C165948 (USB-C).

Kept:
- D3 stays Hottech BAT54WS (C191198), the user's choice. LRC LBAT54HT1G (C12744, solar-power-module) is equivalent, but its EasyEDA library footprint differs.
- U5 stays ST USBLC6-2SC6 (C7519). The marauder BOM lists C323793, whose LCSC page returned "Page Not Found" on 2026-11-09.
- 4.7 µF, 47 µF and all other parts have no match in the reference BOMs.

Checks:
- Every new part was found in the EasyEDA library by LCSC code. Its footprint UUID is the same as the old part's (R0603 or C0603), and its tolerance, dielectric, voltage and power are equal to the old ones.
- Only the Manufacturer, Manufacturer Part, Supplier Part, Datasheet and rating attributes were changed, in place, on the 27 schematic parts and on the same 27 PCB parts. The PCB was not updated from the schematic. Positions, rotations and layers read back unchanged, still 58 parts. DRC: 0 errors.
- The new `bom-rev1.csv` differs from the previous one only in the 9 changed lines. In `placement-rev1.csv` only the Datasheet column changed, for the 27 parts. Columns 1–17 are identical, so the Device column still names the old library device. The API export of the placement file covered a 2 × 2 panel, so only the first 58 rows (one board) were kept. The Gerbers were not exported again; copper did not change.
- All new parts are JLCPCB Extended parts; most replaced parts were Basic. Stock and prices were not checked.

### Open items before ordering

- All open audit items from step 4 still apply.
- D4 attributes were changed in place, so the schematic and PCB parts still link to the old library device. ERC is clean; if EasyEDA warns that attributes do not match, use its device standardization or replace the device.
- The 27 passives changed on 2026-11-09 also keep the link to their old library device. Same remedy as for D4.
- Measure the J4 Cd switch sense and the SW1 ON direction on the first board.
- J4 on the bottom: confirm the enclosure and the battery position leave room for the 2 mm slot and for card access.
- J4 is soldered on the second side, by hand, after the top side.
- Review the design, including the custom-routed copper, with the fabricator's DFM check.

## Battery

Selected by the user: **PoliCell LP502030-PCM**, 200 (250) mAh, 3.7 V, with protection, actual size 5 × 20 × 30 mm. No data sheet was reviewed for it. The documents below were reviewed as examples only.

| Document | Cell | Size | Capacity | Limits | Protection |
|---|---|---|---|---|---|
| Robiton LP-100-601120 | Li-pol | 20 × 11 × 6 mm max | 100 mAh | Fast charge 1C, 2C continuous discharge, minimum 2.75 V | Not stated |
| LP103450 drawing | Li-pol | 34 × 50 × 10 mm | 2000 mAh | Outside the agreed 100–200 mAh range | PCM, no NTC; AWG26 red and black wires |

J1 stays. The user prepares the plug. Polarity is still to be checked before connecting: pin 1 = "+".
