// Purpose: Declares helper functions for filtering supported MJPEG file paths.
#pragma once

#include <FS.h>

bool openSDFile(const char *path, File &fileHandle);
bool openSDDirectory(const char *path, File &dirHandle);
bool mediaPathIsMjpeg(const char *path);
