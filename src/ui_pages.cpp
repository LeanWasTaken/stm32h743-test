#include <Arduino.h>
#include "ui_pages.h"
#include "display.h"
#include "flash_utils.h"
#include "sensors.h"
#include "tests.h"
#include "usb_storage.h"

UiState ui;

// Page 1: MCU Info
static void drawChipInfo(void) {
    display_drawHeader("STM32H743VIT6");

    uint32_t idcode = DBGMCU->IDCODE;
    tft.setCursor(0, 14);
    display_printLabel("DEV:", ST77XX_YELLOW);
    tft.printf("0x%03lX ", idcode & 0xFFF);
    display_printLabel("REV:", ST77XX_YELLOW);
    tft.printf("0x%04lX", (idcode >> 16) & 0xFFFF);

    tft.setCursor(0, 24);
    display_printLabel("Flash: ", ST77XX_YELLOW);
    tft.printf("%d KB", *(volatile uint16_t *)0x1FF1E880UL);

    tft.setCursor(0, 34);
    display_printLabel("SYSCLK: ", ST77XX_YELLOW);
    tft.printf("%lu MHz", HAL_RCC_GetSysClockFreq() / 1000000UL);

    tft.setCursor(0, 44);
    display_printLabel("HCLK:   ", ST77XX_YELLOW);
    tft.printf("%lu MHz", HAL_RCC_GetHCLKFreq() / 1000000UL);

    uint32_t uid0 = *(volatile uint32_t *)0x1FF1E800UL;
    uint32_t uid1 = *(volatile uint32_t *)0x1FF1E804UL;
    uint32_t uid2 = *(volatile uint32_t *)0x1FF1E808UL;
    tft.setCursor(0, 54);
    display_printLabel("UID:", ST77XX_GREEN);
    tft.printf("%08lX", uid0);
    tft.setCursor(0, 64);
    tft.printf(" %08lX%08lX", uid1, uid2);

    display_drawPageIndicator(1, NUM_PAGES);
}

// Pages 2-3: Flash chips
static void drawFlashPage(const char *title, const uint8_t *jedec,
                           const char *busInfo, const char *content,
                           const uint8_t *rawData, uint8_t page) {
    display_drawHeader(title);

    tft.setCursor(0, 14);
    display_printLabel("JEDEC: ", ST77XX_YELLOW);
    tft.printf("%02X %02X %02X", jedec[0], jedec[1], jedec[2]);

    tft.setCursor(0, 24);
    display_printLabel("Mfr:   ", ST77XX_YELLOW);
    tft.printf("%s ", flash_decodeMfr(jedec[0]));
    uint32_t capKB = flash_decodeCapacityKB(jedec[2]);
    if (capKB >= 1024) tft.printf("%luMB", capKB / 1024);
    else if (capKB > 0) tft.printf("%luKB", capKB);

    tft.setCursor(0, 34);
    display_printLabel("Bus:   ", ST77XX_YELLOW);
    tft.print(busInfo);

    tft.setCursor(0, 44);
    display_printLabel("Data:  ", ST77XX_GREEN);
    tft.print(content);

    tft.setCursor(0, 54);
    display_printLabel("@0x00: ", ST77XX_YELLOW);
    for (int i = 0; i < 8; i++) {
        tft.printf("%02X", rawData[i]);
        if (i < 7) tft.print(" ");
    }

    tft.setCursor(0, 64);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" ");
    for (int i = 8; i < 16; i++) {
        tft.printf("%02X", rawData[i]);
        if (i < 15) tft.print(" ");
    }

    display_drawPageIndicator(page, NUM_PAGES);
}

// Page 4: Sensors
static void drawSensorsLabels(void) {
    display_drawHeader("Sensors");
    tft.setCursor(0, 14); display_printLabel("Temp:  ", ST77XX_YELLOW);
    tft.setCursor(0, 24); display_printLabel("VDDA:  ", ST77XX_YELLOW);
    tft.setCursor(0, 34); display_printLabel("VBAT:  ", ST77XX_YELLOW);
    tft.setCursor(0, 54); display_printLabel("Reset: ", ST77XX_YELLOW);
    tft.print(sensors_decodeResetCause(ui.cachedRSR));
    display_drawPageIndicator(4, NUM_PAGES);
}

