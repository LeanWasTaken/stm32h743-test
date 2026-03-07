#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "spi_flash.h"
#include "qspi_flash.h"
#include "flash_utils.h"
#include "sensors.h"
#include "boot_anim.h"

#define FLASH_SAMPLE_SIZE 512

static uint8_t spiJedec[3];
static uint8_t qspiJedec[3];
static uint8_t spiFlashBuf[FLASH_SAMPLE_SIZE];
static uint8_t qspiFlashBuf[FLASH_SAMPLE_SIZE];
static const char *spiContent;
static const char *qspiContent;
static uint32_t cachedRSR;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    cachedRSR = RCC->RSR;
    RCC->RSR |= RCC_RSR_RMVF;

    display_init();
    boot_animation();

    spi_flash_init();
    spi_flash_readJedec(spiJedec);
    spi_flash_read(0, spiFlashBuf, FLASH_SAMPLE_SIZE);
    spiContent = flash_detectContent(spiFlashBuf, FLASH_SAMPLE_SIZE);

    qspi_flash_init();
    qspi_flash_readJedec(qspiJedec);
    qspi_flash_read(0, qspiFlashBuf, FLASH_SAMPLE_SIZE);
    qspiContent = flash_detectContent(qspiFlashBuf, FLASH_SAMPLE_SIZE);

    sensors_init();

    display_drawHeader("STM32H743 Test");

    uint32_t vdda = sensors_readVDDA_mV();
    int32_t temp = sensors_readTemp_x10(vdda);

    tft.setCursor(0, 14);
    display_printLabel("Temp:  ", ST77XX_YELLOW);
    tft.printf("%ld.%ld C", temp / 10, temp % 10);

    tft.setCursor(0, 24);
    display_printLabel("VDDA:  ", ST77XX_YELLOW);
    tft.printf("%lu.%02lu V", vdda / 1000, (vdda % 1000) / 10);

    tft.setCursor(0, 34);
    display_printLabel("Reset: ", ST77XX_YELLOW);
    tft.print(sensors_decodeResetCause(cachedRSR));

    tft.setCursor(0, 48);
    display_printLabel("SPI:  ", ST77XX_GREEN);
    tft.print(spiContent);

    tft.setCursor(0, 58);
    display_printLabel("QSPI: ", ST77XX_MAGENTA);
    tft.print(qspiContent);

    digitalWrite(LED_PIN, LOW);
}

void loop() {
}
