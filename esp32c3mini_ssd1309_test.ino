#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

// Define ESP32-C3 Hardware SPI2 pins
#define SPI_CLK  4
#define SPI_MOSI 6
#define SPI_MISO -1 // Not used for display output
#define SPI_CS   7

// Define Display Control Pins
#define OLED_DC   3
#define OLED_RST  10

// Initialize U8g2 using full-buffer Hardware SPI (Fastest performance)
// Note: If SSD1309 constructor isn't explicitly available, the standard 
// SSD1306 128x64 noname configuration acts as a fully compatible fallback.
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs= */ SPI_CS, /* dc= */ OLED_DC, /* reset= */ OLED_RST);

void setup() {
  // Force ESP32-C3 to map SPI2 to our chosen hardware pins
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, SPI_CS);
  
  // Start the display
  u8g2.begin();
}

void loop() {
  u8g2.clearBuffer();          // Clear internal memory

  u8g2.setFont(u8g2_font_ncenB08_tr); // Choose a legible font
  u8g2.drawStr(10, 20, "ESP32-C3 + SPI"); // Write text
  u8g2.drawStr(10, 40, "SSD1309 2.42\" OLED");
  u8g2.drawStr(10, 60, "tested by niq_ro"); // Write text
  
  // Draw a frame border around the screen
  u8g2.drawFrame(0, 0, 128, 64);
  
  u8g2.sendBuffer();          // Push memory to physical display
  delay(1000);
}
