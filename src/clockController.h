#pragma once

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <esp_sntp.h>
#include <wifiClient.h>

constexpr int64_t EVENT_DEADLINE_EPOCH = 1794182400LL; // 2026-11-09 00:00 UTC
constexpr uint32_t NTP_SYNC_INTERVAL_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t NTP_ERROR_AFTER_MS = 30UL * 1000UL;
constexpr uint32_t NTP_STALE_AFTER_MS = 10UL * 60UL * 1000UL;

portMUX_TYPE clockSyncMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool hasValidClock = false;
volatile time_t epochAtLastSync = 0;
volatile uint32_t millisAtLastSync = 0;
volatile uint32_t millisAtLastNtpSync = 0;
bool ntpConfigured = false;
uint32_t millisAtNtpStart = 0;

void onNtpTimeSync(struct timeval *timeValue)
{
  uint32_t syncedAt = millis();
  portENTER_CRITICAL(&clockSyncMux);
  epochAtLastSync = timeValue->tv_sec;
  millisAtLastSync = syncedAt;
  millisAtLastNtpSync = syncedAt;
  hasValidClock = true;
  portEXIT_CRITICAL(&clockSyncMux);

  Serial.printf("NTP synchronized: %lld\n", (long long)timeValue->tv_sec);
}

void clockLoop()
{
  if (!wifiIsConnected() || ntpConfigured)
    return;

  sntp_set_time_sync_notification_cb(onNtpTimeSync);
  configTime(-5 * 60 * 60, 0,
             "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  sntp_set_sync_interval(NTP_SYNC_INTERVAL_MS);
  sntp_restart();
  millisAtNtpStart = millis();
  ntpConfigured = true;
  Serial.println("NTP started (UTC-5, 5-minute sync interval)");
}

bool getCurrentEpoch(time_t &currentEpoch)
{
  time_t baseEpoch;
  uint32_t baseMillis;

  portENTER_CRITICAL(&clockSyncMux);
  bool valid = hasValidClock;
  baseEpoch = epochAtLastSync;
  baseMillis = millisAtLastSync;
  portEXIT_CRITICAL(&clockSyncMux);

  if (!valid)
    return false;

  uint32_t elapsedMillis = (uint32_t)(millis() - baseMillis);
  currentEpoch = baseEpoch + (elapsedMillis / 1000U);
  return true;
}

bool getSecondsRemaining(int64_t &remainingSeconds)
{
  time_t currentEpoch;
  if (!getCurrentEpoch(currentEpoch))
    return false;

  remainingSeconds = EVENT_DEADLINE_EPOCH - (int64_t)currentEpoch;
  return true;
}

bool clockSyncHasTimedOut()
{
  return ntpConfigured && !hasValidClock &&
         (uint32_t)(millis() - millisAtNtpStart) >= NTP_ERROR_AFTER_MS;
}

bool clockSyncIsStale()
{
  uint32_t lastSync;
  bool valid;
  portENTER_CRITICAL(&clockSyncMux);
  valid = hasValidClock;
  lastSync = millisAtLastNtpSync;
  portEXIT_CRITICAL(&clockSyncMux);

  return valid && (uint32_t)(millis() - lastSync) >= NTP_STALE_AFTER_MS;
}

String formatCountdown(int64_t remainingSeconds)
{
  if (remainingSeconds <= 0)
    return "GG!";

  uint64_t remaining = (uint64_t)remainingSeconds;
  uint64_t totalHours = remaining / 3600ULL;
  uint64_t minutes = (remaining / 60ULL) % 60ULL;
  uint64_t seconds = remaining % 60ULL;

  String hoursText = totalHours < 10 ? "0" + String((unsigned long)totalHours)
                                     : String((unsigned long)totalHours);
  String minutesText = minutes < 10 ? "0" + String((unsigned int)minutes)
                                    : String((unsigned int)minutes);
  String secondsText = seconds < 10 ? "0" + String((unsigned int)seconds)
                                    : String((unsigned int)seconds);

  String estimate;
  if (remaining >= 24ULL * 60ULL * 60ULL)
  {
    uint64_t days = (remaining + 24ULL * 60ULL * 60ULL - 1) /
                    (24ULL * 60ULL * 60ULL);
    estimate = "~" + String((unsigned long)days) + "d";
  }
  else if (remaining >= 60ULL * 60ULL)
  {
    estimate = String((unsigned long)(remaining / 3600ULL)) + "h";
  }
  else
  {
    estimate = "<1h";
  }

  String timer = "T-" + hoursText + ":" + minutesText + ":" + secondsText;
  constexpr uint8_t DISPLAY_WIDTH = 16;
  if (timer.length() + estimate.length() >= DISPLAY_WIDTH)
    timer = timer.substring(0, DISPLAY_WIDTH - estimate.length() - 1);

  String line = timer;
  uint8_t spaces = DISPLAY_WIDTH - timer.length() - estimate.length();
  while (spaces-- > 0)
    line += ' ';
  line += estimate;
  return line;
}
