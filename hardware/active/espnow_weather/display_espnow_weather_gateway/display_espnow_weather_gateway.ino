#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#define TFT_BL 21
#define SOIL_RAW_DRY 370
#define SOIL_RAW_WET 220
#define ESPNOW_WIFI_CHANNEL 11

const char* WIFI_SSID = "ARRIS-ED5E";
const char* WIFI_PASSWORD = "70DFF79FED5E";
const char* BACKEND_URL = "http://192.168.0.4:5000/api/weather/update";
const char* DEVICE_ID = "620169874";
IPAddress BACKEND_HOST(192, 168, 0, 4);
const uint16_t BACKEND_PORT = 5000;

typedef struct {
  float temperature;
  float humidity;
  float heatIndex;
  float pressure;
  float altitude;
  int soilMoisture;
} SensorData;

TFT_eSPI tft = TFT_eSPI();
SensorData data;
bool hasReading = false;
bool dashboardDirty = false;
bool postPending = false;
unsigned long lastPostMs = 0;

int computeSoilPercent(int rawValue) {
  int percent = map(rawValue, SOIL_RAW_DRY, SOIL_RAW_WET, 0, 100);
  return constrain(percent, 0, 100);
}

const char* getStatusText(int soilPercent) {
  if (soilPercent < 30) {
    return "DRY SOIL";
  }

  if (data.heatIndex >= 32.0f) {
    return "HIGH HEAT";
  }

  if (data.pressure < 980.0f) {
    return "LOW PRESSURE";
  }

  return "SYSTEM NORMAL";
}

uint16_t getStatusColor(int soilPercent) {
  if (soilPercent < 30) {
    return TFT_ORANGE;
  }

  if (data.heatIndex >= 32.0f) {
    return TFT_RED;
  }

  if (data.pressure < 980.0f) {
    return TFT_SKYBLUE;
  }

  return TFT_GREENYELLOW;
}

void drawHeader() {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Weather Station", 14, 12);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("ESP-NOW gateway node", 14, 36);
  tft.drawFastHLine(12, 50, 296, TFT_DARKGREY);
}

void drawField(int x, int y, const char* label, const char* value, uint16_t valueColor) {
  tft.drawRoundRect(x, y, 144, 52, 6, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(label, x + 10, y + 8);

  tft.setTextColor(valueColor, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(value, x + 10, y + 24);
}

void drawDashboard() {
  char temperatureText[20];
  char humidityText[20];
  char heatIndexText[20];
  char pressureText[20];
  char altitudeText[20];
  char soilText[20];
  const int soilPercent = computeSoilPercent(data.soilMoisture);
  const char* statusText = getStatusText(soilPercent);
  const uint16_t statusColor = getStatusColor(soilPercent);

  snprintf(temperatureText, sizeof(temperatureText), "%.1f C", data.temperature);
  snprintf(humidityText, sizeof(humidityText), "%.1f %%", data.humidity);
  snprintf(heatIndexText, sizeof(heatIndexText), "%.1f C", data.heatIndex);
  snprintf(pressureText, sizeof(pressureText), "%.1f hPa", data.pressure);
  snprintf(altitudeText, sizeof(altitudeText), "%.1f m", data.altitude);
  snprintf(soilText, sizeof(soilText), "%d %%", soilPercent);

  tft.fillScreen(TFT_BLACK);
  drawHeader();
  drawField(12, 64, "Temp", temperatureText, TFT_YELLOW);
  drawField(164, 64, "Hum", humidityText, TFT_SKYBLUE);
  drawField(12, 122, "Heat", heatIndexText, TFT_ORANGE);
  drawField(164, 122, "Press", pressureText, TFT_GREENYELLOW);
  drawField(12, 180, "Altitude", altitudeText, TFT_MAGENTA);
  drawField(164, 180, "Soil %", soilText, TFT_CYAN);

  tft.drawRoundRect(180, 10, 128, 22, 4, TFT_DARKGREY);
  tft.setTextColor(statusColor, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(statusText, 190, 17);
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Wi-Fi channel: ");
    Serial.println(WiFi.channel());
    return true;
  }

  Serial.println("Wi-Fi connection failed");
  return false;
}

void postReadingToBackend() {
  if (!ensureWiFiConnected()) {
    return;
  }

  WiFiClient client;
  Serial.print("Testing TCP connection to ");
  Serial.print(BACKEND_HOST);
  Serial.print(":");
  Serial.println(BACKEND_PORT);

  if (!client.connect(BACKEND_HOST, BACKEND_PORT)) {
    Serial.println("TCP connect failed");
    Serial.println("------------------------");
    client.stop();
    return;
  }

  Serial.println("TCP connect succeeded");
  client.stop();

  HTTPClient http;
  http.setTimeout(5000);
  http.begin(client, BACKEND_URL);
  http.addHeader("Content-Type", "application/json");

  char payload[256];
  snprintf(
      payload,
      sizeof(payload),
      "{\"id\":\"%s\",\"type\":\"weather\",\"temperature\":%.1f,\"humidity\":%.1f,\"heatIndex\":%.1f,\"pressure\":%.1f,\"altitude\":%.1f,\"soilMoisture\":%d}",
      DEVICE_ID,
      data.temperature,
      data.humidity,
      data.heatIndex,
      data.pressure,
      data.altitude,
      data.soilMoisture);

  Serial.println("----- GATEWAY POST -----");
  Serial.print("Backend URL: ");
  Serial.println(BACKEND_URL);
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
  Serial.print("Soil Moisture: ");
  Serial.println(data.soilMoisture);
  Serial.println("Sending payload to backend...");

  int httpCode = http.POST(payload);
  Serial.print("HTTP POST code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
  } else {
    Serial.print("POST failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  Serial.println("------------------------");

  http.end();
}

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
    Serial.print("Pressure: ");
    Serial.println(data.pressure);
    Serial.print("Altitude: ");
    Serial.println(data.altitude);
    Serial.print("Soil Moisture Raw: ");
    Serial.println(data.soilMoisture);
    Serial.print("Soil Moisture Percent: ");
    Serial.println(computeSoilPercent(data.soilMoisture));

    hasReading = true;
    dashboardDirty = true;
    postPending = true;
  } else {
    Serial.print("Wrong packet length: ");
    Serial.println(len);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  drawHeader();

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Waiting for data...", 14, 90);

  Serial.println("DISPLAY ESPNOW WEATHER GATEWAY STARTING");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (!ensureWiFiConnected()) {
    Serial.println("Gateway Wi-Fi init failed");
    return;
  }

  if (WiFi.channel() != ESPNOW_WIFI_CHANNEL) {
    Serial.print("Gateway channel mismatch. Router channel is ");
    Serial.print(WiFi.channel());
    Serial.print(", but ESPNOW_WIFI_CHANNEL is ");
    Serial.println(ESPNOW_WIFI_CHANNEL);
    Serial.println("Gateway will stay on router channel for ESP-NOW + Wi-Fi coexistence.");
  }
  Serial.print("Gateway active channel: ");
  Serial.println(WiFi.channel());

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW weather gateway ready");
}

void loop() {
  if (dashboardDirty) {
    drawDashboard();
    dashboardDirty = false;
  }

  if (postPending && millis() - lastPostMs >= 4000) {
    lastPostMs = millis();
    postPending = false;
    postReadingToBackend();
  }

  if (!hasReading) {
    delay(100);
  } else {
    delay(10);
  }
}
