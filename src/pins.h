#pragma once

// LCD (SPI4)
#define LCD_CS   PE11
#define LCD_DC   PE13
#define LCD_BL   PE10
#define LCD_MOSI PE14
#define LCD_MISO PE5
#define LCD_SCK  PE12

// SPI NOR flash (SPI1)
#define FLASH_SPI_MOSI PD7
#define FLASH_SPI_MISO PB4
#define FLASH_SPI_SCK  PB3
#define FLASH_SPI_CS   PD6

// QSPI NOR flash
#define QSPI_CLK PB2
#define QSPI_NCS PB6
#define QSPI_IO0 PD11
#define QSPI_IO1 PD12
#define QSPI_IO2 PE2
#define QSPI_IO3 PD13

// User interface
#define BTN_K1   PC13
#define LED_PIN  PE3
