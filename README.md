# STM32H743 Board Test

Hardware diagnostic and test firmware for the WeAct Studio STM32H743VIT6 development board.

## Features

- **7-page LCD UI** on the built-in 0.96" ST7735 display, navigated with K1 button
- **MCU info** — device/revision ID, flash size, clock speeds, 96-bit unique ID
- **SPI NOR flash** — JEDEC identification, content detection, hex dump, erase
- **QSPI NOR flash** — quad-mode reads at 80MHz, same features as SPI
- **Live sensors** — die temperature, VDDA/VBAT voltage, reset cause
- **CPU & power** — Cortex-M7 revision, cache status, VOS level, PLL source
- **Flash speed test** — 256KB sequential read benchmark (SPI ~500 KB/s, QSPI ~8 MB/s)
- **SRAM memory test** — pattern test across SRAM1-4 (352KB)
- **USB serial console** — command interface over CDC (`info`, `flash`, `sensors`, `cpu`, `speedtest`, `memtest`, `erase spi`, `erase qspi`)
- **Boot animation** — splash screen on startup
- **Hold-to-act** — long press K1 on flash/test pages to trigger erase or run benchmarks

## Hardware

| Component | Details |
|-----------|---------|
| MCU | STM32H743VIT6 @ 480MHz, 2MB flash, 1MB SRAM |
| Display | 0.96" ST7735R 160x80 IPS on SPI4 |
| SPI Flash | Winbond W25Q64 8MB on SPI1 |
| QSPI Flash | Winbond W25Q128 16MB on QUADSPI |
| Button | K1 on PC13 (active high) |
| LED | PE3 (active high) |

## Building

Requires [PlatformIO](https://platformio.org/).

```
pio run              # build
pio run -t upload    # upload via DFU
```

Hold BOOT0 + press RESET to enter DFU mode before uploading.

## Serial Console

Connect via USB and open a serial terminal at 115200 baud. Type `help` for available commands.

## Project Structure

```
src/
  main.cpp        — setup/loop, button state machine
  pins.h          — board pin definitions
  display.h/cpp   — ST7735 driver, drawing helpers
  spi_flash.h/cpp — SPI NOR flash driver
  qspi_flash.h/cpp— QSPI NOR flash driver (quad mode)
  flash_utils.h/cpp— JEDEC decode, content detection
  sensors.h/cpp   — ADC3 temperature and voltage
  tests.h/cpp     — speed benchmark, SRAM test, erase
  ui_pages.h/cpp  — LCD page rendering and navigation
  boot_anim.h/cpp — boot splash animation
  console.h/cpp   — USB serial command handler
```
