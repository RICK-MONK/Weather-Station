#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

typedef struct {
  float temperature;
  float humidity;
} SensorData;

SensorData data;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  Serial.println("DATA RECEIVED CALLBACK");

  Serial.print("From: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.print("Length: ");
  Serial.println(len);

  if (len == sizeof(SensorData)) {
    memcpy(&data, incomingData, sizeof(data));
    Serial.print("Temperature: ");
    Serial.println(data.temperature);
    Serial.print("Humidity: ");
    Serial.println(data.humidity);
  } else {
    Serial.println("Unexpected packet size");
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
  Serial.println("Receiver channel forced to 1");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW receiver ready");
}

void loop() {
  delay(1000);
}
