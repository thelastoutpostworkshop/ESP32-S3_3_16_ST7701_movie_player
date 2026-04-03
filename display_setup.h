#pragma once

#define DISPLAY_BACKEND_ARDUINO_GFX 0
#define DISPLAY_BACKEND_NATIVE_IDF 1

#include "screen_config.h"

#ifndef DISPLAY_BACKEND
#define DISPLAY_BACKEND DISPLAY_BACKEND_ARDUINO_GFX
#endif

#if (DISPLAY_BACKEND != DISPLAY_BACKEND_ARDUINO_GFX) && (DISPLAY_BACKEND != DISPLAY_BACKEND_NATIVE_IDF)
#error "DISPLAY_BACKEND must be DISPLAY_BACKEND_ARDUINO_GFX (0) or DISPLAY_BACKEND_NATIVE_IDF (1)"
#endif

#if DISPLAY_BACKEND == DISPLAY_BACKEND_ARDUINO_GFX

#include <Arduino_GFX_Library.h>
using DisplayDriver = Arduino_RGB_Display;

#else

#include <Arduino.h>

#ifndef RGB565_BLACK
#define RGB565_BLACK 0x0000
#endif

#ifndef RGB565_WHITE
#define RGB565_WHITE 0xFFFF
#endif

class DisplayDriver
{
public:
  bool begin();
  int16_t width() const;
  int16_t height() const;

  void fillScreen(uint16_t color);
  void draw16bitRGBBitmap(int32_t x, int32_t y, const uint16_t *bitmap, int32_t w, int32_t h);
  void draw16bitBeRGBBitmap(int32_t x, int32_t y, const uint16_t *bitmap, int32_t w, int32_t h);

  void setTextColor(uint16_t color);
  void setTextSize(uint8_t size);
  void setCursor(int16_t x, int16_t y);
  size_t print(const char *text);
  size_t println(const char *text);

private:
  uint16_t _textColor = RGB565_WHITE;
  uint8_t _textSize = 1;
  int16_t _cursorX = 0;
  int16_t _cursorY = 0;
};

#endif

extern DisplayDriver *gfx;

bool initDisplay();
void draw16bitRGBBitmapFastNoClip(int32_t x, int32_t y, const uint16_t *bitmap, int32_t w, int32_t h);