// Page 5: CPU & Power
static void drawCpuPower(void) {
    display_drawHeader("CPU & Power");

    uint32_t cpuid = SCB->CPUID;
    tft.setCursor(0, 14); display_printLabel("CPU:   ", ST77XX_YELLOW);
    tft.printf("CM7 r%dp%d", (cpuid >> 20) & 0xF, cpuid & 0xF);

    tft.setCursor(0, 24); display_printLabel("Cache: ", ST77XX_YELLOW);
    tft.printf("I:%s D:%s",
        (SCB->CCR & SCB_CCR_IC_Msk) ? "ON" : "off",
        (SCB->CCR & SCB_CCR_DC_Msk) ? "ON" : "off");

    uint32_t cr3 = PWR->CR3;
    tft.setCursor(0, 34); display_printLabel("Power: ", ST77XX_YELLOW);
    if (cr3 & PWR_CR3_LDOEN) tft.print("LDO");
    else if (cr3 & PWR_CR3_BYPASS) tft.print("Bypass");
    else tft.print("???");
    uint32_t vos = (PWR->D3CR >> 14) & 0x3;
    tft.printf(" VOS%lu", (vos == 3) ? 1UL : (vos == 2) ? 2UL : 3UL);

    tft.setCursor(0, 44); display_printLabel("Flash: ", ST77XX_YELLOW);
    tft.printf("%lu wait states", FLASH->ACR & 0xF);

    uint32_t pllsrc = RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC;
    tft.setCursor(0, 54); display_printLabel("PLL:   ", ST77XX_YELLOW);
    switch (pllsrc) {
        case 0: tft.print("HSI"); break;
        case 1: tft.print("CSI"); break;
        case 2: tft.print("HSE"); break;
        default: tft.print("None"); break;
    }

    tft.setCursor(0, 64); display_printLabel("APB:", ST77XX_YELLOW);
    tft.printf("%lu/%lu MHz",
        HAL_RCC_GetPCLK1Freq() / 1000000UL, HAL_RCC_GetPCLK2Freq() / 1000000UL);

    display_drawPageIndicator(5, NUM_PAGES);
}

// Page 6: Flash Speed Test
static void drawSpeedTest(void) {
    display_drawHeader("Flash Speed Test");
    tft.setCursor(0, 14);
    display_printLabel("Test:  ", ST77XX_YELLOW);
    tft.print("256KB seq. read");

    tft.setCursor(0, 30);
    display_printLabel("SPI:   ", ST77XX_GREEN);
    if (ui.spiSpeedKBs > 0) tft.printf("%lu KB/s", ui.spiSpeedKBs);
    else tft.print("--");

    tft.setCursor(0, 40);
    display_printLabel("QSPI:  ", ST77XX_MAGENTA);
    if (ui.qspiSpeedKBs > 0) tft.printf("%lu KB/s", ui.qspiSpeedKBs);
    else tft.print("--");

    if (ui.spiSpeedKBs == 0 && ui.qspiSpeedKBs == 0)
        display_drawHoldHint();

    display_drawPageIndicator(6, NUM_PAGES);
}

// Page 7: Memory Test
static void drawMemTest(void) {
    display_drawHeader("Memory Test");

    if (!ui.memTestRan) {
        tft.setCursor(0, 14);
        tft.setTextColor(ST77XX_WHITE);
        tft.print("Tests SRAM1-4 with");
        tft.setCursor(0, 24);
        tft.print("0x55/0xAA/addr patterns");
        tft.setCursor(0, 40);
        display_printLabel("Total: ", ST77XX_YELLOW);
        tft.print("352 KB");
        display_drawHoldHint();
    } else {
        for (int i = 0; i < NUM_SRAM_REGIONS; i++) {
            int y = 14 + i * 14;
            tft.setCursor(0, y);
            display_printLabel(sramRegions[i].name, ST77XX_YELLOW);
            tft.printf(" %3luKB ", sramRegions[i].sizeKB);
            if (ui.memTestPass[i]) {
                tft.setTextColor(ST77XX_GREEN); tft.print("PASS");
            } else {
                tft.setTextColor(ST77XX_RED); tft.print("FAIL");
            }
        }
        tft.setCursor(0, 70);
        tft.setTextColor(ST77XX_BLUE);
        tft.printf("%lu ms", ui.memTestTimeMs);
    }

    display_drawPageIndicator(7, NUM_PAGES);
}

