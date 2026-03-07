#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "spi_flash.h"
#include "qspi_flash.h"
#include "flash_utils.h"
#include "sensors.h"
#include "boot_anim.h"
#include "ui_pages.h"
#include "console.h"
#include "usb_storage.h"

#define HOLD_THRESHOLD_MS  500
#define HOLD_ACTION_MS     5000

static uint8_t currentView = 0;
static bool lastBtnState = false;
static uint32_t lastDebounce = 0;
static uint32_t btnPressStart = 0;
static bool holdActionActive = false;

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
    console_init();

    ui_drawPage(currentView);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    bool btnState = digitalRead(BTN_K1);
    uint32_t now = millis();

    // Button press
    if (btnState && !lastBtnState && (now - lastDebounce > 50)) {
        lastDebounce = now;
        btnPressStart = now;
        holdActionActive = false;
    }

    // Button release
    if (!btnState && lastBtnState) {
        uint32_t holdTime = now - btnPressStart;
        if (holdActionActive) {
            holdActionActive = false;
            ui_drawPage(currentView);
        } else if (holdTime < HOLD_THRESHOLD_MS) {
            currentView = (currentView + 1) % NUM_PAGES;
            ui_drawPage(currentView);
        }
    }

    // Button held on actionable page
    if (btnState && ui_pageHasHoldAction(currentView)) {
        uint32_t holdTime = now - btnPressStart;

        if (holdTime >= HOLD_THRESHOLD_MS && holdTime < HOLD_ACTION_MS) {
            holdActionActive = true;
            display_drawHoldCountdown(holdTime, HOLD_THRESHOLD_MS, HOLD_ACTION_MS);
        }

        if (holdTime >= HOLD_ACTION_MS && holdActionActive) {
            holdActionActive = false;
            ui_executeHoldAction(currentView);
            ui_drawPage(currentView);
        }
    }

    lastBtnState = btnState;

    // Live sensor update
    if (currentView == 3 && (now - ui.lastSensorUpdate >= 500)) {
        ui.lastSensorUpdate = now;
        ui_updateSensors();
    }

    if (!usb_storage_isActive()) {
        console_poll();
    }

    delay(10);
}
