// Purpose: Declares SD card setup and file access helpers used by the player.
#pragma once

bool sdPinsOverlapLcdSpi();
bool initSDCard();
bool remountSDCard();
bool isSDCardAccessible();
