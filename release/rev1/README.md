# Release rev1

Prototype, exported from EasyEDA Pro 3.2.209 on 2026-11-07. BOM and placement exported again on 2026-11-09 after the passives change; Gerber unchanged. **Not validated and not ordered.**

| File | Content |
|---|---|
| `gerber-rev1.zip` | Gerber and drill, 4 layers. Upload this to the fab |
| `BOM.md` | Parts list with order links |
| `bom-rev1.csv` | EasyEDA BOM export, 55 fitted parts, UTF-16 |
| `placement-rev1.csv` | Part positions, all 58 parts including DNF, UTF-16 |
| `SHA256SUMS` | Checksums |
| `assembly-rev1.step` | 3D assembly, 30 MB, local only; export it from EasyEDA if missing |

The order instruction is in the [root README](../../README.md#ordering).

## Checks on these files

- DRC on the saved PCB: 0 errors, immediately before export.
- Drill: 93 vias, 7 J5 holes, 4 plated J2 slots, 6 non-plated holes.
- No top copper inside the J3 keep-out, x 42.5–44.9, y 16.2–18.2 mm. J4 is the only bottom-side part.

## Not verified

- The stack-up is not in the Gerbers; select it when ordering. The JLC impedance calculator result is unconfirmed.
- Fabricator DFM, fit with the enclosure and battery, LCSC stock and prices.
- Open items: [`docs/DESIGN-RECORD.md`](../../docs/DESIGN-RECORD.md#open-items-before-ordering).
