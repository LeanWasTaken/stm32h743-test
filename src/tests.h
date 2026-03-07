#pragma once
#include <stdint.h>

#define FLASH_SAMPLE_SIZE  512
#define SPEED_BUF_SIZE     4096
#define SPEED_TEST_BYTES   (256UL * 1024)
#define ERASE_BLOCK_SIZE   65536UL

struct SramRegion {
    const char *name;
    volatile uint32_t *base;
    uint32_t sizeKB;
};

#define NUM_SRAM_REGIONS 4
extern const SramRegion sramRegions[NUM_SRAM_REGIONS];

// Speed test
uint32_t test_benchSpiRead(void);
uint32_t test_benchQspiRead(void);

// Memory test
bool test_sramRegion(volatile uint32_t *base, uint32_t sizeBytes);

// Flash erase with progress display
void test_performSpiErase(uint8_t *jedec, uint8_t *flashBuf, const char **content);
void test_performQspiErase(uint8_t *jedec, uint8_t *flashBuf, const char **content);

// Speed/memory test with progress display
void test_performSpeedTest(uint32_t *spiKBs, uint32_t *qspiKBs);
void test_performMemTest(bool *results, uint32_t *timeMs);
