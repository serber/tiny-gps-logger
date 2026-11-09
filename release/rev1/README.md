# Release rev1

Prototype release, exported from EasyEDA Pro 3.2.209 on 2026-11-07. BOM and placement files exported again on 2026-11-09 after the passives change; the Gerbers are unchanged. **Not validated and not ordered.** It has not been built or measured.

| File | Content |
|---|---|
| `gerber-rev1.zip` | Gerber and drill files, four layers. Upload this to the fab |
| `BOM.md` | Parts list with order links |
| `bom-rev1.csv` | EasyEDA BOM export, 55 fitted parts in 30 lines, UTF-16 |
| `placement-rev1.csv` | Part positions, all 58 parts, UTF-16 |
| `SHA256SUMS` | Checksums |
| `assembly-rev1.step` | 3D assembly, 30 MB, not in git and not exported again on 2026-11-07; export it from EasyEDA when needed |

L1, R14, and C18 are not fitted. They are absent from the BOM but present in the placement file, so remove them if a fabricator assembles the board.

The order instruction is in the [root README](../../README.md#ordering).

## Checks on these files

- DRC on the saved PCB: 0 errors, immediately before export.
- Drill: 93 vias, 7 J5 holes, 4 plated J2 slots, 6 non-plated holes.
- Top mask openings at the new J3, L1, D3 and R12 positions, and no Top copper points inside the J3 keep-out at x 42.5–44.9, y 16.2–18.2 mm.
- BOM holds D4 as C699105. J4 is the only bottom-side part in the placement file.

## Not verified

- The stack-up is not stored in the Gerbers. It must be selected when ordering, and the JLC impedance calculator result is still unconfirmed.
- The fabricator's DFM check has not been run.
- Fit with the enclosure and the battery has not been checked.
- LCSC stock and prices have not been checked.
- The `Device` column of `placement-rev1.csv` names the old library device for the 27 passives changed on 2026-11-09; use the BOM.
- See "Open items before ordering" and "Check on the first board" in [`docs/DESIGN-RECORD.md`](../../docs/DESIGN-RECORD.md) and [`firmware/BOARD.md`](../../firmware/BOARD.md).
