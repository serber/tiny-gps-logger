# Board reference for firmware

What the firmware needs to know about the PCB. Pin numbers and nets were read from the EasyEDA board. Function assignments come from `pcb/SPEC.md` section 9.1, checked against ST DS12469. CubeMX was not used, so confirm alternate functions there when the project is created. Nothing here has been measured on hardware yet.

## MCU

STM32L412KBU6, UFQFPN-32: 128 KB Flash, 40 KB RAM. Supply is 3.3 V on all rails.

No crystals are fitted. Use the internal oscillators, HSI48 with CRS for USB. Time of day comes from GPS.

## Pin map

| Pin | Port | Net | Direction | Function |
|---|---|---|---|---|
| 6 | PA0 | VBAT_SENSE | Analog in | Battery voltage, 1:2 divider |
| 7 | PA1 | VBUS_SENSE | Analog in | USB present, 1:2 divider |
| 10 | PA4 | SD_CS | Output | SPI1 chip select, software controlled |
| 11 | PA5 | SD_SCK_MCU | Output | SPI1_SCK, 33 Ω series resistor R11 |
| 12 | PA6 | SD_MISO | Input | SPI1_MISO |
| 13 | PA7 | SD_MOSI | Output | SPI1_MOSI |
| 14 | PB0 | SD_DET | Input | Card detect, external pull-up |
| 15 | PB1 | GPS_EXTINT | Output | GPS EXTINT |
| 18 | PA8 | GPS_TP | Input | GPS time pulse |
| 21 | PA11 | USB_DN | USB | USB FS |
| 22 | PA12 | USB_DP | USB | USB FS, no external pull-up |
| 23 | PA13 | SWDIO | Debug | SWD |
| 24 | PA14 | SWCLK | Debug | SWD |
| 26 | PB3 | LED_STATUS | Output | Green LED D1, active high, 1 kΩ |
| 27 | PB4 | GPS_RESET | Open-drain | GPS RESET_N. Never drive it high |
| 29 | PB6 | GPS_RXD | Output | USART1_TX, to the GPS RXD pin |
| 30 | PB7 | GPS_TXD | Input | USART1_RX, from the GPS TXD pin |
| 31 | PH3 | BOOT0 | Input | 10 kΩ pull-down, J5 pin 6 |
| 4 | NRST | NRST | — | 100 nF to GND, J5 pin 4 |

Net names GPS_TXD and GPS_RXD are from the GPS side.

Unconnected: PC14, PC15, PA2, PA3, PA9, PA10, PA15, PB5. Set them to analog mode. PA10 floats until firmware runs.

## GPS: MAX-M10S

- UART default is 9600 8N1. Configure the module on every start. Planned output is UBX-NAV-PVT at 1 Hz, received with DMA.
- GPS_RESET has no capacitor. Do not hold it low for normal operation.
- Do not pull GPS_TP low at startup: a low TIMEPULSE makes the module enter safeboot. Leave PA8 without a pull-down.
- SAFEBOOT_N goes only to J5 pin 7.
- BT1 keeps the module's backup domain alive through V_BCKP. It charges from 3V3 through D3 and R13.
- The active antenna supply is not fitted. L1, R14 and C18 are left off, so VCC_RF does not reach the antenna connector.

## microSD

- SPI mode on SPI1. The slot has pull-ups: R16 on CS, R17 on MOSI, R18 on MISO, R19 on DAT1, R15 on DAT2.
- The card is powered directly from 3V3 and cannot be switched off.
- SD_DET: the expected behaviour is low with a card inserted, but the drawing does not state it. Check it on the first board.
- FAT32 through FatFs. Cards above 32 GB must be formatted as FAT32 by hand.

## Power and sensing

- **VBAT_SENSE:** R2 and R3 are 220 kΩ, with 100 nF on the pin. VBAT = 2 × pin voltage. Source impedance is about 110 kΩ, so use a long ADC sampling time.
- **VBUS_SENSE:** R8 and R9 are 10 kΩ. About 2.5 V on the pin means USB is present.
- **SW1 off with USB connected:** the board still runs, and VBAT_SENSE reads 0. With SW1 on it reads the battery. Firmware can tell the two states apart.
- **Battery cut-off:** 3V3 stops being regulated below about 3.56 V at peak load, by calculation, and follows the battery. The ADC reference falls with it, so read VREFINT to correct the measurement.
- **Low battery:** close the file and sleep well above 3.0 V. The threshold is not chosen yet.
- **Power loss:** SW1 cuts power without warning, so use `f_sync` every few seconds. Extra capacitance on the board does not help.
- **Charging:** runs by itself through MCP73831 at 50 mA. The red LED D2 is driven by the charger. Firmware has no connection to it.

## Programming

Primary method is SWD through J5, including the first flash.

| J5 pin | Net |
|---|---|
| 1 | 3V3 (target voltage sense) |
| 2 | SWDIO |
| 3 | SWCLK |
| 4 | NRST |
| 5 | GND |
| 6 | BOOT0 |
| 7 | SAFEBOOT_N |

USB DFU is also available, with no external parts. A blank chip enters the bootloader by itself. After that, hold BOOT0 at 3V3 during reset. If the USB device does not appear, reset and try again: floating PA10 can select USART1 in the bootloader.

## Check on the first board

- Battery plug polarity before connecting: J1 pin 1 is plus.
- SW1 ON position. The expected position is the actuator toward terminal 1.
- SD_DET level with and without a card.
- 3V3 ramp at the GPS and peak current with a real card.
