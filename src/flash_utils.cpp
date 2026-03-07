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

    // Filesystems
    if ((buf[0] == 0xEB || buf[0] == 0xE9) && len >= 512 &&
        buf[510] == 0x55 && buf[511] == 0xAA) {
        if (len >= 11 && buf[3] == 'E' && buf[4] == 'X' && buf[5] == 'F' &&
            buf[6] == 'A' && buf[7] == 'T')
            return "ExFAT filesystem";
        if (len >= 7 && buf[3] == 'N' && buf[4] == 'T' && buf[5] == 'F' && buf[6] == 'S')
            return "NTFS filesystem";
        return "FAT filesystem";
    }
    if (len >= 512 && buf[510] == 0x55 && buf[511] == 0xAA)
        return "MBR partition table";
    if (len >= 520 && buf[512] == 'E' && buf[513] == 'F' && buf[514] == 'I' && buf[515] == ' ')
        return "GPT partition table";

    // Bootloaders / firmware
    if (buf[0] == 0x27 && buf[1] == 0x05 && buf[2] == 0x19 && buf[3] == 0x56)
        return "U-Boot image";
    if (buf[0] == 0xD0 && buf[1] == 0x0D && buf[2] == 0xFE && buf[3] == 0xED)
        return "Device Tree Blob";
    if (len >= 8 && buf[0] == '_' && buf[1] == 'F' && buf[2] == 'V' && buf[3] == 'H')
        return "UEFI Firmware Vol";

    // Executables
    if (buf[0] == 'M' && buf[1] == 'Z') return "PE/DOS executable";
    if (buf[0] == 0xCF && buf[1] == 0xFA && buf[2] == 0xED && buf[3] == 0xFE)
        return "Mach-O 64-bit";
    if (buf[0] == 0xCE && buf[1] == 0xFA && buf[2] == 0xED && buf[3] == 0xFE)
        return "Mach-O 32-bit";
    if (buf[0] == 0xCA && buf[1] == 0xFE && buf[2] == 0xBA && buf[3] == 0xBE)
        return "Java class file";
    if (buf[0] == 0x00 && buf[1] == 'a' && buf[2] == 's' && buf[3] == 'm')
        return "WebAssembly";
    if (buf[0] == 'd' && buf[1] == 'e' && buf[2] == 'x' && buf[3] == '\n')
        return "Android DEX";

    // Images
    if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF) return "JPEG image";
    if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G') return "PNG image";
    if (buf[0] == 'B' && buf[1] == 'M') return "BMP image";
    if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == '8') return "GIF image";

    // Archives
    if (buf[0] == 'P' && buf[1] == 'K') return "ZIP archive";
    if (buf[0] == 0x1F && buf[1] == 0x8B) return "GZIP compressed";
    if (buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'r' && buf[3] == '!') return "RAR archive";
    if (buf[0] == 0x37 && buf[1] == 0x7A && buf[2] == 0xBC && buf[3] == 0xAF)
        return "7z archive";

    // Documents / databases
    if (buf[0] == '%' && buf[1] == 'P' && buf[2] == 'D' && buf[3] == 'F') return "PDF document";
    if (len >= 16 && buf[0] == 'S' && buf[1] == 'Q' && buf[2] == 'L' && buf[3] == 'i')
        return "SQLite database";

    // Hex records
    if (buf[0] == ':' && len >= 11) {
        bool validHex = true;
        for (int i = 1; i < 11 && validHex; i++)
            validHex = (buf[i] >= '0' && buf[i] <= '9') ||
                       (buf[i] >= 'A' && buf[i] <= 'F') ||
                       (buf[i] >= 'a' && buf[i] <= 'f');
        if (validHex) return "Intel HEX";
    }
    if (buf[0] == 'S' && (buf[1] >= '0' && buf[1] <= '9'))
        return "Motorola S-record";

    // Text detection
    uint16_t printable = 0;
    for (uint16_t i = 0; i < (len < 128 ? len : 128); i++)
        if (buf[i] >= 0x20 && buf[i] <= 0x7E) printable++;
    if (printable > 100) return "ASCII text";

    return "Unknown data";
}
