#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

typedef struct {
  float temperature;
  float humidity;
  float heatIndex;
} SensorData;

SensorData data;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  Serial.println("DATA RECEIVED CALLBACK");

  if (len == sizeof(SensorData)) {
    memcpy(&data, incomingData, sizeof(data));

    Serial.print("Temperature: ");
    Serial.println(data.temperature);

    Serial.print("Humidity: ");
    Serial.println(data.humidity);

    Serial.print("Heat Index: ");
    Serial.println(data.heatIndex);
  } else {
    Serial.print("Wrong packet length: ");
    Serial.println(len);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("DISPLAY ESPNOW RECEIVER STARTING");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW receiver ready");
}

void loop() {
}
