#pragma once
#include <stdint.h>

void spi_flash_init(void);
void spi_flash_readJedec(uint8_t *buf);
void spi_flash_read(uint32_t addr, uint8_t *buf, uint16_t len);
void spi_flash_blockErase64K(uint32_t addr);
