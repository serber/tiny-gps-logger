/* Time base, card detect and the two ways of leaving the application. */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

void board_init(void);

/* Called from SysTick_Handler. */
void board_tick_isr(void);

uint32_t board_millis(void);
void board_set_led(bool on);
bool board_card_inserted(void);

/* Jump to the ST system bootloader for USB DFU. Does not return. */
void board_enter_bootloader(void);

/* Enter Shutdown mode. Only a reset or a power cycle leaves it. Does not return. */
void board_shutdown(void);

#endif
