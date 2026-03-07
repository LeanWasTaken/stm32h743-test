#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "spi_flash.h"
#include "qspi_flash.h"
#include "flash_utils.h"
#include "sensors.h"
#include "boot_anim.h"
#include "ui_pages.h"

static uint8_t currentView = 0;
static bool lastBtnState = false;
static uint32_t lastDebounce = 0;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(BTN_K1, INPUT_PULLDOWN);

    ui.cachedRSR = RCC->RSR;
    RCC->RSR |= RCC_RSR_RMVF;

    display_init();
    boot_animation();

    spi_flash_init();
    spi_flash_readJedec(ui.spiJedec);
    spi_flash_read(0, ui.spiFlashBuf, FLASH_SAMPLE_SIZE);
    ui.spiContent = flash_detectContent(ui.spiFlashBuf, FLASH_SAMPLE_SIZE);

    qspi_flash_init();
    qspi_flash_readJedec(ui.qspiJedec);
    qspi_flash_read(0, ui.qspiFlashBuf, FLASH_SAMPLE_SIZE);
    ui.qspiContent = flash_detectContent(ui.qspiFlashBuf, FLASH_SAMPLE_SIZE);

    sensors_init();

    ui_drawPage(currentView);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    bool btnState = digitalRead(BTN_K1);
    uint32_t now = millis();

    // Short press: next page
    if (btnState && !lastBtnState && (now - lastDebounce > 50)) {
        lastDebounce = now;
        currentView = (currentView + 1) % NUM_PAGES;
        ui_drawPage(currentView);
    }
    lastBtnState = btnState;

    // Live sensor update
    if (currentView == 3 && (now - ui.lastSensorUpdate >= 500)) {
        ui.lastSensorUpdate = now;
        ui_updateSensors();
    }

    delay(10);
}
