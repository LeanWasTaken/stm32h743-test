#pragma once
#include <stdint.h>

void qspi_flash_init(void);
void qspi_flash_readJedec(uint8_t *buf);
void qspi_flash_read(uint32_t addr, uint8_t *buf, uint16_t len);
void qspi_flash_sectorErase4K(uint32_t addr);
void qspi_flash_blockErase64K(uint32_t addr);
void qspi_flash_pageProgram(uint32_t addr, const uint8_t *buf, uint16_t len);
