# Tiny GPS Logger — PCB specification

Status: **revision 1 released for prototype manufacture, not built yet.** Files are in [release/rev1](../release/rev1/README.md). Decisions are in section 8, schematic details in section 9. Process records are in [DESIGN-RECORD.md](DESIGN-RECORD.md).

EDA: EasyEDA Pro, project `Tiny GPS Logger` (Board1 / Schematic1 / PCB1).

## 1. Purpose

A tiny standalone battery-powered GPS logger.

Firmware behaviour:

1. On power-up the firmware starts and creates a **new CSV file** on the microSD card.
2. It appends coordinates from the GPS module at a 1 s interval.
3. Logging continues while the board is on. Power-off is a physical battery disconnect.

## 2. Bill of materials (key parts)

| Function | Part | LCSC | Stock / price¹ | Notes |
|---|---|---|---|---|
| GNSS | u-blox MAX-M10S-00B | C4153167 | 1013 / $10.78 | Kept, no replacement needed |
| MCU | STM32L412KBU6 (UFQFPN-32 5×5) | C529439 | 1388 / $3.67 | Selected |
| microSD | TF PUSH (push-push, card detect) | C393941 | 278651 / $0.07 | 2 mm high |
| GPS backup cell | Seiko MS621FE-FL11E (6.8 mm, 5.5 mAh) | C80909 | unknown | Hand soldering only, see 4.3 |
| Antenna connector | U.FL-R-SMT-1 | C88374 | 349490 / $0.11 | Footprint `ANT-SMD_UFL-R-SMT-1` |
| Status LED | KT-0603G (green) + 1 kΩ | C12624 | — | Driven by the MCU |
| Charge LED | KT-0603R (red) + 1 kΩ | C2286 | — | Driven by the charger STAT pin |
| USB connector | TYPE-C-31-M-12 | C165948 | — | |
| USB ESD | USBLC6-2SC6 | C7519 | — | |
| Debug header | MTP315-1107S1, 1×7, 1.27 mm | C5142369 | — | Optional, see 9.3 |
| Battery connector | BX-SH1.0-2PWT | C18077908 | unknown | As requested, 1 A |
| Power switch | ALPS SSSS811101, 300 mA, 5 V | C109335 | 35237 / $0.15 | See 4.4 |
| 3.3 V LDO | Richtek RT9080-33GJ5, 600 mA, Iq 2 µA | C841192 | unknown | Do not substitute, see 9.2 |
| Li-Po charger | MCP73831T-2ACI/OT | C424093 | 11178 / $0.79 | |
| Power path FET | AO3401A | C15127 | — | See 9.2 |
| USB power diode | B5819WS (Yangjie) | C699105 | — | See 9.2 |

¹ jlcsearch data as of 2026-11-04, indicative only. Re-check before ordering.

## 3. Block diagram

```text
 [Li-Po 1S]--[BX-SH1.0]--+--[SW]--VBAT_SW--+--[P-FET]--VSYS--[LDO 3.3V]--3V3--+--> MAX-M10S VCC + V_IO
                         |                 |                ^                  +--> STM32L412
                         |                 +--[divider]-ADC |                  +--> microSD
 [USB-C]--+--[charger]---+                                  |                  +--> LED
          +-------------------[Schottky]--------------------+
                                                               |
                                          3V3--[diode]--[R]--+-+
                                                             |
                                       MS621FE --- V_BCKP (MAX-M10S)

 MAX-M10S  <-- UART 9600 8N1 -->  STM32L412  <-- SPI -->  microSD
 U.FL  ---- 50 ohm ---->  RF_IN
```

## 4. Design decisions

### 4.1. GPS: MAX-M10S

From data sheet UBX-20035208 R08 (2026-01-30):

