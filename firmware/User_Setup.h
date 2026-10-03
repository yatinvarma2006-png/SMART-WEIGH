// =========================================================================
// TFT_eSPI User Setup for Smart Nutrition Scale
// Target Display: 2.4" ILI9341 SPI TFT (240x320)
// Controller: ESP32 WROOM-32
//
// Copy this file to your Arduino/libraries/TFT_eSPI/User_Setup.h
// or use PlatformIO where it is configured via build_flags.
// =========================================================================

#ifndef USER_SETUP_H
#define USER_SETUP_H

// 1. Define Driver
#define ILI9341_DRIVER

// 2. Define ESP32 SPI Pins
#define TFT_CS   5   // Chip select control pin
#define TFT_DC   2   // Data/Command control pin
#define TFT_RST  4   // Reset pin
#define TFT_MOSI 23  // Master Out Slave In (VSPI MOSI)
#define TFT_SCLK 18  // SPI Clock (VSPI SCLK)
#define TFT_MISO 19  // MISO (optional/unused for display writes)

// 3. Fonts to load
#define LOAD_GLCD   // Standard font 1
#define LOAD_FONT2  // Small 16 pixel high font
#define LOAD_FONT4  // Medium 26 pixel high font
#define LOAD_FONT6  // Large 48 pixel font
#define LOAD_FONT7  // 7 segment 48 pixel font
#define LOAD_FONT8  // Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// 4. SPI Speed
#define SPI_FREQUENCY       40000000  // 40 MHz SPI clock for ILI9341
#define SPI_READ_FREQUENCY  20000000

#endif // USER_SETUP_H
