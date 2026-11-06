# tiny-gps-logger

A tiny battery-powered GPS logger. It creates a new CSV file on microSD at each start and writes the position about once a second.

![Board, top side](docs/images/board-top.png)

*Render of the 3D model, not a photo. Copper is not shown.*

- 46 × 28 mm, 4-layer board
- u-blox MAX-M10S GNSS module, U.FL antenna connector
- STM32L412KBU6 MCU
- microSD slot on the bottom side
- Li-Po battery with USB-C charging, coin cell for GNSS backup

## Status

| Part | State |
|---|---|
| PCB | Designed in EasyEDA Pro, not yet manufactured |
| Firmware | Not started |

![Board, bottom side](docs/images/board-bottom.png)

## Ordering

Files are in [`release/rev1`](release/rev1). Upload `gerber-rev1.zip` and send this instruction:

```text
5PCS, 4 LAYERS, FR4, 1.6MM, OUTER 1OZ, INNER 0.5OZ, STACKUP JLC04161H-3313, NO IMPEDANCE CONTROL, Purple Soldermask, White Legend, ENIG, TENTED VIAS, order number will be added
```

The stack-up is not stored in the Gerbers, so select it when ordering. The open items are in the [design record](docs/DESIGN-RECORD.md).

## Bill of materials

See [`release/rev1/BOM.md`](release/rev1/BOM.md).

## Repository

| Path | Content |
|---|---|
| [`release/rev1`](release/rev1) | Gerbers, BOM, and placement for ordering |
| [`docs/SPEC.md`](docs/SPEC.md) | PCB specification |
| [`docs/DESIGN-RECORD.md`](docs/DESIGN-RECORD.md) | Design decisions, checks, and open items |
| [`firmware/BOARD.md`](firmware/BOARD.md) | Board reference for firmware |

The EasyEDA Pro project is not stored in this repository.