- VCC and V_IO are tied together to 3V3. Range with VIO_SEL open: 2.7–3.6 V.
- **Pin 15 VIO_SEL stays unconnected**, which selects 3.3 V mode. The older Integration Manual (R01) calls this pin Reserved; the requirement is the same.
- Current: 9.6–11.8 mA tracking at 1 Hz (3.0 V; VCC + V_IO; GPS only / default constellations).
- Startup inrush up to 100 mA; the supply must deliver it.
- Supply line series resistance at most 0.2 Ω. No ferrites or resistors in series with VCC/V_IO.
- V_IO ramp limit: 25–35000 µs/V. Exceeding it may **permanently damage the module**. Verify the LDO startup with the chosen output capacitance.
- The module has no overvoltage or reverse-voltage protection.
- PIOs must not be driven while the module is unpowered. The single 3V3 rail guarantees this.
- Connected to the MCU: TXD, RXD, EXTINT, TIMEPULSE, RESET_N. On the debug header: SAFEBOOT_N.
- RESET_N: no capacitor to ground (u-blox requirement).
- TIMEPULSE must not be loaded low at startup, otherwise the module enters safeboot.
- Default UART: 9600 8N1, configurable up to 921600.
- VCC_RF: active antenna supply output, up to 50 mA in operation.

### 4.2. MCU: STM32L412KBU6

Requirements: UART, SPI, FatFs (512-byte buffer plus filesystem stack), low power, few external parts, available at LCSC.

| Candidate | Pros | Cons |
|---|---|---|
| **STM32L412KBU6** | Cortex-M4 80 MHz, 128 KB Flash, 40 KB RAM, crystal-less USB FS, low power, 5×5 mm, in stock | $3.67 |
| STM32U073KCU6 | 256 KB Flash, newer | Not found in jlcsearch, scarce at distributors, 52+ week lead time |
| STM32G071KBU6 | Cheaper | No USB |
| RP2040 | Cheap | External Flash and crystal, higher power and area |
| ESP32-C3 | Wi-Fi offload | Current draw, radio next to GNSS, 3.0 V minimum, needs an antenna |

Pin assignment must be confirmed in STM32CubeMX before layout.

### 4.3. GPS backup cell: MS621FE-FL11E

From Seiko data:

- Charge voltage 2.8–3.3 V, standard 3.1 V. Internal impedance 80 Ω.
- **MS621FE is not reflowable; hand soldering only.** Solder it last.
- MS621T-FL11E (C5346588) shares the footprint. Its reflow rating is unconfirmed.

Charge circuit: `3V3 → Schottky diode → resistor → V_BCKP + cell`.

- The cell connects to V_BCKP directly, with no series resistor: u-blox requires low resistance on this line.
- The diode prevents the cell from feeding the 3V3 rail.
- Seiko's product page gives no charge resistor value, so the value is a conservative choice to be reviewed.

Hardware backup runtime: 5.5 mAh / 28 µA ≈ 195 h (about 8 days), theoretical. That covers ephemeris validity (about 4 h) for hot starts and a few days for warm starts.

### 4.4. Power

- Battery: PoliCell LP502030-PCM, Li-Pol 3.7 V, 200 (250) mAh, with protection, 5 × 20 × 30 mm (3.0–4.2 V).
- The switch breaks the battery line, as requested. MSK12C02 is **not suitable**: rated 50 mA against 200+ mA peaks. A rating of at least 300 mA is required.
- Battery sense: divider after the switch (so nothing drains the battery when off) plus 100 nF at the ADC pin.

Current estimate (calculated, not measured):

| Load | Average current |
|---|---|
| MAX-M10S, tracking | 10–12 mA |
| MCU | 2–4 mA |
| microSD, buffered writes | 1–5 mA |
| LED, short flashes | up to 0.2 mA |
| **Total** | **15–20 mA** |

That gives roughly 10 h from 200 mAh.
Peaks: 200–250 mA at startup (GPS plus SD initialisation).

### 4.5. microSD

- SPI, 3V3 supply. Pull-ups on CMD, DAT1, DAT2 and CS; 10 µF + 100 nF decoupling.
- FAT32 through FatFs. Cards above 32 GB must be formatted as FAT32 manually.
- No SD power switching, to save space.

### 4.6. Antenna and RF

