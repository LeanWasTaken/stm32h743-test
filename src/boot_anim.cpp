#include <Arduino.h>
#include "display.h"

void boot_animation(void) {
    tft.fillScreen(ST77XX_BLACK);

    // "H743" — letter by letter, size 2, cyan
    tft.setTextSize(2);
    const int xStart = 20;
    const int yText = 20;
    const char *h743 = "H743";
    tft.setTextColor(ST77XX_CYAN);
    for (int i = 0; i < 4; i++) {
        tft.setCursor(xStart + i * 12, yText);
        tft.print(h743[i]);
        delay(80);
    }

    // " Test" — letter by letter, white
    const char *test = " Test";
    tft.setTextColor(ST77XX_WHITE);
    for (int i = 0; i < 5; i++) {
        tft.setCursor(xStart + 48 + i * 12, yText);
        tft.print(test[i]);
        delay(50);
    }

    delay(80);

    // Expanding line from center
    for (int w = 0; w <= 80; w += 4) {
        tft.drawFastHLine(80 - w, 42, w * 2, ST77XX_CYAN);
        delay(6);
    }

    // Subtitle
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_BLUE);
    tft.setCursor(16, 50);
    tft.print("WeAct STM32H743VIT6");

    delay(500);
}
