#include "mjpeg_helpers.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>

#include "display_setup.h"
#include "media_file_helpers.h"
#include "mjpeg_class.h"
#include "next_video_button.h"
#include "playback_abort.h"
#include "shake_next_video.h"
#include "screen_config.h"

static MjpegClass mjpeg;
static File mjpegFile;
static uint8_t *mjpegBuffer = NULL;

#if DISPLAY_BACKEND == DISPLAY_BACKEND_NATIVE_IDF
static const bool kMjpegUseBigEndian = false;
#else
static const bool kMjpegUseBigEndian = true;
#endif

static void logMjpegPlaybackStats(const char *path, uint32_t frameCount, uint32_t elapsedMs, bool success, bool aborted)
{
  float fps = 0.0f;
  if (elapsedMs > 0)
  {
    fps = (frameCount * 1000.0f) / (float)elapsedMs;
  }

  Serial.print("MJPEG stats: path=");
  Serial.print(path);
  Serial.print(" frames=");
  Serial.print(frameCount);
  Serial.print(" elapsed_ms=");
  Serial.print(elapsedMs);
  Serial.print(" fps=");
  Serial.print(fps, 2);
  Serial.print(" success=");
  Serial.print(success ? "true" : "false");
  Serial.print(" aborted=");
  Serial.println(aborted ? "true" : "false");
}

static int jpegDrawCallback(JPEGDRAW *pDraw)
{
#if DISPLAY_BACKEND == DISPLAY_BACKEND_NATIVE_IDF
  draw16bitRGBBitmapFastNoClip(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
#else
  if (pDraw->x < 0 || pDraw->y < 0)
  {
    return 1;
  }

  if ((pDraw->x + pDraw->iWidth) > gfx->width() || (pDraw->y + pDraw->iHeight) > gfx->height())
  {
    return 1;
  }

  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
#endif
}

static bool initMjpegBuffer()
{
  if (mjpegBuffer != NULL)
  {
    return true;
  }

  mjpegBuffer = (uint8_t *)heap_caps_malloc(MJPEG_BUFFER_SIZE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (mjpegBuffer != NULL)
  {
    Serial.println("MJPEG buffer allocated in PSRAM");
    return true;
  }

  mjpegBuffer = (uint8_t *)heap_caps_malloc(MJPEG_BUFFER_SIZE_BYTES, MALLOC_CAP_8BIT);
  if (mjpegBuffer != NULL)
  {
    Serial.println("MJPEG buffer allocated in internal RAM (fallback)");
    return true;
  }

  mjpegBuffer = (uint8_t *)malloc(MJPEG_BUFFER_SIZE_BYTES);
  if (mjpegBuffer == NULL)
  {
    Serial.println("MJPEG buffer alloc failed");
    return false;
  }
  Serial.println("MJPEG buffer allocated with malloc() fallback");

  return true;
}

bool playMjpegOnce(const char *path)
{
  if (!initMjpegBuffer())
  {
    return false;
  }

  if (!openSDFile(path, mjpegFile) || mjpegFile.isDirectory())
  {
    Serial.println("mjpeg open failed");
    return false;
  }

  if (!mjpeg.setup(
          &mjpegFile,
          mjpegBuffer,
          MJPEG_BUFFER_SIZE_BYTES,
          jpegDrawCallback,
          kMjpegUseBigEndian,
          0 /* x */,
          0 /* y */,
          gfx->width() /* widthLimit */,
          gfx->height() /* heightLimit */))
  {
    Serial.println("mjpeg.setup() failed");
    mjpegFile.close();
    return false;
  }

  uint32_t frameCount = 0;
  uint32_t startMs = millis();

  while (mjpegFile.available())
  {
    pollShakeNextVideo();
    pollNextVideoButton();
    if (isPlaybackAbortRequested())
    {
      uint32_t elapsedMs = millis() - startMs;
      logMjpegPlaybackStats(path, frameCount, elapsedMs, false, true);
      mjpegFile.close();
      return false;
    }

    if (!mjpeg.readMjpegBuf())
    {
      Serial.println("mjpeg parse failed (bad stream or buffer too small)");
      uint32_t elapsedMs = millis() - startMs;
      logMjpegPlaybackStats(path, frameCount, elapsedMs, false, false);
      mjpegFile.close();
      return false;
    }
    mjpeg.drawJpg();
    frameCount++;
    yield();
  }

  mjpegFile.close();

  uint32_t elapsedMs = millis() - startMs;
  bool success = frameCount > 0;
  logMjpegPlaybackStats(path, frameCount, elapsedMs, success, false);

  return success;
}