- U.FL → RF_IN line: 50 Ω, coplanar with ground on layer 2, shortest possible length. The routed line is 1.8 mm, so it has no stitching vias along it (see DESIGN-RECORD.md, step 7).
- Solid ground under the module on Top and layer 2, no signals there.
- Keep U.FL close to the module, away from the SD card and SPI clock.
- Active antenna bias-T per the u-blox reference design: VCC_RF → 10 Ω → 27 nH → RF, plus 10 nF to ground. Fitted footprints, not populated by default.

### 4.7. PCB

- 4 layers, JLCPCB stackup. Two layers would make the 50 Ω trace too wide.
- 50 Ω width is calculated with the JLC impedance calculator for the chosen stackup.

Fabrication order text:

```text
5PCS, 4 LAYERS, FR4, 1.6MM, OUTER 1OZ, INNER 0.5OZ, STACKUP JLC04161H-3313, NO IMPEDANCE CONTROL, Purple Soldermask, White Legend, ENIG, TENTED VIAS, order number will be added
```

- Stack-up: the RF line width is calculated for this stack, so it must be stated even though the RF line is too short (1.8 mm) to need paid impedance control. Other fabricators need the build instead of the JLC code: L1–L2 and L3–L4 prepreg 0.0994 mm, εr 4.1, core 1.265 mm.
- ENIG: U2 has 0.5 mm pitch and U1 and J3 need flat pads. HASL is cheaper and possible, but it is uneven at this pitch.
- The drill files contain plated slots for J2 and non-plated holes for J2, J4 and SW1.

## 5. Firmware (preliminary, as far as it affects the hardware)

A detailed firmware specification will be separate.

- GPS: UBX-NAV-PVT and UBX-NAV-DOP at 1 Hz, UART with DMA. Configuration is sent on every start.
- Files: `LOG00001.CSV`, `LOG00002.CSV`, … A counter is used because GPS time is unknown at startup.
- Columns: `utc_time, lat, lon, alt_m, speed_mps, course_deg, sats, hdop, fix_type`.
- Buffered writes with `f_sync` every few seconds.
- LED: distinct patterns for no fix / fix / SD error.

### Risk: power-off during a write

The switch cuts power without warning. The last few seconds of a track may be lost, with a small risk of corrupting a FAT write.

Mitigation: frequent `f_sync`, block writes, one file per session. Bulk capacitance does not help noticeably: 100 µF gives roughly 1 ms at this load, while an SD write can take hundreds of ms.

On low battery (ADC) the firmware closes the file and goes to sleep.

## 6. Out of scope for revision 1

- External sensors, display, buttons.
- Wi-Fi / BLE.
- SAW filter: no transmitters on the board.
- Antenna supervisor.

## 7. Stages

1. Agree this specification.
2. Schematic in EasyEDA, ERC and review.
3. Placement and routing, DRC, review.
4. Gerber, BOM, CPL, order.
5. Firmware.

## 8. Decisions

| Question | Decision |
|---|---|
| MCU | STM32L412KBU6 |
| Charging | USB-C and on-board charger (MCP73831), 50 mA; PoliCell LP502030-PCM battery |
| Battery connection | J1 connector kept; the user prepares the plug |
| Board size | 46 × 28 mm, 1 mm corner radius (35 × 22 mm target was not reachable) |
| USB power | Board runs from USB without a battery, through a power path |
| Antenna | Both: bias-T on board, not populated by default (passive) |
| Assembly | By hand: 0603 passives; all parts on top except the microSD slot J4, which is on the bottom |

Assumptions still to confirm:

- Battery plug pinout: pin 1 = "+" (red). **There is no reverse-polarity protection**; check polarity with a multimeter before connecting.

## 9. Schematic (draft)

Three A4 pages in `Schematic1`: POWER, MCU and GNSS_SD. 58 components, 43 nets; nets connect across pages by name. ERC reports no errors and no warnings. The exported netlist matches the intended connections pin by pin, with no single-pin nets.

### 9.1. MCU pin map

