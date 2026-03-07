#include <Arduino.h>
#include <string.h>
#include "console.h"
#include "ui_pages.h"
#include "flash_utils.h"
#include "sensors.h"
#include "tests.h"
#include "spi_flash.h"
#include "qspi_flash.h"

#define SERIAL_BUF_SIZE 64
static char buf[SERIAL_BUF_SIZE];
static uint8_t pos = 0;

static void printInfo(void) {
    Serial.println("=== STM32H743 Test ===");
    uint32_t idcode = DBGMCU->IDCODE;
    Serial.printf("DEV: 0x%03lX  REV: 0x%04lX\n", idcode & 0xFFF, (idcode >> 16) & 0xFFFF);
    Serial.printf("Flash: %d KB\n", *(volatile uint16_t *)0x1FF1E880UL);
    Serial.printf("SYSCLK: %lu MHz  HCLK: %lu MHz\n",
        HAL_RCC_GetSysClockFreq() / 1000000UL, HAL_RCC_GetHCLKFreq() / 1000000UL);
    uint32_t uid0 = *(volatile uint32_t *)0x1FF1E800UL;
    uint32_t uid1 = *(volatile uint32_t *)0x1FF1E804UL;
    uint32_t uid2 = *(volatile uint32_t *)0x1FF1E808UL;
    Serial.printf("UID: %08lX %08lX %08lX\n", uid0, uid1, uid2);
}

static void printFlash(const char *name, const uint8_t *jedec, const char *content) {
    Serial.printf("--- %s ---\n", name);
    Serial.printf("JEDEC: %02X %02X %02X  Mfr: %s\n",
        jedec[0], jedec[1], jedec[2], flash_decodeMfr(jedec[0]));
    Serial.printf("Size: %lu KB  Content: %s\n",
        flash_decodeCapacityKB(jedec[2]), content);
}

static void printSensors(void) {
    uint32_t vdda = sensors_readVDDA_mV();
    int32_t temp = sensors_readTemp_x10(vdda);
    uint32_t vbat = sensors_readVBAT_mV(vdda);
    Serial.printf("Temp: %ld.%ld C  VDDA: %lu.%02lu V  VBAT: %lu.%02lu V\n",
        temp / 10, temp >= 0 ? temp % 10 : (-temp) % 10,
        vdda / 1000, (vdda % 1000) / 10,
        vbat / 1000, (vbat % 1000) / 10);
    Serial.printf("Reset cause: %s\n", sensors_decodeResetCause(ui.cachedRSR));
}

static void processCommand(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        Serial.println("Commands: info, flash, sensors, cpu, speedtest, memtest, erase spi, erase qspi");
    } else if (strcmp(cmd, "info") == 0) {
        printInfo();
    } else if (strcmp(cmd, "flash") == 0) {
        printFlash("SPI NOR Flash", ui.spiJedec, ui.spiContent);
        printFlash("QSPI NOR Flash", ui.qspiJedec, ui.qspiContent);
    } else if (strcmp(cmd, "sensors") == 0) {
        printSensors();
    } else if (strcmp(cmd, "cpu") == 0) {
        uint32_t cpuid = SCB->CPUID;
        Serial.printf("CPU: Cortex-M7 r%dp%d\n", (cpuid >> 20) & 0xF, cpuid & 0xF);
        Serial.printf("Cache: I:%s D:%s\n",
            (SCB->CCR & SCB_CCR_IC_Msk) ? "ON" : "off",
            (SCB->CCR & SCB_CCR_DC_Msk) ? "ON" : "off");
        Serial.printf("Flash: %lu wait states\n", FLASH->ACR & 0xF);
    } else if (strcmp(cmd, "speedtest") == 0) {
        Serial.println("Running flash speed test...");
        ui.spiSpeedKBs = test_benchSpiRead();
        ui.qspiSpeedKBs = test_benchQspiRead();
        Serial.printf("SPI read:  %lu KB/s\n", ui.spiSpeedKBs);
        Serial.printf("QSPI read: %lu KB/s\n", ui.qspiSpeedKBs);
    } else if (strcmp(cmd, "memtest") == 0) {
        Serial.println("Running memory test...");
        uint32_t t0 = millis();
        for (int i = 0; i < NUM_SRAM_REGIONS; i++) {
            ui.memTestPass[i] = test_sramRegion(sramRegions[i].base, sramRegions[i].sizeKB * 1024);
            Serial.printf("  %s (%luKB): %s\n", sramRegions[i].name,
                sramRegions[i].sizeKB, ui.memTestPass[i] ? "PASS" : "FAIL");
        }
        ui.memTestTimeMs = millis() - t0;
        ui.memTestRan = true;
        Serial.printf("Done in %lu ms\n", ui.memTestTimeMs);
    } else if (strcmp(cmd, "erase spi") == 0) {
        Serial.println("Erasing SPI flash...");
        uint32_t totalSize = flash_decodeCapacityKB(ui.spiJedec[2]) * 1024UL;
        uint16_t numBlocks = totalSize / ERASE_BLOCK_SIZE;
        for (uint16_t i = 0; i < numBlocks; i++) {
            spi_flash_blockErase64K((uint32_t)i * ERASE_BLOCK_SIZE);
            if ((i % 16) == 15) Serial.printf("  %d/%d blocks\n", i + 1, numBlocks);
        }
        spi_flash_read(0, ui.spiFlashBuf, FLASH_SAMPLE_SIZE);
        ui.spiContent = flash_detectContent(ui.spiFlashBuf, FLASH_SAMPLE_SIZE);
        Serial.println("SPI flash erased.");
    } else if (strcmp(cmd, "erase qspi") == 0) {
        Serial.println("Erasing QSPI flash...");
        uint32_t totalSize = flash_decodeCapacityKB(ui.qspiJedec[2]) * 1024UL;
        uint16_t numBlocks = totalSize / ERASE_BLOCK_SIZE;
        for (uint16_t i = 0; i < numBlocks; i++) {
            qspi_flash_blockErase64K((uint32_t)i * ERASE_BLOCK_SIZE);
            if ((i % 16) == 15) Serial.printf("  %d/%d blocks\n", i + 1, numBlocks);
        }
        qspi_flash_read(0, ui.qspiFlashBuf, FLASH_SAMPLE_SIZE);
        ui.qspiContent = flash_detectContent(ui.qspiFlashBuf, FLASH_SAMPLE_SIZE);
        Serial.println("QSPI flash erased.");
    } else {
        Serial.printf("Unknown command: %s\n", cmd);
        Serial.println("Type 'help' for commands.");
    }
}

void console_init(void) {
    Serial.begin(115200);
}

void console_poll(void) {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (pos > 0) {
                buf[pos] = '\0';
                processCommand(buf);
                pos = 0;
            }
        } else if (pos < SERIAL_BUF_SIZE - 1) {
            buf[pos++] = c;
        }
    }
}
