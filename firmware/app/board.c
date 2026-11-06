#include "board.h"

#include "main.h"

#define SYSTEM_MEMORY_BASE 0x1FFF0000u

/* Expected level. firmware/BOARD.md: confirm SD_DET on the first board. */
#define SD_DET_INSERTED_LEVEL 0u

static volatile uint32_t tick_ms;

void board_init(void)
{
    LL_SYSTICK_EnableIT();
}

void board_tick_isr(void)
{
    tick_ms++;
}

uint32_t board_millis(void)
{
    return tick_ms;
}

void board_set_led(bool on)
{
    if (on) {
        LL_GPIO_SetOutputPin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
    } else {
        LL_GPIO_ResetOutputPin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
    }
}

bool board_card_inserted(void)
{
    const uint32_t level = LL_GPIO_IsInputPinSet(SD_DET_GPIO_Port, SD_DET_Pin);

    return level == SD_DET_INSERTED_LEVEL;
}

void board_enter_bootloader(void)
{
    const uint32_t stack = *(const volatile uint32_t *)SYSTEM_MEMORY_BASE;
    const uint32_t entry = *(const volatile uint32_t *)(SYSTEM_MEMORY_BASE + 4u);

    __disable_irq();
    SysTick->CTRL = 0;
    for (unsigned i = 0; i < sizeof NVIC->ICER / sizeof NVIC->ICER[0]; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    /* Return what the application used to its reset state. GPIO reset leaves SWD active. */
    LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_USART1 | LL_APB2_GRP1_PERIPH_SPI1);
    LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_USART1 | LL_APB2_GRP1_PERIPH_SPI1);
    LL_AHB1_GRP1_ForceReset(LL_AHB1_GRP1_PERIPH_DMA1);
    LL_AHB1_GRP1_ReleaseReset(LL_AHB1_GRP1_PERIPH_DMA1);
    LL_AHB2_GRP1_ForceReset(LL_AHB2_GRP1_PERIPH_ALL);
    LL_AHB2_GRP1_ReleaseReset(LL_AHB2_GRP1_PERIPH_ALL);
    (void)LL_RCC_DeInit();

    LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SYSTEMFLASH);
    __DSB();
    __ISB();
    __set_MSP(stack);
    __enable_irq();
    ((void (*)(void))entry)();

    for (;;) {
    }
}

void board_shutdown(void)
{
    __disable_irq();
    SysTick->CTRL = 0;
    LL_PWR_ClearFlag_WU();
    LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
    LL_LPM_EnableDeepSleep();
    for (;;) {
        __WFI();
    }
}
