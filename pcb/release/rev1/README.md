# Tiny GPS Logger PCB, revision 1

Prototype release, exported from EasyEDA Pro 3.2.209 on 2026-11-05. It has not been built or measured. Do not treat it as validated.

## Files

| File | Use | SHA-256 |
|---|---|---|
| `tiny-gps-logger-rev1-gerber.zip` | Gerber and drill files | `37117dab8d0b1463d956673e28254dab7befa324b51523151d6d12b0f82349be` |
| `tiny-gps-logger-rev1-bom.csv` | BOM, 55 fitted parts in 30 lines, UTF-16 | `51ecbed4e22b035dd0447d1a6c4c9f9b71c3bec0ecea04324a772ca64a320989` |
| `tiny-gps-logger-rev1-cpl.csv` | Placement, all 58 parts, UTF-16 | `157a7b391c851e5970d47ef7c53045ed363c4b12f0c9cc4e317246fed1c6da72` |
| `tiny-gps-logger-rev1.step` | 3D model with components | `082acb91cf8e019e0e593381ecb4fbc38526a7cb906dfa4e0c1a1d710d485506` |

The STEP file is 30 MB and is kept out of git. Export it again from EasyEDA if it is missing.

L1, R14 and C18 are not fitted. They are absent from the BOM but present in the placement file, so remove them if a fabricator assembles the board.

## Order text

```text
5PCS, 4 LAYERS, FR4, 1.6MM, OUTER 1OZ, INNER 0.5OZ, STACKUP JLC04161H-3313, NO IMPEDANCE CONTROL, Purple Soldermask, White Legend, ENIG, TENTED VIAS, order number will be added
```

## Checks on these files

- DRC on the saved PCB: 0 errors, immediately before export.
- Drill: 93 vias, 7 J5 holes, 4 plated J2 slots, 6 non-plated holes.
- Top mask opening at the new BT1 position, and no Top copper points inside the J3 keep-out.
- BOM holds D4 as C699105. J4 is the only bottom-side part in the placement file.

## Not verified

- The stack-up is not stored in the Gerbers. It must be selected when ordering, and the JLC impedance calculator result is still unconfirmed.
- The fabricator's DFM check has not been run.
- Fit with the enclosure and the battery has not been checked.
- LCSC stock and prices have not been checked.
- See "Open items before ordering" and "Check on the first board" in `../../DESIGN-RECORD.md` and `../../../firmware/BOARD.md`.
