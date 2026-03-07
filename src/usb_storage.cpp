#include <Arduino.h>
#include "usb_storage.h"
#include "spi_flash.h"
#include "qspi_flash.h"
#include "flash_utils.h"
#include "ui_pages.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"

// Defined in the Arduino CDC layer (usbd_cdc_if.c)
extern "C" void CDC_deInit(void);

static bool mscActive = false;
static uint8_t sectorBuf[4096];
static USBD_HandleTypeDef hUsbMsc;

// ---- SCSI Inquiry Data (2 LUNs x 36 bytes) ----

static const uint8_t inquiryData[] = {
    // LUN 0: SPI Flash
    0x00, 0x80, 0x02, 0x02, 36 - 4, 0x00, 0x00, 0x00,
    'S', 'T', 'M', '3', '2', ' ', ' ', ' ',
    'S', 'P', 'I', ' ', 'F', 'l', 'a', 's',
    'h', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    '1', '.', '0', '0',
    // LUN 1: QSPI Flash
    0x00, 0x80, 0x02, 0x02, 36 - 4, 0x00, 0x00, 0x00,
    'S', 'T', 'M', '3', '2', ' ', ' ', ' ',
    'Q', 'S', 'P', 'I', ' ', 'F', 'l', 'a',
    's', 'h', ' ', ' ', ' ', ' ', ' ', ' ',
    '1', '.', '0', '0',
};

// ---- Storage Callbacks ----

static int8_t storage_Init(uint8_t lun) { (void)lun; return 0; }

static int8_t storage_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size) {
    *block_size = 512;
    uint8_t *jedec = (lun == 0) ? ui.spiJedec : ui.qspiJedec;
    *block_num = (flash_decodeCapacityKB(jedec[2]) * 1024UL) / 512;
    return 0;
}

static int8_t storage_IsReady(uint8_t lun) { (void)lun; return 0; }
static int8_t storage_IsWriteProtected(uint8_t lun) { (void)lun; return 0; }

static int8_t storage_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
    uint32_t addr = blk_addr * 512;
    uint32_t len = (uint32_t)blk_len * 512;
    while (len > 0) {
        uint16_t chunk = (len > 4096) ? 4096 : (uint16_t)len;
        if (lun == 0) spi_flash_read(addr, buf, chunk);
        else qspi_flash_read(addr, buf, chunk);
        addr += chunk; buf += chunk; len -= chunk;
    }
    return 0;
}

static int8_t storage_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
    uint32_t addr = blk_addr * 512;
    uint32_t len = (uint32_t)blk_len * 512;
    while (len > 0) {
        uint32_t sectorAddr = addr & ~0xFFFUL;
        uint32_t offset = addr - sectorAddr;
        uint32_t bytes = 4096 - offset;
        if (bytes > len) bytes = len;

        if (lun == 0) spi_flash_read(sectorAddr, sectorBuf, 4096);
        else qspi_flash_read(sectorAddr, sectorBuf, 4096);

        memcpy(sectorBuf + offset, buf, bytes);

        if (lun == 0) {
            spi_flash_sectorErase4K(sectorAddr);
            for (uint32_t p = 0; p < 4096; p += 256)
                spi_flash_pageProgram(sectorAddr + p, sectorBuf + p, 256);
        } else {
            qspi_flash_sectorErase4K(sectorAddr);
            for (uint32_t p = 0; p < 4096; p += 256)
                qspi_flash_pageProgram(sectorAddr + p, sectorBuf + p, 256);
        }
        addr += bytes; buf += bytes; len -= bytes;
    }
    return 0;
}

static int8_t storage_GetMaxLun(void) { return 1; }

static USBD_StorageTypeDef storage_fops = {
    storage_Init, storage_GetCapacity, storage_IsReady,
    storage_IsWriteProtected, storage_Read, storage_Write,
    storage_GetMaxLun, (int8_t *)(void *)inquiryData,
};

// ---- Public API ----

bool usb_storage_isActive(void) {
    return mscActive;
}

void usb_storage_enable(void) {
    if (mscActive) return;

    // Tear down Arduino CDC USB
    CDC_deInit();

    // Init fresh USB device with MSC class
    if (USBD_Init(&hUsbMsc, &USBD_Desc, 0) == USBD_OK) {
        if (USBD_RegisterClass(&hUsbMsc, &USBD_MSC) == USBD_OK) {
            if (USBD_MSC_RegisterStorage(&hUsbMsc, &storage_fops) == USBD_OK) {
                USBD_Start(&hUsbMsc);
                mscActive = true;
            }
        }
    }
}

void usb_storage_disable(void) {
    if (!mscActive) return;

    USBD_Stop(&hUsbMsc);
    USBD_DeInit(&hUsbMsc);
    mscActive = false;

    // Re-init CDC (Arduino Serial)
    Serial.begin(115200);
}
