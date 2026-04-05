// Purpose: Reads the QMI8658 IMU and turns qualifying shake gestures into
// skip-to-next-video requests.
#include "shake_next_video.h"

#include <Arduino.h>

#include "app_config.h"

#if SHAKE_NEXT_VIDEO_ENABLED
#include <math.h>

#include <Wire.h>
#include "SensorQMI8658.hpp"
#endif

#include "playback_abort.h"

#if SHAKE_NEXT_VIDEO_ENABLED
static SensorQMI8658 g_qmi8658;
static bool g_shakeSensorReady = false;
static bool g_hasPreviousAccelSample = false;
static float g_lastAccelX = 0.0f;
static float g_lastAccelY = 0.0f;
static float g_lastAccelZ = 0.0f;
static uint32_t g_lastPollMs = 0;
static uint32_t g_ignoreMotionUntilMs = 0;
static uint32_t g_lastShakeMs = 0;
static uint32_t g_firstPulseMs = 0;
static uint8_t g_pulseCount = 0;

static float magnitude3(float x, float y, float z)
{
  return sqrtf((x * x) + (y * y) + (z * z));
}

static void clearShakeWindow()
{
  g_firstPulseMs = 0;
  g_pulseCount = 0;
}

static void storeAccelSample(float x, float y, float z)
{
  g_lastAccelX = x;
  g_lastAccelY = y;
  g_lastAccelZ = z;
  g_hasPreviousAccelSample = true;
}

static void confirmShakePulse(uint32_t now, float accelDeltaG, float gyroMagnitudeDps)
{
  if (g_pulseCount == 0 || (uint32_t)(now - g_firstPulseMs) > SHAKE_NEXT_VIDEO_CONFIRM_WINDOW_MS)
  {
    g_firstPulseMs = now;
    g_pulseCount = 1;
  }
  else
  {
    g_pulseCount++;
  }

  if (g_pulseCount < SHAKE_NEXT_VIDEO_REQUIRED_PULSES)
  {
    return;
  }

  Serial.print("IMU shake detected: accel_delta_g=");
  Serial.print(accelDeltaG, 2);
  Serial.print(" gyro_dps=");
  Serial.println(gyroMagnitudeDps, 1);

  g_lastShakeMs = now;
  clearShakeWindow();
  requestPlaybackAbort();
}
#endif

void initShakeNextVideo()
{
#if !SHAKE_NEXT_VIDEO_ENABLED
  Serial.println("Shake-to-next video disabled");
  return;
#else
  g_shakeSensorReady = false;
  g_hasPreviousAccelSample = false;
  clearShakeWindow();

  if (!g_qmi8658.begin(Wire, SHAKE_NEXT_VIDEO_QMI8658_ADDRESS, SHAKE_NEXT_VIDEO_QMI8658_SDA, SHAKE_NEXT_VIDEO_QMI8658_SCL))
  {
    Serial.println("QMI8658 init failed; shake-to-next video disabled");
    return;
  }

  g_qmi8658.configAccelerometer(SensorQMI8658::ACC_RANGE_8G, SensorQMI8658::ACC_ODR_125Hz, SensorQMI8658::LPF_MODE_3);
  g_qmi8658.configGyroscope(SensorQMI8658::GYR_RANGE_512DPS, SensorQMI8658::GYR_ODR_112_1Hz, SensorQMI8658::LPF_MODE_3);
  g_qmi8658.enableAccelerometer();
  g_qmi8658.enableGyroscope();

  uint32_t now = millis();
  g_lastPollMs = now;
  g_ignoreMotionUntilMs = now + SHAKE_NEXT_VIDEO_STARTUP_SETTLE_MS;
  g_lastShakeMs = now;
  g_shakeSensorReady = true;

  Serial.print("QMI8658 ready: chip_id=0x");
  Serial.println(g_qmi8658.getChipID(), HEX);
  Serial.println("Shake-to-next video enabled");
#endif
}

void pollShakeNextVideo()
{
#if !SHAKE_NEXT_VIDEO_ENABLED
  return;
#else
  if (!g_shakeSensorReady)
  {
    return;
  }

  uint32_t now = millis();
  if ((uint32_t)(now - g_lastPollMs) < SHAKE_NEXT_VIDEO_POLL_INTERVAL_MS)
  {
    return;
  }
  g_lastPollMs = now;

  if (g_pulseCount > 0 && (uint32_t)(now - g_firstPulseMs) > SHAKE_NEXT_VIDEO_CONFIRM_WINDOW_MS)
  {
    clearShakeWindow();
  }

  if (!g_qmi8658.getDataReady())
  {
    return;
  }

  float accelX = 0.0f;
  float accelY = 0.0f;
  float accelZ = 0.0f;
  if (!g_qmi8658.getAccelerometer(accelX, accelY, accelZ))
  {
    return;
  }

  float gyroX = 0.0f;
  float gyroY = 0.0f;
  float gyroZ = 0.0f;
  if (!g_qmi8658.getGyroscope(gyroX, gyroY, gyroZ))
  {
    return;
  }

  if (!g_hasPreviousAccelSample)
  {
    storeAccelSample(accelX, accelY, accelZ);
    return;
  }

  float accelDeltaG = magnitude3(accelX - g_lastAccelX, accelY - g_lastAccelY, accelZ - g_lastAccelZ);
  float gyroMagnitudeDps = magnitude3(gyroX, gyroY, gyroZ);
  storeAccelSample(accelX, accelY, accelZ);

  if (now < g_ignoreMotionUntilMs)
  {
    return;
  }

  if ((uint32_t)(now - g_lastShakeMs) < SHAKE_NEXT_VIDEO_COOLDOWN_MS)
  {
    return;
  }

  if (accelDeltaG < SHAKE_NEXT_VIDEO_ACCEL_DELTA_G || gyroMagnitudeDps < SHAKE_NEXT_VIDEO_GYRO_DPS)
  {
    return;
  }

  confirmShakePulse(now, accelDeltaG, gyroMagnitudeDps);
#endif
}
