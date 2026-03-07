#pragma once
#include <stdint.h>

#define NUM_PAGES       5
#define FLASH_SAMPLE_SIZE 512

// Shared state for UI pages
struct UiState {
    uint8_t spiJedec[3];
    uint8_t qspiJedec[3];
    uint8_t spiFlashBuf[FLASH_SAMPLE_SIZE];
    uint8_t qspiFlashBuf[FLASH_SAMPLE_SIZE];
    const char *spiContent;
    const char *qspiContent;
    uint32_t cachedRSR;
    uint32_t lastSensorUpdate;
};

extern UiState ui;

void ui_drawPage(uint8_t page);
void ui_updateSensors(void);
