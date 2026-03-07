#include "flash_utils.h"

const char *flash_decodeMfr(uint8_t id) {
    switch (id) {
        case 0xEF: return "Winbond";
        case 0xC8: return "GigaDevice";
        case 0x20: return "Micron";
        case 0x01: return "Spansion";
        case 0xBF: return "SST";
        case 0x1F: return "Atmel";
        case 0xC2: return "Macronix";
        default:   return "Unknown";
    }
}

uint32_t flash_decodeCapacityKB(uint8_t cap) {
    if (cap >= 0x10 && cap <= 0x21) return (1UL << cap) / 1024;
    return 0;
}

const char *flash_detectContent(const uint8_t *buf, uint16_t len) {
    if (len < 4) return "Too short";

    bool allFF = true, all00 = true;
    for (uint16_t i = 0; i < len; i++) {
        if (buf[i] != 0xFF) allFF = false;
        if (buf[i] != 0x00) all00 = false;
        if (!allFF && !all00) break;
    }
    if (allFF) return "Empty (erased)";
    if (all00) return "Zero-filled";

    if (buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F')
        return "ELF binary";

    uint32_t w0 = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    uint32_t w1 = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
    if ((w0 >= 0x20000000UL && w0 <= 0x24080000UL) &&
        (w1 & 1) && ((w1 >= 0x08000000UL && w1 < 0x08200000UL) ||
                      (w1 >= 0x90000000UL && w1 < 0x91000000UL)))
        return "ARM firmware";

    if (len >= 0x14 && buf[0x10] == 0x5A && buf[0x11] == 0xA5 &&
        buf[0x12] == 0xF0 && buf[0x13] == 0x0F)
        return "UEFI BIOS (IFD)";

    if ((buf[0] == 0xEB || buf[0] == 0xE9) && len >= 512 &&
        buf[510] == 0x55 && buf[511] == 0xAA)
        return "FAT filesystem";

    if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF) return "JPEG image";
    if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G') return "PNG image";
    if (buf[0] == 'B' && buf[1] == 'M') return "BMP image";
    if (buf[0] == 'P' && buf[1] == 'K') return "ZIP archive";

    uint16_t printable = 0;
    for (uint16_t i = 0; i < (len < 128 ? len : 128); i++)
        if (buf[i] >= 0x20 && buf[i] <= 0x7E) printable++;
    if (printable > 100) return "ASCII text";

    return "Unknown data";
}
