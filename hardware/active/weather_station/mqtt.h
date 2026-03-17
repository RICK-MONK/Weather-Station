#ifndef MQTT_H
#define MQTT_H

#include <WiFi.h>
#include <PubSubClient.h>

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const char* MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const char* MQTT_PUBLISH_TOPIC;
extern const char* MQTT_SUBSCRIBE_TOPIC;
extern char DEVICE_NAME[50];

extern WiFiClient espClient;
extern PubSubClient mqtt;
extern TaskHandle_t xMQTTConnectHandle;
extern TaskHandle_t xMQTTLoopHandle;

void mqttCallback(char* topic, byte* payload, unsigned int length);

inline void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("\nConnecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial.print(".");
  }

  Serial.printf("\nWiFi connected. IP address: %s\n", WiFi.localIP().toString().c_str());
}

inline bool publishWeatherPayload(const char* payload) {
  if (!mqtt.connected()) {
    Serial.println("MQTT publish skipped: broker not connected.");
    return false;
  }

  const bool ok = mqtt.publish(MQTT_PUBLISH_TOPIC, payload);
  if (!ok) {
    Serial.println("MQTT publish failed.");
  }
  return ok;
}

inline void MQTTConnectTask(void* pvParameters) {
  (void)pvParameters;
  randomSeed(micros());

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }

    while (!mqtt.connected()) {
      char clientId[64] = {0};
      snprintf(clientId, sizeof(clientId), "IOT_%s_%lu", DEVICE_NAME, random(1000, 9999));

      Serial.printf("Connecting to MQTT broker %s:%u as %s\n", MQTT_SERVER, MQTT_PORT, clientId);
      if (mqtt.connect(clientId)) {
        Serial.println("MQTT connected.");

        if (MQTT_SUBSCRIBE_TOPIC[0] != '\0') {
          mqtt.subscribe(MQTT_SUBSCRIBE_TOPIC);
          Serial.printf("Subscribed to %s\n", MQTT_SUBSCRIBE_TOPIC);
        }
      } else {
        Serial.printf("MQTT connect failed, state=%d. Retrying in 5 seconds.\n", mqtt.state());
        vTaskDelay(pdMS_TO_TICKS(5000));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

inline void MQTTLoopTask(void* pvParameters) {
  (void)pvParameters;

  for (;;) {
    if (mqtt.connected()) {
      mqtt.loop();
    }

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

inline void startMQTTConnectTask() {
  if (xMQTTConnectHandle != nullptr) {
    return;
  }

  const BaseType_t created = xTaskCreatePinnedToCore(
      MQTTConnectTask,
      "MQTT_CONNECT",
      4096,
      nullptr,
      4,
      &xMQTTConnectHandle,
      1);

  if (created != pdPASS) {
    Serial.println("Unable to create MQTT connect task.");
  }
}

inline void startMQTTLoopTask() {
  if (xMQTTLoopHandle != nullptr) {
    return;
  }

  const BaseType_t created = xTaskCreatePinnedToCore(
      MQTTLoopTask,
      "MQTT_LOOP",
      4096,
      nullptr,
      3,
      &xMQTTLoopHandle,
      0);

  if (created != pdPASS) {
    Serial.println("Unable to create MQTT loop task.");
  }
}

inline void initMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(15);
  mqtt.setSocketTimeout(15);

  startMQTTConnectTask();
  startMQTTLoopTask();
}

#endif