| Pin | Signal | Function |
|---|---|---|
| PA0 | VBAT_SENSE | ADC, battery divider 1:2 |
| PA1 | VBUS_SENSE | USB present, divider 1:2 |
| PA4 | SD_CS | SPI1 chip select |
| PA5 | SD_SCK_MCU | SPI1_SCK, 33 Ω series resistor to the card |
| PA6 | SD_MISO | SPI1_MISO |
| PA7 | SD_MOSI | SPI1_MOSI |
| PA8 | GPS_TP | Time pulse input |
| PA11 / PA12 | USB_DN / USB_DP | USB FS |
| PA13 / PA14 | SWDIO / SWCLK | Debug |
| PB0 | SD_DET | Card detect |
| PB1 | GPS_EXTINT | GPS external interrupt |
| PB3 | LED_STATUS | Status LED |
| PB4 | GPS_RESET | Must be driven open-drain |
| PB6 | GPS_RXD | USART1_TX to GPS |
| PB7 | GPS_TXD | USART1_RX from GPS |
| PH3 | BOOT0 | 10 kΩ pull-down |

Pins and functions verified against ST DS12469 Rev 9, table 14, and against the STM32CubeMX 6.18.1 pin database. Pin 17 is VDD.

### 9.2. Findings that changed the design

- **LDO:** C20617299 is a TECH PUBLIC part sold as RT9080-33GJ5, and its library symbol has VOUT on pin 4. Richtek's part has VOUT on pin 5. Use only C841192 unless the pinout of a substitute is checked.
- **V_IO ramp:** the RT9080 data sheet specifies no soft-start, and its current limit is typically 1.1 A. Staying under the 25 µs/V limit needs about 28 µF of effective capacitance on 3V3, so C3 is 47 µF. This is an estimate and must be measured on the first board.
- **Switch bounce:** R1 (100 kΩ) and C1 (1 µF) delay LDO EN by about 30 ms.
- **LED pin numbering differs:** KT-0603G has K on pin 1, KT-0603R has K on pin 2. Check polarity by the K/A names.
- **Backup diode:** BAT54WS C191198; R13 is 1 kΩ. Check that V_BCKP stays at or below 3.3 V.

- **USB power path:** a Schottky diode alone from VBUS to the battery side would have charged the battery directly, bypassing the charger. VSYS is therefore a separate rail: D4 feeds it from VBUS, and Q1 (P-FET, gate on VBUS) connects the battery only when USB is absent. Q1's body diode blocks VSYS from reaching the battery.
- **Switch behaviour:** with USB connected the board runs whatever the position of SW1. VBAT_SENSE stays on the battery side of Q1, so it reads 0 with SW1 off and the real battery voltage with SW1 on. Firmware can use this to tell the two apart.
- **VBUS_SENSE:** R8 and R9 are 10 kΩ, so D4 reverse leakage cannot look like USB being present.
- **Charge current:** 1000 V / 20 kΩ = 50 mA, per Microchip. That is 0.25C for the selected 200 mAh battery.
- **GPS UART:** moved from PA2/PA3 to PB6/PB7, away from the USART2 bootloader pins, so GPS output cannot interfere with DFU on a blank chip. The bootloader pin list itself (AN2606) was not read.

### 9.3. Assembly and bring-up notes

- J5 pinout: 3V3, SWDIO, SWCLK, NRST, GND, BOOT0, SAFEBOOT_N.
- Programming: SWD through J5 is the primary method. See DESIGN-RECORD.md. USB alone can power the board.
- R14, L1 and C18 (active antenna bias-T) are excluded from the BOM. Fit them only for an active antenna.
- SD_DET assumes the TF PUSH CD pin closes to the shell (GND) when a card is inserted. Check with a multimeter.

## 10. Sources

- [MAX-M10S Data sheet, UBX-20035208 R08](https://content.u-blox.com/sites/default/files/MAX-M10S_DataSheet_UBX-20035208.pdf)
- [MAX-M10S Integration manual, UBX-20053088 R01](https://cdn.sparkfun.com/assets/5/c/a/0/b/MAX-M10S_IntegrationManual_UBX-20053088.pdf)
- [Seiko MS621FE](https://www.sii.co.jp/en/me/datasheets/ms-rechargeable/ms621fe/)
- [Richtek RT9080, DS9080-09](https://www.richtek.com/assets/product_file/RT9080/DS9080-09.pdf)