// Page 8: USB Mass Storage
static void drawUsbStorage(void) {
    display_drawHeader("USB Mass Storage");

    if (usb_storage_isActive()) {
        tft.setCursor(0, 14);
        tft.setTextColor(ST77XX_GREEN);
        tft.print("MSC ACTIVE");

        tft.setCursor(0, 28);
        tft.setTextColor(ST77XX_WHITE);
        tft.print("LUN0: SPI  ");
        tft.printf("%luMB", flash_decodeCapacityKB(ui.spiJedec[2]) / 1024);

        tft.setCursor(0, 38);
        tft.print("LUN1: QSPI ");
        tft.printf("%luMB", flash_decodeCapacityKB(ui.qspiJedec[2]) / 1024);

        tft.setCursor(0, 54);
        tft.setTextColor(ST77XX_YELLOW);
        tft.print("Serial disabled.");
        tft.setCursor(0, 72);
        tft.setTextColor(ST77XX_BLUE);
        tft.print("Hold K1 to stop");
    } else {
        tft.setCursor(0, 14);
        tft.setTextColor(ST77XX_WHITE);
        tft.print("Expose NOR flash as");
        tft.setCursor(0, 24);
        tft.print("USB removable drives.");

        tft.setCursor(0, 40);
        tft.setTextColor(ST77XX_RED);
        tft.print("WARNING: This will");
        tft.setCursor(0, 50);
        tft.print("disable serial!");

        display_drawHoldHint();
    }

    display_drawPageIndicator(8, NUM_PAGES);
}

void ui_drawPage(uint8_t page) {
    switch (page) {
        case 0: drawChipInfo(); break;
        case 1: drawFlashPage("SPI NOR Flash", ui.spiJedec, "SPI1",
                               ui.spiContent, ui.spiFlashBuf, 2); break;
        case 2: drawFlashPage("QSPI NOR Flash", ui.qspiJedec, "QUADSPI",
                               ui.qspiContent, ui.qspiFlashBuf, 3); break;
        case 3:
            drawSensorsLabels();
            ui_updateSensors();
            ui.lastSensorUpdate = millis();
            break;
        case 4: drawCpuPower(); break;
        case 5: drawSpeedTest(); break;
        case 6: drawMemTest(); break;
        case 7: drawUsbStorage(); break;
    }
}

bool ui_pageHasHoldAction(uint8_t page) {
    return page == 1 || page == 2 || page == 5 || page == 6 || page == 7;
}

void ui_executeHoldAction(uint8_t page) {
    switch (page) {
        case 1: test_performSpiErase(ui.spiJedec, ui.spiFlashBuf, &ui.spiContent); break;
        case 2: test_performQspiErase(ui.qspiJedec, ui.qspiFlashBuf, &ui.qspiContent); break;
        case 5: test_performSpeedTest(&ui.spiSpeedKBs, &ui.qspiSpeedKBs); break;
        case 6:
            test_performMemTest(ui.memTestPass, &ui.memTestTimeMs);
            ui.memTestRan = true;
            break;
        case 7:
            if (usb_storage_isActive()) usb_storage_disable();
            else usb_storage_enable();
            break;
    }
}

void ui_updateSensors(void) {
    uint32_t vdda = sensors_readVDDA_mV();
    int32_t temp = sensors_readTemp_x10(vdda);
    uint32_t vbat = sensors_readVBAT_mV(vdda);

    tft.fillRect(42, 14, 88, 8, ST77XX_BLACK);
    tft.fillRect(42, 24, 88, 8, ST77XX_BLACK);
    tft.fillRect(42, 34, 88, 8, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);

    tft.setCursor(42, 14);
    if (temp >= 0) tft.printf("%ld.%ld C", temp / 10, temp % 10);
    else { int32_t a = -temp; tft.printf("-%ld.%ld C", a / 10, a % 10); }

    tft.setCursor(42, 24);
    tft.printf("%lu.%02lu V", vdda / 1000, (vdda % 1000) / 10);
    tft.setCursor(42, 34);
    tft.printf("%lu.%02lu V", vbat / 1000, (vbat % 1000) / 10);
}
