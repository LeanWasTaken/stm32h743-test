#include <Arduino.h>
#include "pins.h"
#include "display.h"

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    display_init();

    display_drawHeader("STM32H743 Test");
    tft.setCursor(0, 30);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Display OK");

    digitalWrite(LED_PIN, LOW);
}

void loop() {
}
