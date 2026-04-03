#include "display_setup.h"
#include "media_file_helpers.h"
#include "mjpeg_helpers.h"
#include "sd_card_setup.h"
#include "screen_config.h"

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
}

static void stopWithStatus(const char *line1, const char *line2 = NULL)
{
  showStatus(line1, line2);
  while (true)
  {
    delay(1000);
  }
}

static bool playMovieFile(const char *path)
{
  if (path == NULL || path[0] == '\0')
  {
    return false;
  }

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
  if (!ok)
  {
    Serial.print("Playback failed: ");
    Serial.println(path);
  }

  return ok;
}

static bool playMoviesFromDirectory(const char *directoryPath)
{
  File directory;
  if (!openSDDirectory(directoryPath, directory) || !directory.isDirectory())
  {
    if (directory)
    {
      directory.close();
    }

    Serial.print("Directory open failed: ");
    Serial.println(directoryPath);
    return false;
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
  return foundMovie;
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("ST7701 MJPEG SD player init...");

  if (sdPinsOverlapLcdSpi())
  {
    Serial.println("Warning: SDMMC pins overlap LCD init SPI pins; native display backend is required.");
  }

  if (!initDisplay())
  {
    stopWithStatus("Display init failed");
  }

  if (!initSDCard())
  {
    stopWithStatus("SD mount failed");
  }

  showStatus("Player ready", MJPEG_DIRECTORY_PATH);
}

void loop()
{
  bool playedAny = playMoviesFromDirectory(MJPEG_DIRECTORY_PATH);

  if (!playedAny)
  {
    showStatus("No MJPEG files found", MJPEG_DIRECTORY_PATH);
    delay(MJPEG_RETRY_DELAY_MS);
    return;
  }

#if MJPEG_LOOP_FOREVER
  delay(10);
#else
  stopWithStatus("Playback complete");
#endif
}
