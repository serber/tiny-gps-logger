# Firmware specification

Draft for review. This is the separate specification promised by `docs/SPEC.md` section 4. Pins and electrical facts are in `firmware/BOARD.md`. Nothing here has been built or run yet, and nothing has been measured on hardware.

## Goal

On each start, create a new CSV on the microSD card and append one position about every second until power is cut, the card is removed or the battery is low.

## Toolchain

| Item | Choice |
|---|---|
| Language | C11 |
| Drivers | STM32CubeL4: CMSIS and LL |
| Configuration | STM32CubeMX 6.18.1 with STM32Cube FW_L4 V1.18.2, generating a CMake project from `firmware.ioc`. ADC1 and DMA are not in the `.ioc`: their code uses LL directly |
| Build | CMake, Ninja, Arm GNU Toolchain (`arm-none-eabi-gcc`) |
| Scheduling | Bare metal: main loop, interrupts, DMA and `WFI` |
| File system | FatFs, with a small SD-over-SPI driver of our own |
| Editor | VS Code with clangd, CMake Tools and Cortex-Debug |
| Flashing | USB DFU with `dfu-util`. SWD through J5 once an ST-Link is available |

## Clocks and power

- SYSCLK is HSI16 at 16 MHz, voltage scaling range 1 (the reset default), 0 wait states. No crystal is fitted. Range 2 would need 2 wait states at this clock.
- The application does not use USB, so HSI48 stays off. USB is only for charging and the system bootloader.
- The MCU sleeps with `WFI` between events. Stop modes are not used in revision 1, because UART DMA must keep running.

## GPS

- USART1 at 9600 8N1, received into a 512-byte circular DMA buffer on DMA1 channel 5, request 2. The main loop reads it after each SysTick wake-up.
- Configuration is sent on every start, and after each GPS reset, with UBX-CFG-VALSET to the RAM layer: disable NMEA output on UART1, enable UBX-NAV-PVT and UBX-NAV-DOP on UART1, 1 Hz measurement rate. Check each ACK and retry. Key IDs were compared with pyubx2, not with the u-blox document.
- 9600 baud is enough: about 126 bytes per second against 960 available. Staying at the default also means there is no baud rate to recover after a reset.
- Nothing is stored in the module's BBR or Flash, so the module always starts from defaults.
- `hdop` comes from UBX-NAV-DOP because UBX-NAV-PVT carries only pDOP. `docs/SPEC.md` section 4 plans NAV-PVT alone, which cannot fill this column.
- A row is written after a NAV-PVT and NAV-DOP pair with the same `iTOW` has arrived.
- If no valid UBX arrives for 5 s, pulse GPS_RESET low briefly and configure again. PB4 stays open-drain and is never driven high.

## Logging

- Files: `LOG00001.CSV`, `LOG00002.CSV`, … in the root directory. The next number is one above the highest existing number.
- The first line is the header: `utc_time,lat,lon,alt_m,speed_mps,course_deg,sats,hdop,fix_type`.
- Example: `2026-11-06T09:30:15Z,55.7558261,37.6173001,152.3,1.24,87.5,14,0.82,3`.
- Rows are written only when the fix is valid: `gnssFixOK` is set, `fixType` is 2 or 3, and date and time are valid. A start without a fix leaves a file with a header only.
- `alt_m` is height above mean sea level (`hMSL`).
- Each row goes to `f_write`. FatFs holds it in its 512-byte sector buffer, so most calls do not touch the card. `f_sync` runs every 5 s, so a power cut loses at most about 5 s of track.
- GPS data keeps arriving in the DMA buffer during a slow SD write, which can take hundreds of ms. 512 bytes hold about 4 s of output.
- A full card or the file number 99999 counts as an SD error.

## Card handling

- SD_DET polarity is not confirmed. It is one constant in the code, set on the first board.
- SPI starts at 400 kHz or lower for card initialisation, then 8 MHz.
- Card removed: stop writing, show the SD error pattern, and wait. Card inserted again: mount it and start a new file.
- Write error: close the file if possible, then try the same recovery once before showing the SD error pattern.

## Battery

- Sample VBAT_SENSE and VREFINT every 10 s. Use the longest ADC sampling time, because the source impedance is about 110 kΩ.
- VBAT = 2 × pin voltage, with the reference corrected from VREFINT.
- Cut-off: below 3.30 V on 3 readings in a row, close the file, put the GPS into backup mode and enter MCU Shutdown mode. Only SW1 off and on again, or a reset, leaves it, even if USB is connected in the meantime. This value was chosen as a working estimate, not calculated from a discharge curve. Confirm it with a discharge test on the real board.

## USB present

USB power is read from VBUS_SENSE. The firmware cannot see the charger state, so USB present is treated as charging, whatever the SW1 position.

| At start | Action |
|---|---|
| USB present, card inserted | Do not log. Sleep so the battery charges |
| USB present, no card | Jump to the system bootloader for DFU |
| No USB | Log as usual |

- In this state the GPS is put into backup mode with UBX-RXM-PMREQ. The next configuration wakes it, and the 5 s watchdog resets it if it stays silent.
- If USB is plugged in while logging, close the file and sleep.
- If USB is removed while sleeping for charge, reset the MCU, so a normal start creates a new file. Check this against SW1 behaviour on the first board: with SW1 off, removing USB simply cuts power.
- `APP_LOG_ON_USB` logs with USB present, for bench work. Debug builds enable it, release builds do not. Flash a Release build for real use.
- A broken application cannot reach the bootloader this way. Recovery then needs J5 pin 6 held at 3V3 during reset, or SWD.

## LED

| State | Pattern |
|---|---|
| Starting, no fix yet | Short blink every 1 s |
| Fix, logging | Short blink every 5 s |
| SD error or no card | Two fast blinks every 1 s |
| GPS not responding | Four fast blinks every 1 s |
| Low battery | Off |

## Modules

All in `app/`:

| File | Responsibility | Tested on the Mac |
|---|---|---|
| `app.c` | Start-up decision and main loop | No |
| `ubx.c` | UBX parser, decoding, frame building | Yes |
| `csv.c` | CSV rows, file names | Yes |
| `policy.c` | Start action, voltage maths, battery filter, LED patterns | Yes |
| `logger.c` | Log files on FatFs | Yes, on a RAM disk |
| `gps.c` | UART, DMA, module configuration, watchdog | No |
| `sd_spi.c`, `diskio.c` | SD card over SPI, FatFs media layer | No |
| `power.c` | ADC, battery and USB state | No |
| `board.c` | Time base, card detect, bootloader jump, Shutdown | No |

Run the host tests with `test/run.sh`. `test/ubx_vectors.h` holds reference frames made with pyubx2.

No watchdog timer is used yet. An SD or GPS hang that the timeouts miss would stop logging until the next power cycle.

## Status

Alpha. All modules are written and build for the target, and the host tests pass. Nothing has run on hardware.

Check on the first board, in this order:

1. LED blinks, SD_DET polarity (`SD_DET_INSERTED_LEVEL` in `board.c`).
2. VBUS_SENSE and VBAT_SENSE readings against a meter.
3. SD: real cards, including removal during a write.
4. GPS: ACK to the configuration, NAV-PVT and NAV-DOP arrive, RXM-PMREQ and wake-up.
5. Bootloader jump from the application, Shutdown current, unplugging USB with SW1 on.
