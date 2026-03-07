#pragma once
#include <stdint.h>

const char *flash_decodeMfr(uint8_t id);
uint32_t flash_decodeCapacityKB(uint8_t cap);
const char *flash_detectContent(const uint8_t *buf, uint16_t len);
