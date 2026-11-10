# tiny-gps-logger

A tiny battery-powered GPS logger. It creates a new CSV file on microSD at each start and writes the position about once a second.

![Board, top side](docs/images/board-top.png)

*Render, not a photo. L1, R14 and C18 are not fitted.*

## Status

| Part | State |
|---|---|
| PCB | rev1, EasyEDA Pro, DRC 0 errors. Not manufactured |
| Firmware | Alpha: builds and passes host tests, not run on hardware |
| Validation | Calculated and checked in CAD only. Nothing measured |

Open items: [design record](docs/DESIGN-RECORD.md#open-items-before-ordering).

## Features

- u-blox MAX-M10S GNSS, U.FL antenna connector, coin cell for GNSS backup
- STM32L412KBU6 MCU, USB DFU and SWD programming
- microSD slot on the bottom side
- Li-Po battery with USB-C charging, runs from USB without a battery

## Specifications

| Parameter | Value |
|---|---|
| Board | 46 × 28 mm, 4 layers, 1.6 mm |
| Battery | 1S Li-Po, 3.0–4.2 V, LP502030-PCM (200 mAh) |
| Charge current | 50 mA |
| Current | 15–20 mA average, 200–250 mA peak at start (calculated) |
| Runtime | About 10 h on 200 mAh (calculated) |

![Board, 3D view](docs/images/board-overview.png)

![Board, bottom side](docs/images/board-bottom.png)

## Ordering

Upload [`release/rev1/gerber-rev1.zip`](release/rev1) and send:

```text
5PCS, 4 LAYERS, FR4, 1.6MM, OUTER 1OZ, INNER 0.5OZ, STACKUP JLC04161H-3313, NO IMPEDANCE CONTROL, Purple Soldermask, White Legend, Lead-free HASL, TENTED VIAS, order number will be added
```

| Item | Why |
|---|---|
| Stack-up JLC04161H-3313 | Not stored in the Gerbers, so select it. The RF line width is calculated for it. Other fabs: L1–L2 and L3–L4 prepreg 0.0994 mm, εr 4.1, core 1.265 mm |
| No impedance control | The RF line is 2.7 mm long |
| Lead-free HASL | Cheaper. U2 has 0.5 mm pitch: solder with a preheater and hot air, check for bridges. ENIG is an option at extra cost |

## Bill of materials

See [`release/rev1/BOM.md`](release/rev1/BOM.md).

## Firmware

C11 on bare metal with STM32Cube LL and FatFs. Build, flash and use: [`firmware/README.md`](firmware/README.md).

## Repository

| Path | Content |
|---|---|
| [`release/rev1`](release/rev1) | Ordering package: Gerber, BOM, placement |
| [`docs/SPEC.md`](docs/SPEC.md) | Requirements, decisions, pin map |
| [`docs/DESIGN-RECORD.md`](docs/DESIGN-RECORD.md) | Decisions, calculations, checks, open items |
| [`docs/images`](docs/images) | Renders |
| [`firmware`](firmware) | Firmware, its specification and board reference |

The EasyEDA Pro project `Tiny GPS Logger` is not stored here.
