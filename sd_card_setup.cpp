#include "sd_card_setup.h"

#include <Arduino.h>
#include <SD_MMC.h>

#include "screen_config.h"

bool sdPinsOverlapLcdSpi()
{
  return (SDMMC_PIN_CLK == LCD_PIN_SPI_CS) ||
         (SDMMC_PIN_CLK == LCD_PIN_SPI_SCK) ||
         (SDMMC_PIN_CLK == LCD_PIN_SPI_SDO) ||
         (SDMMC_PIN_CMD == LCD_PIN_SPI_CS) ||
         (SDMMC_PIN_CMD == LCD_PIN_SPI_SCK) ||
         (SDMMC_PIN_CMD == LCD_PIN_SPI_SDO) ||
         (SDMMC_PIN_D0 == LCD_PIN_SPI_CS) ||
         (SDMMC_PIN_D0 == LCD_PIN_SPI_SCK) ||
         (SDMMC_PIN_D0 == LCD_PIN_SPI_SDO);
}

bool initSDCard()
{
  // SDMMC needs pullups on CMD and D0 for reliable command/response timing.
  pinMode(SDMMC_PIN_CMD, INPUT_PULLUP);
  pinMode(SDMMC_PIN_D0, INPUT_PULLUP);

  if (!SD_MMC.setPins(SDMMC_PIN_CLK, SDMMC_PIN_CMD, SDMMC_PIN_D0))
  {
    Serial.println("SD_MMC.setPins() failed");
    return false;
  }

  if (!SD_MMC.begin("/sdcard", true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_KHZ))
  {
    Serial.println("SD_MMC.begin() failed");
    return false;
  }

  return true;
}

bool remountSDCard()
{
  SD_MMC.end();
  delay(SDMMC_REMOUNT_DELAY_MS);
  return initSDCard();
}

bool isSDCardAccessible()
{
  if (SD_MMC.cardType() == CARD_NONE)
  {
    return false;
  }

  if (SD_MMC.totalBytes() == 0)
  {
    return false;
  }

  File root = SD_MMC.open("/");
  bool ok = root && root.isDirectory();
  if (root)
  {
    root.close();
  }

  return ok;
}
