#ifndef NTP_H
#define NTP_H

#include <WiFi.h>
#include <time.h>
#include <sntp.h>

extern const char* NTP_SERVER_1;
extern const char* NTP_SERVER_2;
extern const char* NTP_TIME_ZONE;
extern TaskHandle_t xNTPHandle;

class Ntp {
 public:
  static void printLocalTime() {
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
      Serial.println("No time available yet.");
      return;
    }

    Serial.println(&timeInfo, "%B %d, %Y %I:%M:%S %p");
  }

  static void onTimeSync(struct timeval* tv) {
    (void)tv;
    Serial.println("Time adjusted from NTP.");
    printLocalTime();
  }

  static unsigned long getTimeStamp() {
    time_t now;
    time(&now);
    return static_cast<unsigned long>(now);
  }

  static void setup() {
    sntp_set_time_sync_notification_cb(onTimeSync);
    sntp_servermode_dhcp(1);
    configTzTime(NTP_TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
  }
};

inline void NTPTask(void* pvParameters) {
  (void)pvParameters;
  Ntp::setup();

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      Ntp::printLocalTime();
    }

    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

inline void startNTPTask() {
  if (xNTPHandle != nullptr) {
    return;
  }

  const BaseType_t created = xTaskCreatePinnedToCore(
      NTPTask,
      "NTP_TASK",
      4096,
      nullptr,
      2,
      &xNTPHandle,
      1);

  if (created != pdPASS) {
    Serial.println("Unable to create NTP task.");
  }
}

#endif
