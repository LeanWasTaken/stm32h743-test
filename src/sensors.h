#pragma once
#include <stdint.h>

void sensors_init(void);
uint32_t sensors_readVDDA_mV(void);
int32_t sensors_readTemp_x10(uint32_t vdda_mv);
uint32_t sensors_readVBAT_mV(uint32_t vdda_mv);
const char *sensors_decodeResetCause(uint32_t rsr);
