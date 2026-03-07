#include <Arduino.h>
#include "qspi_flash.h"
#include "pins.h"

static QSPI_HandleTypeDef hqspi;

static void writeEnable(void) {
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x06;
    HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static void waitReady(void) {
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x05;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 1;
    uint8_t sr;
    do {
        if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) break;
        HAL_QSPI_Receive(&hqspi, &sr, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    } while (sr & 0x01);
}

static void qspiPeriphInit(uint8_t flashSize) {
    hqspi.Instance = QUADSPI;
    hqspi.Init.ClockPrescaler = 2;  // HCLK/3 = 80MHz
    hqspi.Init.FifoThreshold = 1;
    hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize = flashSize;
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
    HAL_QSPI_DeInit(&hqspi);
    HAL_QSPI_Init(&hqspi);
}

static void enableQuadMode(void) {
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x35;  // Read Status Register 2
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 1;
    uint8_t sr2 = 0;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK)
        HAL_QSPI_Receive(&hqspi, &sr2, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    if (sr2 & 0x02) return;  // QE already set

    writeEnable();
    sr2 |= 0x02;
    cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x31;  // Write Status Register 2
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 1;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK)
        HAL_QSPI_Transmit(&hqspi, &sr2, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    waitReady();
}

void qspi_flash_init(void) {
    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = GPIO_PIN_2; gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_6; gpio.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &gpio);
    gpio.Pin = GPIO_PIN_2; gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOE, &gpio);

    // Init with max flash size to read JEDEC
    qspiPeriphInit(25);
}

void qspi_flash_readJedec(uint8_t *buf) {
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x9F;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 3;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK)
        HAL_QSPI_Receive(&hqspi, buf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    // Re-init with correct flash size and enable quad mode
    if (buf[2] >= 0x10 && buf[2] <= 0x21)
        qspiPeriphInit(buf[2] - 1);
    enableQuadMode();
}

void qspi_flash_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x6B;  // Quad Output Fast Read
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = addr;
    cmd.DummyCycles = 8;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.NbData = len;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK)
        HAL_QSPI_Receive(&hqspi, buf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

void qspi_flash_sectorErase4K(uint32_t addr) {
    writeEnable();
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x20;  // Sector Erase 4KB
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = addr;
    HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    waitReady();
}

void qspi_flash_pageProgram(uint32_t addr, const uint8_t *buf, uint16_t len) {
    if (len > 256) len = 256;
    writeEnable();
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x02;  // Page Program
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = addr;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = len;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK)
        HAL_QSPI_Transmit(&hqspi, (uint8_t *)buf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    waitReady();
}

void qspi_flash_blockErase64K(uint32_t addr) {
    writeEnable();
    QSPI_CommandTypeDef cmd = {};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0xD8;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = addr;
    HAL_QSPI_Command(&hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    waitReady();
}
