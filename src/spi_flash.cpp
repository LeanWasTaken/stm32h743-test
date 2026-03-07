#include <Arduino.h>
#include <SPI.h>
#include "spi_flash.h"
#include "pins.h"

static SPIClass SPI_1(FLASH_SPI_MOSI, FLASH_SPI_MISO, FLASH_SPI_SCK);

static void writeEnable(void) {
    SPI_1.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0x06);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
}

static void waitReady(void) {
    SPI_1.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0x05);
    while (SPI_1.transfer(0) & 0x01) {}
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
}

void spi_flash_init(void) {
    pinMode(FLASH_SPI_CS, OUTPUT);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.begin();
}

void spi_flash_readJedec(uint8_t *buf) {
    SPI_1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0x9F);
    buf[0] = SPI_1.transfer(0);
    buf[1] = SPI_1.transfer(0);
    buf[2] = SPI_1.transfer(0);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
}

void spi_flash_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    SPI_1.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0x0B);  // Fast Read
    SPI_1.transfer((addr >> 16) & 0xFF);
    SPI_1.transfer((addr >> 8) & 0xFF);
    SPI_1.transfer(addr & 0xFF);
    SPI_1.transfer(0);  // dummy byte
    for (uint16_t i = 0; i < len; i++) buf[i] = SPI_1.transfer(0);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
}

void spi_flash_sectorErase4K(uint32_t addr) {
    writeEnable();
    SPI_1.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0x20);  // Sector Erase 4KB
    SPI_1.transfer((addr >> 16) & 0xFF);
    SPI_1.transfer((addr >> 8) & 0xFF);
    SPI_1.transfer(addr & 0xFF);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
    waitReady();
}

void spi_flash_pageProgram(uint32_t addr, const uint8_t *buf, uint16_t len) {
    if (len > 256) len = 256;
    writeEnable();
    SPI_1.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0x02);  // Page Program
    SPI_1.transfer((addr >> 16) & 0xFF);
    SPI_1.transfer((addr >> 8) & 0xFF);
    SPI_1.transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) SPI_1.transfer(buf[i]);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
    waitReady();
}

void spi_flash_blockErase64K(uint32_t addr) {
    writeEnable();
    SPI_1.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
    digitalWrite(FLASH_SPI_CS, LOW);
    SPI_1.transfer(0xD8);
    SPI_1.transfer((addr >> 16) & 0xFF);
    SPI_1.transfer((addr >> 8) & 0xFF);
    SPI_1.transfer(addr & 0xFF);
    digitalWrite(FLASH_SPI_CS, HIGH);
    SPI_1.endTransaction();
    waitReady();
}
