#pragma once
#include <Adafruit_ST7735.h>

class WeActST7735 : public Adafruit_ST7735 {
public:
    using Adafruit_ST7735::Adafruit_ST7735;
    void setOffsets(uint8_t col, uint8_t row) {
        _colstart = col;
        _rowstart = row;
    }
};

extern WeActST7735 tft;

void display_init(void);
void display_drawHeader(const char *title);
void display_drawPageIndicator(uint8_t page, uint8_t total);
void display_printLabel(const char *label, uint16_t color);
void display_drawHoldHint(void);
void display_drawHoldCountdown(uint32_t holdTime, uint32_t thresholdMs, uint32_t actionMs);
void display_drawActionProgress(const char *title, uint16_t current, uint16_t total);
