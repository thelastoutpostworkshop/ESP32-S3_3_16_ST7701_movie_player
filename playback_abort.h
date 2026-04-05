// Purpose: Declares the shared playback abort flag helpers used across the player.
#pragma once

void requestPlaybackAbort();
void clearPlaybackAbort();
bool isPlaybackAbortRequested();
