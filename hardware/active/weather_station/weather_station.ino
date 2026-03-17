#include <WiFi.h>
#include <PubSubClient.h>

#include "mqtt.h"
#include "ntp.h"

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_PUBLISH_TOPIC = "weatherstation/update";
const char* MQTT_SUBSCRIBE_TOPIC = "weatherstation/commands";
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.nist.gov";
const char* NTP_TIME_ZONE = "EST5";

char DEVICE_NAME[50] = "weather-station-1";

WiFiClient espClient;
PubSubClient mqtt(espClient);

TaskHandle_t xMQTTConnectHandle = nullptr;
TaskHandle_t xMQTTLoopHandle = nullptr;
TaskHandle_t xNTPHandle = nullptr;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("MQTT message received on %s: ", topic);
  for (unsigned int i = 0; i < length; i++) {
    Serial.print(static_cast<char>(payload[i]));
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  connectWiFi();
  startNTPTask();
  initMQTT();
}

void loop() {
  static unsigned long lastPublish = 0;

  if (millis() - lastPublish >= 15000) {
    lastPublish = millis();

    const unsigned long timestamp = Ntp::getTimeStamp();
    char payload[192] = {0};
    snprintf(
        payload,
        sizeof(payload),
        "{\"station\":\"%s\",\"temperature\":%.1f,\"humidity\":%d,\"pressure\":%.1f,\"timestamp\":%lu}",
        DEVICE_NAME,
        29.4,
        78,
        1008.2,
        timestamp);

    publishWeatherPayload(payload);
  }

  delay(100);
}
