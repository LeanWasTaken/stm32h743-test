#include <Arduino.h>
#include "sensors.h"

#define TS_CAL1_ADDR     0x1FF1E820UL
#define TS_CAL2_ADDR     0x1FF1E840UL
#define VREFINT_CAL_ADDR 0x1FF1E860UL

static ADC_HandleTypeDef hadc3;

static uint32_t readChannel(uint32_t channel) {
    ADC_ChannelConfTypeDef cfg = {};
    cfg.Channel = channel;
    cfg.Rank = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
    cfg.SingleDiff = ADC_SINGLE_ENDED;
    cfg.OffsetNumber = ADC_OFFSET_NONE;
    HAL_ADC_ConfigChannel(&hadc3, &cfg);
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 100);
    uint32_t val = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    return val;
}

void sensors_init(void) {
    __HAL_RCC_ADC3_CLK_ENABLE();
    MODIFY_REG(RCC->D3CCIPR, RCC_D3CCIPR_ADCSEL, RCC_D3CCIPR_ADCSEL_1);

    hadc3.Instance = ADC3;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV6;
    hadc3.Init.Resolution = ADC_RESOLUTION_16B;
    hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc3.Init.LowPowerAutoWait = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.NbrOfConversion = 1;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc3.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    HAL_ADC_Init(&hadc3);
    HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

uint32_t sensors_readVDDA_mV(void) {
    uint32_t raw = readChannel(ADC_CHANNEL_VREFINT);
    uint16_t cal = *(volatile uint16_t *)VREFINT_CAL_ADDR;
    return raw ? 3300UL * cal / raw : 0;
}

int32_t sensors_readTemp_x10(uint32_t vdda_mv) {
    uint16_t cal1 = *(volatile uint16_t *)TS_CAL1_ADDR;
    uint16_t cal2 = *(volatile uint16_t *)TS_CAL2_ADDR;
    uint32_t raw = readChannel(ADC_CHANNEL_TEMPSENSOR);
    int32_t comp = (int32_t)raw * 3300 / (int32_t)vdda_mv;
    int32_t diff = (int32_t)cal2 - (int32_t)cal1;
    return diff ? 300 + 800 * (comp - (int32_t)cal1) / diff : 0;
}

uint32_t sensors_readVBAT_mV(uint32_t vdda_mv) {
    uint32_t raw = readChannel(ADC_CHANNEL_VBAT);
    return 4UL * vdda_mv * raw / 65535;
}

const char *sensors_decodeResetCause(uint32_t rsr) {
    if (rsr & RCC_RSR_LPWRRSTF)   return "Low Power";
    if (rsr & RCC_RSR_WWDG1RSTF)  return "WWDG";
    if (rsr & RCC_RSR_IWDG1RSTF)  return "IWDG";
    if (rsr & RCC_RSR_SFTRSTF)    return "Software";
    if (rsr & RCC_RSR_PORRSTF)    return "Power-On";
    if (rsr & RCC_RSR_PINRSTF)    return "Pin Reset";
    if (rsr & RCC_RSR_BORRSTF)    return "Brownout";
    return "Unknown";
}
