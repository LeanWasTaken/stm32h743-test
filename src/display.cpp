#include <SPI.h>
#include "display.h"
#include "pins.h"

static SPIClass SPI_4(LCD_MOSI, LCD_MISO, LCD_SCK);
WeActST7735 tft = WeActST7735(&SPI_4, LCD_CS, LCD_DC, -1);

void display_init(void) {
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);  // backlight on (active low)

    tft.initR(INITR_MINI160x80);
    tft.setOffsets(26, 1);
    tft.invertDisplay(true);
    tft.setRotation(3);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextWrap(false);
}

void display_drawHeader(const char *title) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_CYAN);
    tft.print(title);
    tft.drawFastHLine(0, 10, 160, ST77XX_BLUE);
}

void display_drawPageIndicator(uint8_t page, uint8_t total) {
    tft.setCursor(130, 72);
    tft.setTextColor(ST77XX_BLUE);
    tft.printf("[%d/%d]", page, total);
}

void display_printLabel(const char *label, uint16_t color) {
    tft.setTextColor(color);
    tft.print(label);
    tft.setTextColor(ST77XX_WHITE);
}

void display_drawHoldHint(void) {
    tft.setCursor(0, 72);
    tft.setTextColor(ST77XX_BLUE);
    tft.print("Hold K1 to start");
}

void display_drawHoldCountdown(uint32_t holdTime, uint32_t thresholdMs, uint32_t actionMs) {
    uint32_t range = actionMs - thresholdMs;
    uint32_t elapsed = holdTime - thresholdMs;
    int barW = constrain((int)(elapsed * 160UL / range), 0, 160);
    tft.fillRect(0, 72, barW, 8, ST77XX_RED);
    tft.fillRect(barW, 72, 160 - barW, 8, ST77XX_BLACK);
}

void display_drawActionProgress(const char *title, uint16_t current, uint16_t total) {
    int barFill = (int)((uint32_t)current * 136 / total);
    int percent = (int)((uint32_t)current * 100 / total);

    if (current == 0) {
        display_drawHeader(title);
        tft.drawRect(10, 30, 140, 14, ST77XX_WHITE);
        tft.setCursor(40, 60);
        tft.setTextColor(ST77XX_YELLOW);
        tft.print("Do not unplug!");
    }

    tft.fillRect(12, 32, barFill, 10, ST77XX_RED);
    tft.fillRect(50, 48, 60, 8, ST77XX_BLACK);
    tft.setCursor(60, 48);
    tft.setTextColor(ST77XX_WHITE);
    tft.printf("%d%%", percent);
}
