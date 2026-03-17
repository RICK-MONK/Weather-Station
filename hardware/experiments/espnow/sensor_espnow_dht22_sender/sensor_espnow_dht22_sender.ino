#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

uint8_t displayAddress[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x7B, 0xF0};

typedef struct {
  float temperature;
  float humidity;
  float heatIndex;
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

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

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
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT22");
    delay(2000);
    return;
  }

  float hi = dht.computeHeatIndex(t, h, false);

  data.temperature = t;
  data.humidity = h;
  data.heatIndex = hi;

  Serial.print("Temperature: ");
  Serial.println(data.temperature);
  Serial.print("Humidity: ");
  Serial.println(data.humidity);
  Serial.print("Heat Index: ");
  Serial.println(data.heatIndex);

  esp_err_t result = esp_now_send(displayAddress, (uint8_t *)&data, sizeof(data));

  Serial.print("esp_now_send result: ");
  Serial.println(result == ESP_OK ? "OK" : "ERROR");

  delay(2000);
}
