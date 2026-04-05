#include "display_setup.h"
#include "media_file_helpers.h"
#include "mjpeg_helpers.h"
#include "next_video_button.h"
#include "playback_abort.h"
#include "shake_next_video.h"
#include "sd_card_setup.h"
#include "app_config.h"

enum MovieScanResult
{
  MOVIE_SCAN_PLAYED = 0,
  MOVIE_SCAN_NO_CARD,
  MOVIE_SCAN_NO_FOLDER,
  MOVIE_SCAN_NO_MOVIES,
};

// Estimates the pixel width of a centered status string using the current text scale.
static int16_t centeredTextWidth(const char *text, uint8_t textSize)
{
  if (text == NULL)
  {
    return 0;
  }

  return (int16_t)(strlen(text) * 6 * textSize);
}

// Picks the largest readable text size that fits one or two centered status lines on screen.
static uint8_t pickCenteredTextSize(const char *line1, const char *line2)
{
  int16_t displayWidth = gfx->width();
  int16_t displayHeight = gfx->height();
  int lineCount = (line2 != NULL && line2[0] != '\0') ? 2 : 1;

  for (uint8_t size = 4; size >= 1; size--)
  {
    int16_t width1 = centeredTextWidth(line1, size);
    int16_t width2 = centeredTextWidth(line2, size);
    int16_t maxWidth = (width1 > width2) ? width1 : width2;
    int16_t lineHeight = (int16_t)(8 * size);
    int16_t lineGap = (int16_t)(3 * size);
    int16_t totalHeight = (int16_t)(lineCount * lineHeight + ((lineCount - 1) * lineGap));

    if (maxWidth <= (displayWidth - 20) && totalHeight <= (displayHeight - 20))
    {
      return size;
    }

    if (size == 1)
    {
      break;
    }
  }

  return 1;
}

// Clears the screen, logs the message to serial, and draws centered status text.
static void showStatus(const char *line1, const char *line2 = NULL)
{
  if (gfx != NULL)
  {
    gfx->fillScreen(RGB565_BLACK);
  }

  if (line1 != NULL && line1[0] != '\0')
  {
    Serial.println(line1);
  }

  if (line2 != NULL && line2[0] != '\0')
  {
    Serial.println(line2);
  }

  if (gfx == NULL || line1 == NULL || line1[0] == '\0')
  {
    return;
  }

  uint8_t textSize = pickCenteredTextSize(line1, line2);
  int16_t lineHeight = (int16_t)(8 * textSize);
  int16_t lineGap = (int16_t)(3 * textSize);
  int lineCount = (line2 != NULL && line2[0] != '\0') ? 2 : 1;
  int16_t totalHeight = (int16_t)(lineCount * lineHeight + ((lineCount - 1) * lineGap));
  int16_t y = (int16_t)((gfx->height() - totalHeight) / 2);

  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(textSize);

  int16_t x1 = (int16_t)((gfx->width() - centeredTextWidth(line1, textSize)) / 2);
  if (x1 < 0)
  {
    x1 = 0;
  }
  gfx->setCursor(x1, y);
  gfx->print(line1);

  if (lineCount > 1)
  {
    int16_t x2 = (int16_t)((gfx->width() - centeredTextWidth(line2, textSize)) / 2);
    if (x2 < 0)
    {
      x2 = 0;
    }
    gfx->setCursor(x2, (int16_t)(y + lineHeight + lineGap));
    gfx->print(line2);
  }
}

// Shows a final status message and halts the application.
static void stopWithStatus(const char *line1, const char *line2 = NULL)
{
  showStatus(line1, line2);
  while (true)
  {
    delay(1000);
  }
}

// Attempts to remount the SD card and updates the display with recovery progress.
static bool recoverSDCard()
{
  showStatus("CHECKING SD CARD", "READING MEDIA");
  Serial.println("Attempting SD remount...");

  if (!remountSDCard())
  {
    showStatus("INSERT SD CARD", "WAITING FOR MEDIA");
    delay(MJPEG_RETRY_DELAY_MS);
    return false;
  }

  Serial.println("SD card mounted");
  showStatus("SD CARD READY", "READING MJPEG FOLDER");
  delay(200);
  return true;
}

// Returns true when the SD card is readable, otherwise tries to recover it.
static bool ensureSDCardReady()
{
  if (isSDCardAccessible())
  {
    return true;
  }

  return recoverSDCard();
}

// Opens the configured movie folder and verifies that the path is a directory.
static bool openMovieDirectory(const char *directoryPath, File &directory)
{
  if (!openSDDirectory(directoryPath, directory))
  {
    return false;
  }

  if (!directory.isDirectory())
  {
    directory.close();
    return false;
  }

  return true;
}

