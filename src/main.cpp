#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "spi_flash.h"
#include "qspi_flash.h"
#include "flash_utils.h"

#define FLASH_SAMPLE_SIZE 512

static uint8_t spiJedec[3];
static uint8_t qspiJedec[3];
static uint8_t spiFlashBuf[FLASH_SAMPLE_SIZE];
static uint8_t qspiFlashBuf[FLASH_SAMPLE_SIZE];
static const char *spiContent;
static const char *qspiContent;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    display_init();

    spi_flash_init();
    spi_flash_readJedec(spiJedec);
    spi_flash_read(0, spiFlashBuf, FLASH_SAMPLE_SIZE);
    spiContent = flash_detectContent(spiFlashBuf, FLASH_SAMPLE_SIZE);

    qspi_flash_init();
    qspi_flash_readJedec(qspiJedec);
    qspi_flash_read(0, qspiFlashBuf, FLASH_SAMPLE_SIZE);
    qspiContent = flash_detectContent(qspiFlashBuf, FLASH_SAMPLE_SIZE);

    display_drawHeader("STM32H743 Test");
    tft.setCursor(0, 14);
    display_printLabel("SPI:  ", ST77XX_GREEN);
    tft.printf("%s %luKB", flash_decodeMfr(spiJedec[0]),
               flash_decodeCapacityKB(spiJedec[2]));
    tft.setCursor(0, 24);
    display_printLabel("      ", ST77XX_WHITE);
    tft.print(spiContent);

    tft.setCursor(0, 38);
    display_printLabel("QSPI: ", ST77XX_MAGENTA);
    tft.printf("%s %luKB", flash_decodeMfr(qspiJedec[0]),
               flash_decodeCapacityKB(qspiJedec[2]));
    tft.setCursor(0, 48);
    display_printLabel("      ", ST77XX_WHITE);
    tft.print(qspiContent);

    digitalWrite(LED_PIN, LOW);
}

void loop() {
}
