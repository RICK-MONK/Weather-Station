#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

uint8_t displayAddress[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x7B, 0xF0};

typedef struct {
  float temperature;
  float humidity;
} SensorData;

SensorData data;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("SENSOR ESPNOW SENDER STARTING");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);

  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println("Sender channel forced to 1");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, displayAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW sender ready");
}

void loop() {
  data.temperature = 29.5;
  data.humidity = 78.2;

  esp_err_t result = esp_now_send(displayAddress, (uint8_t *)&data, sizeof(data));

  Serial.print("esp_now_send result: ");
  Serial.println(result == ESP_OK ? "OK" : "ERROR");

  delay(2000);
}
