#pragma once
#include <stdint.h>
#include "tests.h"

#define NUM_PAGES       8

struct UiState {
    uint8_t spiJedec[3];
    uint8_t qspiJedec[3];
    uint8_t spiFlashBuf[FLASH_SAMPLE_SIZE];
    uint8_t qspiFlashBuf[FLASH_SAMPLE_SIZE];
    const char *spiContent;
    const char *qspiContent;
    uint32_t cachedRSR;
    uint32_t lastSensorUpdate;
    // Speed test results (0 = not run)
    uint32_t spiSpeedKBs;
    uint32_t qspiSpeedKBs;
    // Memory test results
    bool memTestRan;
    bool memTestPass[NUM_SRAM_REGIONS];
    uint32_t memTestTimeMs;
};

extern UiState ui;

void ui_drawPage(uint8_t page);
void ui_updateSensors(void);
bool ui_pageHasHoldAction(uint8_t page);
void ui_executeHoldAction(uint8_t page);
