# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Tiny battery-powered GPS logger. The project has two parts:

- **PCB:** a 46 × 28 mm, 4-layer board with a u-blox MAX-M10S, an STM32L412KBU6 and microSD. Everything in this repository so far belongs to this part.
- **Firmware:** not started. It will create a new CSV on each start and append a position about every second. There is nothing to build, lint or test until it exists. `docs/SPEC.md` sections 5 and 9.1 list what the hardware requires of it.

## Where things live

The PCB design itself is in EasyEDA Pro, not in git: project `Tiny GPS Logger` (Board1 / Schematic1 with pages POWER, MCU, GNSS_SD / PCB1). The `docs/` and `release/` folders hold only its records:

- `docs/SPEC.md`: requirements, decisions and the MCU pin map (section 9.1). Its parts table is an early key-parts list.
- `docs/DESIGN-RECORD.md`: gate log, audits, rules, step 6–8 decisions and evidence, and open items before ordering.
- `release/rev1/BOM.md`: full parts list, read from the PCB.

`firmware/BOARD.md` is the board reference for firmware: pins, GPS, microSD, power sensing and programming. Update it if a board change touches any of these.

When a record and the CAD disagree, read the CAD. Update the records after any board change.

## Working rules

- Reply in Russian. Write all files and EasyEDA content in English.
- Follow the gated process in `~/.codex/skills/power-pcb-design/SKILL.md`, which does not appear in the Claude Code skill list. Log gate decisions in `docs/DESIGN-RECORD.md`.
- Never order, upload to a fabricator or publish without explicit permission. Commit only when asked.
- Edit EasyEDA pages in place. Never wipe and redraw them.
- State what was calculated, checked in CAD or measured. Nothing on this board has been measured yet.

## EasyEDA bridge

Use the `easyeda-api` skill. Its bridge runs on `http://localhost:49620/execute` with a 30 s call limit, so batch long operations.

- PCB layer IDs: Top 1, Bottom 2, Board Outline 11, Multi 12, L2_GND 15, L3_POWER 16.
- PCB coordinates are in mil: mm × 39.3700787.
- Activate or open the PCB document first. Calls fail with null errors when another tab or no document is active.
- Create vias before tracks: an API track links only to a via that already exists.
- After moving copper, rebuild all four GND pours (`rebuildCopperRegion()`), then run DRC. Pour clearance errors before the rebuild are expected.
- Flipping a part with `setState_Layer(2)` mirrors X, so it needs an extra 180° rotation to keep the pads in place.
- Batch deletes can fail silently, so check the counts afterwards.
- `getState_Hole()` returns a tenth of the footprint's hole size in mil. Confirm drills in the generated `Drill_PTH_Through.DRL`.
- The 3D view drew the plated J2 slots as solid copper, so check drills in the drill files instead.

## Board constraints that are easy to break

- J4 (microSD) is on the bottom side. Signal vias must stay out of its shell area, x 1.0–13.6 mm, y 11–27.5 mm, so that area can be reached only on Top.
- No Top routing under U1. No vias under its body.
- RF_IN is a 0.20 mm line on Top, referenced to L2. `J3_NO_POUR` keeps the Top pour out from between the J3 GND pads, as Hirose requires.
- L1, R14 and C18 are not fitted in revision 1. They stay on the PCB for the active antenna supply.
- Check silkscreen against real footprint outlines and label boxes from `getPrimitivesBBox`. DRC does not catch silkscreen overlaps here.
