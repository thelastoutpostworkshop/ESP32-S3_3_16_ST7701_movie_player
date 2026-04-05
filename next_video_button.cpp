// Purpose: Debounces the next-video button input and converts presses into playback
// skip requests.
#include "next_video_button.h"

#include <Arduino.h>

#include "playback_abort.h"
#include "app_config.h"

static bool g_lastRawPressed = false;
static bool g_stablePressed = false;
static uint32_t g_lastChangeMs = 0;

static bool readNextVideoButtonPressed()
{
  int level = digitalRead(NEXT_VIDEO_BUTTON_PIN);
#if NEXT_VIDEO_BUTTON_ACTIVE_LOW
  return level == LOW;
#else
  return level == HIGH;
#endif
}

void initNextVideoButton()
{
#if NEXT_VIDEO_BUTTON_ACTIVE_LOW
  pinMode(NEXT_VIDEO_BUTTON_PIN, INPUT_PULLUP);
#else
  pinMode(NEXT_VIDEO_BUTTON_PIN, INPUT_PULLDOWN);
#endif

  bool pressed = readNextVideoButtonPressed();
  g_lastRawPressed = pressed;
  g_stablePressed = pressed;
  g_lastChangeMs = millis();
}

void pollNextVideoButton()
{
  bool pressed = readNextVideoButtonPressed();
  uint32_t now = millis();

  if (pressed != g_lastRawPressed)
  {
    g_lastRawPressed = pressed;
    g_lastChangeMs = now;
  }

  if ((uint32_t)(now - g_lastChangeMs) < NEXT_VIDEO_BUTTON_DEBOUNCE_MS)
  {
    return;
  }

  if (pressed == g_stablePressed)
  {
    return;
  }

  g_stablePressed = pressed;
  if (g_stablePressed)
  {
    requestPlaybackAbort();
  }
}
