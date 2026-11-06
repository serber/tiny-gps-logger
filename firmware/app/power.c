#include "power.h"

#include "main.h"
#include "policy.h"
#include "stm32l4xx_ll_adc.h"

/* The sense divider has about 110 kOhm source impedance, so use the longest sampling time.
 * At 16 MHz this is about 40 us per channel. */
#define SAMPLING_TIME LL_ADC_SAMPLINGTIME_640CYCLES_5

#define ADC_TIMEOUT_SPINS 10000u /* Several milliseconds at 16 MHz. */

static bool adc_ready;

static bool wait_flag(uint32_t (*read_flag)(const ADC_TypeDef *), bool wanted)
{
    uint32_t spins = ADC_TIMEOUT_SPINS;

    while ((read_flag(ADC1) != 0u) != wanted) {
        if (--spins == 0u) {
            return false;
        }
    }
    return true;
}

void power_init(void)
{
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC);
    LL_ADC_SetCommonClock(ADC12_COMMON, LL_ADC_CLOCK_SYNC_PCLK_DIV1);
    LL_ADC_SetCommonPathInternalCh(ADC12_COMMON, LL_ADC_PATH_INTERNAL_VREFINT);

    LL_ADC_DisableDeepPowerDown(ADC1);
    LL_ADC_EnableInternalRegulator(ADC1);
    LL_mDelay(1); /* Regulator start-up, and VREFINT buffer start-up. */

    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);
    LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_5, SAMPLING_TIME);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_6, SAMPLING_TIME);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, SAMPLING_TIME);

    LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
    if (!wait_flag(LL_ADC_IsCalibrationOnGoing, false)) {
        return;
    }
    LL_mDelay(1); /* ADEN must wait a few ADC cycles after calibration. */

    LL_ADC_ClearFlag_ADRDY(ADC1);
    LL_ADC_Enable(ADC1);
    adc_ready = wait_flag(LL_ADC_IsActiveFlag_ADRDY, true);
}

static uint32_t read_raw(uint32_t channel)
{
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, channel);
    LL_ADC_ClearFlag_EOC(ADC1);
    LL_ADC_REG_StartConversion(ADC1);
    if (!wait_flag(LL_ADC_IsActiveFlag_EOC, true)) {
        return 0;
    }
    return LL_ADC_REG_ReadConversionData12(ADC1);
}

static uint32_t read_pin_mv(uint32_t channel)
{
    if (!adc_ready) {
        return 0;
    }
    const uint32_t raw_vrefint = read_raw(LL_ADC_CHANNEL_VREFINT);
    const uint32_t raw_pin = read_raw(channel);

    return policy_pin_mv(raw_pin, raw_vrefint, *VREFINT_CAL_ADDR);
}

bool power_usb_present(void)
{
    return policy_usb_present(read_pin_mv(LL_ADC_CHANNEL_6));
}

uint32_t power_vbat_mv(void)
{
    return policy_vbat_mv(read_pin_mv(LL_ADC_CHANNEL_5));
}
