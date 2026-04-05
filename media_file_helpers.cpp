// Purpose: Implements simple path helpers for recognizing playable MJPEG media files.
#include "media_file_helpers.h"

#include <Arduino.h>
#include <SD_MMC.h>

static bool hasSdCardMountPrefix(const char *path)
{
  return (path != NULL) && (strncmp(path, "/sdcard/", 8) == 0);
}

static bool tryOpenPath(const char *path, const char *mode, File &handle)
{
  if (path == NULL || path[0] == '\0')
  {
    handle = File();
    return false;
  }

  handle = (mode == NULL) ? SD_MMC.open(path) : SD_MMC.open(path, mode);
  if (handle)
  {
    return true;
  }

  if (hasSdCardMountPrefix(path))
  {
    return false;
  }

  String altPath = String("/sdcard") + String(path);
  handle = (mode == NULL) ? SD_MMC.open(altPath) : SD_MMC.open(altPath, mode);
  return (bool)handle;
}

bool openSDFile(const char *path, File &fileHandle)
{
  return tryOpenPath(path, FILE_READ, fileHandle);
}

bool openSDDirectory(const char *path, File &dirHandle)
{
  return tryOpenPath(path, NULL, dirHandle);
}

static bool pathEndsWith(const char *path, const char *suffix)
{
  if (path == NULL || suffix == NULL)
  {
    return false;
  }

  size_t pathLen = strlen(path);
  size_t suffixLen = strlen(suffix);
  if (pathLen < suffixLen)
  {
    return false;
  }

  const char *p = path + pathLen - suffixLen;
  while (*p && *suffix)
  {
    char a = *p++;
    char b = *suffix++;
    if (a >= 'A' && a <= 'Z')
    {
      a = (char)(a + ('a' - 'A'));
    }
    if (b >= 'A' && b <= 'Z')
    {
      b = (char)(b + ('a' - 'A'));
    }
    if (a != b)
    {
      return false;
    }
  }
  return true;
}

bool mediaPathIsMjpeg(const char *path)
{
  return pathEndsWith(path, ".mjpeg") || pathEndsWith(path, ".mjpg");
}