// Plays one MJPEG file and treats button or IMU aborts as "skip to next video".
static bool playMovieFile(const char *path)
{
  if (path == NULL || path[0] == '\0')
  {
    return false;
  }

  clearPlaybackAbort();

  if (!mediaPathIsMjpeg(path))
  {
    Serial.print("Skipping unsupported file: ");
    Serial.println(path);
    return false;
  }

  Serial.print("Playing: ");
  Serial.println(path);
  gfx->fillScreen(RGB565_BLACK);

  bool ok = playMjpegOnce(path);
  if (isPlaybackAbortRequested())
  {
    Serial.println("Skip requested: advancing to next video");
    clearPlaybackAbort();
    return true;
  }

  if (!ok)
  {
    Serial.print("Playback failed: ");
    Serial.println(path);
  }

  return ok;
}

// Scans the movie folder, plays each supported file once, and reports why playback stopped.
static MovieScanResult playMoviesFromDirectory(const char *directoryPath)
{
  File directory;
  if (!openMovieDirectory(directoryPath, directory))
  {
    Serial.print("Directory open failed: ");
    Serial.println(directoryPath);

    if (!recoverSDCard())
    {
      return MOVIE_SCAN_NO_CARD;
    }

    if (!openMovieDirectory(directoryPath, directory))
    {
      Serial.print("Directory still unavailable after remount: ");
      Serial.println(directoryPath);
      return MOVIE_SCAN_NO_FOLDER;
    }
  }

  bool foundMovie = false;
  directory.rewindDirectory();

  while (true)
  {
    File entry = directory.openNextFile(FILE_READ);
    if (!entry)
    {
      break;
    }

    if (entry.isDirectory())
    {
      entry.close();
      continue;
    }

    const char *entryPath = entry.path();
    if (entryPath == NULL || entryPath[0] == '\0')
    {
      entryPath = entry.name();
    }

    String moviePath = (entryPath != NULL) ? String(entryPath) : String();
    entry.close();

    if (moviePath.length() == 0 || !mediaPathIsMjpeg(moviePath.c_str()))
    {
      continue;
    }

    foundMovie = true;
    playMovieFile(moviePath.c_str());
  }

  directory.close();

  if (!foundMovie)
  {
    File verifyDirectory;
    if (!openMovieDirectory(directoryPath, verifyDirectory))
    {
      Serial.print("Directory verify failed: ");
      Serial.println(directoryPath);

      if (!recoverSDCard())
      {
        return MOVIE_SCAN_NO_CARD;
      }

      if (!openMovieDirectory(directoryPath, verifyDirectory))
      {
        Serial.print("Directory missing after remount verify: ");
        Serial.println(directoryPath);
        return MOVIE_SCAN_NO_FOLDER;
      }
    }

    verifyDirectory.close();
  }

  return foundMovie ? MOVIE_SCAN_PLAYED : MOVIE_SCAN_NO_MOVIES;
}

// Initializes serial output, display, input controls, IMU, and the SD card.
void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("ST7701 MJPEG SD PLAYER INIT...");

  if (sdPinsOverlapLcdSpi())
  {
    Serial.println("Warning: SDMMC pins overlap LCD init SPI pins; native display backend is required.");
  }

  if (!initDisplay())
  {
    stopWithStatus("DISPLAY INIT FAILED");
  }

  initNextVideoButton();
  initShakeNextVideo();

  if (!initSDCard())
  {
    showStatus("INSERT SD CARD", "WAITING FOR MEDIA");
    return;
  }

  showStatus("PLAYER READY", "READING MJPEG FOLDER");
}

// Keeps the player running by polling controls, recovering media, and replaying the folder.
void loop()
{
  pollShakeNextVideo();

  if (!ensureSDCardReady())
  {
    return;
  }

  MovieScanResult result = playMoviesFromDirectory(MJPEG_DIRECTORY_PATH);

  if (result == MOVIE_SCAN_NO_CARD)
  {
    ensureSDCardReady();
    return;
  }

  if (result == MOVIE_SCAN_NO_FOLDER)
  {
    showStatus("NO MJPEG FOLDER", "CREATE FOLDER ON SD");
    delay(MJPEG_RETRY_DELAY_MS);
    return;
  }

  if (result == MOVIE_SCAN_NO_MOVIES)
  {
    showStatus("NO MOVIES TO PLAY", "COPY MJPEG FILES TO SD");
    delay(MJPEG_RETRY_DELAY_MS);
    return;
  }

#if MJPEG_LOOP_FOREVER
  delay(10);
#else
  stopWithStatus("Playback complete");
#endif
}
