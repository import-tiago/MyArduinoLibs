#include <Arduino.h>
#include <time.h>

#include <P4RTC.h>

#define USE_SAO_PAULO_TIMEZONE 1

P4RTC rtc;

void setup() {

  Serial.begin(115200);

#if USE_SAO_PAULO_TIMEZONE
  setenv("TZ", "BRT3", 1); // Sao Paulo: UTC-3
  tzset();
#endif

  // rtc.set_epoch(1784204274);

  rtc.enable_vbat_backup();
}

void loop() {

  time_t now = rtc.get_epoch();

#if USE_SAO_PAULO_TIMEZONE
  struct tm *tm = localtime(&now);
  const char *timezoneName = "Sao Paulo";
#else
  struct tm *tm = gmtime(&now);
  const char *timezoneName = "UTC";
#endif

  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d %s (%lld)\n", tm->tm_year + 1900,
                tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
                tm->tm_sec, timezoneName, static_cast<long long>(now));

  delay(1000);
}
