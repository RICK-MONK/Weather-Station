#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define BMP_SDA 21
#define BMP_SCL 22
#define SOIL_PIN 34
#define SEA_LEVEL_HPA 1013.25f
#define ESPNOW_WIFI_CHANNEL 11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

uint8_t displayAddress[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x7B, 0xF0};

typedef struct {
  float temperature;
  float humidity;
  float heatIndex;
  float pressure;
  float altitude;
  int soilMoisture;
} SensorData;

SensorData data;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("SENSOR ESPNOW WEATHER SENDER STARTING");

  dht.begin();
  Wire.begin(BMP_SDA, BMP_SCL);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 init failed");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, displayAddress, 6);
  peerInfo.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW weather sender ready");
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
  float pressure = bmp.readPressure() / 100.0f;
  float altitude = bmp.readAltitude(SEA_LEVEL_HPA);
  int soilMoisture = analogRead(SOIL_PIN);

  if (isnan(pressure) || isnan(altitude)) {
    Serial.println("Failed to read from BMP280");
    delay(2000);
    return;
  }

  data.temperature = t;
  data.humidity = h;
  data.heatIndex = hi;
  data.pressure = pressure;
  data.altitude = altitude;
  data.soilMoisture = soilMoisture;

  Serial.print("Temperature: ");
  Serial.println(data.temperature);
  Serial.print("Humidity: ");
  Serial.println(data.humidity);
  Serial.print("Heat Index: ");
  Serial.println(data.heatIndex);
  Serial.print("Pressure: ");
  Serial.println(data.pressure);
  Serial.print("Altitude: ");
  Serial.println(data.altitude);
  Serial.print("Soil Moisture Raw: ");
  Serial.println(data.soilMoisture);

  esp_err_t result = esp_now_send(displayAddress, (uint8_t *)&data, sizeof(data));

  Serial.print("esp_now_send result: ");
  Serial.println(result == ESP_OK ? "OK" : "ERROR");

  delay(2000);
}
