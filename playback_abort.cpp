// Purpose: Stores the shared playback abort flag used by button and IMU controls
// to skip the current video.
#include "playback_abort.h"

static volatile bool g_playbackAbortRequested = false;

void requestPlaybackAbort()
{
  g_playbackAbortRequested = true;
}

void clearPlaybackAbort()
{
  g_playbackAbortRequested = false;
}

bool isPlaybackAbortRequested()
{
  return g_playbackAbortRequested;
}
