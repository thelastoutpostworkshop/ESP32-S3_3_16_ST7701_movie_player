# ESP32-S3 MJPEG SD Player

This application turns the board into a simple full-screen MJPEG video player for the built-in ST7701 display.

It is designed to:
- play `.mjpeg` and `.mjpg` files from an SD card
- run without WebConfigKit or a web UI
- show clear on-screen status messages when media is missing
- recover when the SD card is removed and reinserted

## What It Does

At startup, the player:
1. initializes the display
2. initializes the SD card
3. scans the configured SD folder for MJPEG files
4. plays each video full screen

If looping is enabled, it starts again from the first file after finishing the folder.

## Main Capabilities

- SD-card MJPEG playback
- Plays all supported videos in one folder
- Full-screen playback on the ST7701 display
- Boot button skip to next video
- Optional shake-to-next-video using the IMU
- Centered on-screen status messages
- Automatic retry when no SD card is present
- Automatic recovery after SD reinsertion

## Supported Media

- File types: `.mjpeg`, `.mjpg`
- Source: SD card
- Playback source folder: configurable in `app_config.h`

## Controls

- `Boot button`: skip to the next video immediately
- `Shake gesture`: skip to the next video if enabled

## User Configuration

The main user settings are in [app_config.h](/c:/Users/charles/Desktop/Repo/ESP32-S3_3_16_ST7701_movie_player/app_config.h):

- `MJPEG_DIRECTORY_PATH`: folder on the SD card that contains the videos
- `MJPEG_LOOP_FOREVER`: replay the folder continuously or stop after one pass
- `MJPEG_RETRY_DELAY_MS`: retry delay when media is missing
- `SHAKE_NEXT_VIDEO_ENABLED`: enable or disable shake-to-next-video

## On-Screen Messages

The player shows clear centered messages for common situations, including:

- no SD card inserted
- SD card ready
- MJPEG folder missing
- no videos found
- playback complete

## Typical Use

1. Put `.mjpeg` or `.mjpg` files into the configured folder on the SD card.
2. Insert the SD card.
3. Power the board.
4. The player will start scanning and playing automatically.

## Notes

- This app plays MJPEG files only.
- It scans one configured folder rather than using playlists or a file browser.
- Configuration is compile-time through `app_config.h`.
- There is no WebConfigKit dependency in this project.
