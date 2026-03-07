#include <Arduino.h>
#include "tests.h"
#include "display.h"
#include "spi_flash.h"
#include "qspi_flash.h"
#include "flash_utils.h"

const SramRegion sramRegions[NUM_SRAM_REGIONS] = {
    {"SRAM1", (volatile uint32_t *)0x30000000UL, 128},
    {"SRAM2", (volatile uint32_t *)0x30020000UL, 128},
    {"SRAM3", (volatile uint32_t *)0x30040000UL,  32},
    {"SRAM4", (volatile uint32_t *)0x38000000UL,  64},
};

static uint8_t speedBuf[SPEED_BUF_SIZE];

uint32_t test_benchSpiRead(void) {
    uint32_t iterations = SPEED_TEST_BYTES / SPEED_BUF_SIZE;
    uint32_t t0 = micros();
    for (uint32_t i = 0; i < iterations; i++)
        spi_flash_read(i * SPEED_BUF_SIZE, speedBuf, SPEED_BUF_SIZE);
    uint32_t elapsed = micros() - t0;
    return elapsed ? (SPEED_TEST_BYTES * 1000UL) / elapsed : 0;
}

uint32_t test_benchQspiRead(void) {
    uint32_t iterations = SPEED_TEST_BYTES / SPEED_BUF_SIZE;
    uint32_t t0 = micros();
    for (uint32_t i = 0; i < iterations; i++)
        qspi_flash_read(i * SPEED_BUF_SIZE, speedBuf, SPEED_BUF_SIZE);
    uint32_t elapsed = micros() - t0;
    return elapsed ? (SPEED_TEST_BYTES * 1000UL) / elapsed : 0;
}

bool test_sramRegion(volatile uint32_t *base, uint32_t sizeBytes) {
    uint32_t words = sizeBytes / 4;

    for (uint32_t i = 0; i < words; i++) base[i] = 0x55555555UL;
    for (uint32_t i = 0; i < words; i++)
        if (base[i] != 0x55555555UL) return false;

    for (uint32_t i = 0; i < words; i++) base[i] = 0xAAAAAAAAUL;
    for (uint32_t i = 0; i < words; i++)
        if (base[i] != 0xAAAAAAAAUL) return false;

    for (uint32_t i = 0; i < words; i++) base[i] = (uint32_t)&base[i];
    for (uint32_t i = 0; i < words; i++)
        if (base[i] != (uint32_t)&base[i]) return false;

    for (uint32_t i = 0; i < words; i++) base[i] = 0;
    return true;
}

void test_performSpiErase(uint8_t *jedec, uint8_t *flashBuf, const char **content) {
    uint32_t totalSize = flash_decodeCapacityKB(jedec[2]) * 1024UL;
    uint16_t numBlocks = totalSize / ERASE_BLOCK_SIZE;
    if (numBlocks == 0) numBlocks = 1;
    display_drawActionProgress("Erasing SPI Flash", 0, numBlocks);
    for (uint16_t i = 0; i < numBlocks; i++) {
        spi_flash_blockErase64K((uint32_t)i * ERASE_BLOCK_SIZE);
        display_drawActionProgress("Erasing SPI Flash", i + 1, numBlocks);
    }
    spi_flash_read(0, flashBuf, FLASH_SAMPLE_SIZE);
    *content = flash_detectContent(flashBuf, FLASH_SAMPLE_SIZE);
    display_drawHeader("SPI Flash Erased");
    tft.setCursor(40, 36); tft.setTextColor(ST77XX_GREEN); tft.print("Complete!");
    delay(1500);
}

void test_performQspiErase(uint8_t *jedec, uint8_t *flashBuf, const char **content) {
    uint32_t totalSize = flash_decodeCapacityKB(jedec[2]) * 1024UL;
    uint16_t numBlocks = totalSize / ERASE_BLOCK_SIZE;
    if (numBlocks == 0) numBlocks = 1;
    display_drawActionProgress("Erasing QSPI Flash", 0, numBlocks);
    for (uint16_t i = 0; i < numBlocks; i++) {
        qspi_flash_blockErase64K((uint32_t)i * ERASE_BLOCK_SIZE);
        display_drawActionProgress("Erasing QSPI Flash", i + 1, numBlocks);
    }
    qspi_flash_read(0, flashBuf, FLASH_SAMPLE_SIZE);
    *content = flash_detectContent(flashBuf, FLASH_SAMPLE_SIZE);
    display_drawHeader("QSPI Flash Erased");
    tft.setCursor(40, 36); tft.setTextColor(ST77XX_GREEN); tft.print("Complete!");
    delay(1500);
}

void test_performSpeedTest(uint32_t *spiKBs, uint32_t *qspiKBs) {
    display_drawHeader("Testing...");
    tft.setCursor(0, 24); display_printLabel("SPI:  ", ST77XX_GREEN);
    tft.print("reading 256KB...");
    *spiKBs = test_benchSpiRead();
    tft.setCursor(0, 40); display_printLabel("QSPI: ", ST77XX_MAGENTA);
    tft.print("reading 256KB...");
    *qspiKBs = test_benchQspiRead();
    delay(500);
}

void test_performMemTest(bool *results, uint32_t *timeMs) {
    display_drawHeader("Testing Memory...");
    uint32_t t0 = millis();
    for (int i = 0; i < NUM_SRAM_REGIONS; i++) {
        int y = 20 + i * 14;
        tft.setCursor(0, y); display_printLabel(sramRegions[i].name, ST77XX_YELLOW);
        tft.print("...");
        results[i] = test_sramRegion(sramRegions[i].base, sramRegions[i].sizeKB * 1024);
        tft.fillRect(60, y, 80, 8, ST77XX_BLACK);
        tft.setCursor(60, y);
        if (results[i]) { tft.setTextColor(ST77XX_GREEN); tft.print("PASS"); }
        else { tft.setTextColor(ST77XX_RED); tft.print("FAIL"); }
    }
    *timeMs = millis() - t0;
    delay(800);
}
