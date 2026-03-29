#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>

#define TFT_BL 21
#define SOIL_RAW_DRY 720
#define SOIL_RAW_WET 160
#define ESPNOW_WIFI_CHANNEL 6
#define STRINGIFY_INNER(value) #value
#define STRINGIFY(value) STRINGIFY_INNER(value)
#define BACKEND_IP_1 172
#define BACKEND_IP_2 16
#define BACKEND_IP_3 194
#define BACKEND_IP_4 81

const char* WIFI_SSID = "MonaConnect";
const char* WIFI_PASSWORD = "";
const char* BACKEND_URL =
    "http://"
    STRINGIFY(BACKEND_IP_1) "."
    STRINGIFY(BACKEND_IP_2) "."
    STRINGIFY(BACKEND_IP_3) "."
    STRINGIFY(BACKEND_IP_4)
    ":5000/api/weather/update";
const char* DEVICE_ID = "620169874";
const char* PROJECT_LOCATION = "Project Site";
const char* NTP_SERVER_PRIMARY = "pool.ntp.org";
const char* NTP_SERVER_SECONDARY = "time.nist.gov";
constexpr long UTC_OFFSET_SECONDS = -5L * 3600L;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;
constexpr unsigned long UI_REFRESH_INTERVAL_MS = 500;
constexpr unsigned long ESPNOW_STALE_AFTER_MS = 8000;
constexpr unsigned long TOUCH_GESTURE_DEBOUNCE_MS = 250;
constexpr int SOIL_WARNING_PERCENT = 20;
constexpr int SOIL_CRITICAL_WET_PERCENT = 95;
constexpr float HUMIDITY_WARNING_PERCENT = 80.0f;
constexpr float TEMPERATURE_CRITICAL_C = 35.0f;
constexpr float HEAT_INDEX_CRITICAL_C = 41.0f;
constexpr uint16_t ICON_SOIL_BROWN = 0x8A22;
constexpr uint32_t TOUCH_SPI_FREQUENCY = 2000000;
constexpr uint16_t TOUCH_PRESSURE_THRESHOLD = 400;
constexpr int TOUCH_SWIPE_THRESHOLD_PX = 60;
constexpr int TOUCH_VERTICAL_DRIFT_LIMIT_PX = 50;
// Poll touch directly by default. Some board variants do not route T_IRQ
// reliably, so page swipes should not depend on that line being present.
constexpr bool TOUCH_USE_IRQ = false;
// Common ESP32-2432S028/CYD XPT2046 touch wiring. Adjust only if your module
// uses a different touch controller pinout or calibration.
constexpr int TOUCH_CS_PIN = 33;
constexpr int TOUCH_IRQ_PIN = 36;
constexpr int TOUCH_SCLK_PIN = 25;
constexpr int TOUCH_MISO_PIN = 39;
constexpr int TOUCH_MOSI_PIN = 32;
constexpr uint16_t TOUCH_X_MIN = 280;
constexpr uint16_t TOUCH_X_MAX = 3860;
constexpr uint16_t TOUCH_Y_MIN = 340;
constexpr uint16_t TOUCH_Y_MAX = 3860;
constexpr bool TOUCH_MIRROR_X = true;
constexpr bool TOUCH_MIRROR_Y = false;
// Keep these octets aligned with the computer currently running the Flask
// backend. If the PC joins a different Wi-Fi/hotspot, update BACKEND_IP_*.
IPAddress BACKEND_HOST(BACKEND_IP_1, BACKEND_IP_2, BACKEND_IP_3, BACKEND_IP_4);
const uint16_t BACKEND_PORT = 5000;

typedef struct {
  float temperature;
  float humidity;
  float heatIndex;
  float pressure;
  float altitude;
  float seaLevelPressureHpa;
  int soilMoisture;
  uint8_t altitudeEstimated;
  uint8_t dhtOk;
  uint8_t bmpOk;
  uint8_t soilOk;
  uint32_t sampleMillis;
} SensorData;

typedef struct {
  const char* label;
  const char* reason;
  uint16_t color;
} DashboardStatus;

typedef struct {
  const char* label;
  uint16_t color;
} HeatComfortInfo;

enum ScreenId : uint8_t {
  SCREEN_DASHBOARD = 0,
  SCREEN_SOIL = 1,
  SCREEN_WEATHER = 2,
  SCREEN_SYSTEM = 3,
};

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSpi(HSPI);
SensorData data;
bool hasReading = false;
bool dashboardDirty = false;
bool postPending = false;
unsigned long lastPostMs = 0;
unsigned long lastPacketReceivedMs = 0;
unsigned long lastUiRefreshMs = 0;
bool clockConfigured = false;
ScreenId currentScreen = SCREEN_DASHBOARD;
bool touchReady = false;
bool touchGestureActive = false;
int touchStartX = 0;
int touchStartY = 0;
int touchLastX = 0;
int touchLastY = 0;
unsigned long lastTouchGestureMs = 0;

int computeSoilPercent(int rawValue) {
  int percent = map(rawValue, SOIL_RAW_DRY, SOIL_RAW_WET, 0, 100);
  return constrain(percent, 0, 100);
}

const char* getSoilStatusBadge(int soilPercent) {
  if (soilPercent <= SOIL_WARNING_PERCENT) {
    return "DRY";
  }

  if (soilPercent <= 70) {
    return "MID";
  }

  return "WET";
}

uint16_t getSoilStatusColor(int soilPercent) {
  if (soilPercent <= SOIL_WARNING_PERCENT) {
    return TFT_ORANGE;
  }

  if (soilPercent <= 70) {
    return TFT_YELLOW;
  }

  return TFT_CYAN;
}

HeatComfortInfo getHeatComfortInfo(float heatIndex) {
  if (heatIndex < 27.0f) {
    return {"Comfortable", TFT_GREENYELLOW};
  }

  if (heatIndex < 32.0f) {
    return {"Warm", TFT_YELLOW};
  }

  if (heatIndex <= 41.0f) {
    return {"Hot", TFT_ORANGE};
  }

  return {"Dangerous", TFT_RED};
}

const char* getSoilActionText(int soilPercent) {
  if (soilPercent <= SOIL_WARNING_PERCENT) {
    return "Action: WATER PLANTS";
  }

  if (soilPercent <= 70) {
    return "Action: SOIL HEALTHY";
  }

  return "Action: HOLD WATERING";
}

uint16_t getSoilActionColor(int soilPercent) {
  if (soilPercent <= SOIL_WARNING_PERCENT) {
    return TFT_ORANGE;
  }

  if (soilPercent <= 70) {
    return TFT_GREENYELLOW;
  }

  return TFT_CYAN;
}

DashboardStatus evaluateDashboardStatus(const SensorData &reading) {
  const int soilPercent = computeSoilPercent(reading.soilMoisture);

  if (reading.temperature >= TEMPERATURE_CRITICAL_C || reading.heatIndex >= HEAT_INDEX_CRITICAL_C) {
    return {"CRITICAL", "Extreme temp", TFT_RED};
  }

  if (soilPercent >= SOIL_CRITICAL_WET_PERCENT) {
    return {"CRITICAL", "Overflow risk", TFT_RED};
  }

  if (soilPercent <= SOIL_WARNING_PERCENT) {
    return {"WARNING", "Dry soil", TFT_ORANGE};
  }

  if (reading.humidity >= HUMIDITY_WARNING_PERCENT) {
    return {"WARNING", "High humidity", TFT_YELLOW};
  }

  return {"NORMAL", "Conditions stable", TFT_GREENYELLOW};
}

uint16_t getDashboardStatusTextColor(uint16_t backgroundColor) {
  if (backgroundColor == TFT_RED) {
    return TFT_WHITE;
  }

  return TFT_BLACK;
}

void printGatewayReading(const SensorData &reading) {
  const int soilPercent = computeSoilPercent(reading.soilMoisture);
  const DashboardStatus status = evaluateDashboardStatus(reading);
  const HeatComfortInfo comfort = getHeatComfortInfo(reading.heatIndex);

  Serial.println("Gateway reading received");
  Serial.print("Temperature: ");
  Serial.print(reading.temperature, 1);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(reading.humidity, 1);
  Serial.println(" %");
  Serial.print("Heat Index: ");
  Serial.print(reading.heatIndex, 1);
  Serial.println(" C");
  Serial.print("Comfort: ");
  Serial.println(comfort.label);
  Serial.print("Pressure: ");
  Serial.print(reading.pressure, 1);
  Serial.println(" hPa");
  Serial.print("Estimated Altitude: ");
  Serial.print(reading.altitude, 1);
  Serial.println(" m");
  Serial.print("Sea-Level Ref: ");
  Serial.print(reading.seaLevelPressureHpa, 1);
  Serial.println(" hPa");
  Serial.print("Soil Raw: ");
  Serial.println(reading.soilMoisture);
  Serial.print("Soil Moisture: ");
  Serial.print(soilPercent);
  Serial.println("%");
  Serial.println(getSoilActionText(soilPercent));
  Serial.print("Status: ");
  Serial.print(status.label);
  Serial.print(" (");
  Serial.print(status.reason);
  Serial.println(")");
  Serial.print("Sensor Flags (DHT/BMP/SOIL): ");
  Serial.print(reading.dhtOk);
  Serial.print("/");
  Serial.print(reading.bmpOk);
  Serial.print("/");
  Serial.println(reading.soilOk);
  Serial.print("Sample millis: ");
  Serial.println(reading.sampleMillis);
}

void ensureClockConfigured() {
  if (clockConfigured) {
    return;
  }

  configTime(UTC_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);
  clockConfigured = true;
  Serial.println("Clock sync requested");
}

bool getClockText(char *buffer, size_t bufferSize) {
  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 10)) {
    snprintf(buffer, bufferSize, "--:--:--");
    return false;
  }

  strftime(buffer, bufferSize, "%H:%M:%S", &timeInfo);
  return true;
}

unsigned long getPacketAgeMs() {
  if (!hasReading) {
    return 0;
  }

  return millis() - lastPacketReceivedMs;
}

bool isEspNowFresh() {
  return hasReading && getPacketAgeMs() <= ESPNOW_STALE_AFTER_MS;
}

const char* getEspNowStatusText() {
  if (!hasReading) {
    return "WAIT";
  }

  return isEspNowFresh() ? "OK" : "STALE";
}

uint16_t getEspNowStatusColor() {
  if (!hasReading) {
    return TFT_DARKGREY;
  }

  return isEspNowFresh() ? TFT_GREENYELLOW : TFT_ORANGE;
}

uint16_t getLiveDotColor() {
  if (!hasReading) {
    return TFT_DARKGREY;
  }

  if (!isEspNowFresh()) {
    return TFT_ORANGE;
  }

  return (millis() / 500) % 2 == 0 ? TFT_GREENYELLOW : TFT_GREEN;
}

const char* getScreenLabel(ScreenId screen) {
  switch (screen) {
    case SCREEN_SOIL:
      return "Soil";
    case SCREEN_WEATHER:
      return "Weather";
    case SCREEN_SYSTEM:
      return "System";
    case SCREEN_DASHBOARD:
    default:
      return "Dashboard";
  }
}

int bestTwoAverage(int x, int y, int z) {
  const int deltaXY = abs(x - y);
  const int deltaXZ = abs(x - z);
  const int deltaYZ = abs(y - z);

  if (deltaXY <= deltaXZ && deltaXY <= deltaYZ) {
    return (x + y) / 2;
  }

  if (deltaXZ <= deltaXY && deltaXZ <= deltaYZ) {
    return (x + z) / 2;
  }

  return (y + z) / 2;
}

int mapTouchAxis(int rawValue, uint16_t rawMin, uint16_t rawMax, int screenMax, bool mirrored) {
  rawValue = constrain(rawValue, min(rawMin, rawMax), max(rawMin, rawMax));

  if (mirrored) {
    return map(rawValue, rawMin, rawMax, screenMax, 0);
  }

  return map(rawValue, rawMin, rawMax, 0, screenMax);
}

bool readTouchPoint(int &x, int &y) {
  if (!touchReady) {
    return false;
  }

  if (TOUCH_USE_IRQ && digitalRead(TOUCH_IRQ_PIN) != LOW) {
    return false;
  }

  int samples[6] = {0};
  int pressure = 0;

  touchSpi.beginTransaction(SPISettings(TOUCH_SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS_PIN, LOW);

  touchSpi.transfer(0xB1);
  const int z1 = touchSpi.transfer16(0xC1) >> 3;
  pressure = z1 + 4095;
  const int z2 = touchSpi.transfer16(0x91) >> 3;
  pressure -= z2;

  if (pressure >= TOUCH_PRESSURE_THRESHOLD) {
    touchSpi.transfer16(0x91);
    samples[0] = touchSpi.transfer16(0xD1) >> 3;
    samples[1] = touchSpi.transfer16(0x91) >> 3;
    samples[2] = touchSpi.transfer16(0xD1) >> 3;
    samples[3] = touchSpi.transfer16(0x91) >> 3;
    samples[4] = touchSpi.transfer16(0xD0) >> 3;
    samples[5] = touchSpi.transfer16(0x00) >> 3;
  }

  digitalWrite(TOUCH_CS_PIN, HIGH);
  touchSpi.endTransaction();

  if (pressure < TOUCH_PRESSURE_THRESHOLD) {
    return false;
  }

  const int rawX = bestTwoAverage(samples[0], samples[2], samples[4]);
  const int rawY = bestTwoAverage(samples[1], samples[3], samples[5]);
  x = mapTouchAxis(rawX, TOUCH_X_MIN, TOUCH_X_MAX, tft.width() - 1, TOUCH_MIRROR_X);
  y = mapTouchAxis(rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, tft.height() - 1, TOUCH_MIRROR_Y);
  return true;
}

void setScreen(ScreenId screen) {
  currentScreen = screen;
  dashboardDirty = true;
  Serial.print("Display screen: ");
  Serial.println(getScreenLabel(currentScreen));
}

void advanceScreen() {
  setScreen(static_cast<ScreenId>((static_cast<uint8_t>(currentScreen) + 1) % 4));
}

void retreatScreen() {
  setScreen(static_cast<ScreenId>((static_cast<uint8_t>(currentScreen) + 3) % 4));
}

void initializeTouchNavigation() {
  pinMode(TOUCH_CS_PIN, OUTPUT);
  digitalWrite(TOUCH_CS_PIN, HIGH);
  if (TOUCH_USE_IRQ) {
    pinMode(TOUCH_IRQ_PIN, INPUT);
  }
  touchSpi.begin(TOUCH_SCLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN, TOUCH_CS_PIN);
  touchReady = true;

  Serial.println("Touch navigation ready");
  Serial.print("Touch CS: ");
  Serial.println(TOUCH_CS_PIN);
  Serial.print("Touch IRQ mode: ");
  if (TOUCH_USE_IRQ) {
    Serial.print("enabled on GPIO ");
    Serial.println(TOUCH_IRQ_PIN);
  } else {
    Serial.println("polling");
  }
}

void handleTouchNavigation() {
  int x = 0;
  int y = 0;
  const bool touched = readTouchPoint(x, y);

  if (touched) {
    if (!touchGestureActive) {
      touchGestureActive = true;
      touchStartX = x;
      touchStartY = y;
    }

    touchLastX = x;
    touchLastY = y;
    return;
  }

  if (!touchGestureActive) {
    return;
  }

  touchGestureActive = false;

  if (millis() - lastTouchGestureMs < TOUCH_GESTURE_DEBOUNCE_MS) {
    return;
  }

  const int deltaX = touchLastX - touchStartX;
  const int deltaY = touchLastY - touchStartY;

  if (abs(deltaX) < TOUCH_SWIPE_THRESHOLD_PX) {
    return;
  }

  if (abs(deltaY) > TOUCH_VERTICAL_DRIFT_LIMIT_PX && abs(deltaY) >= abs(deltaX)) {
    return;
  }

  lastTouchGestureMs = millis();

  if (deltaX < 0) {
    advanceScreen();
  } else {
    retreatScreen();
  }
}

void drawLocationIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x, y, 4, color);
  tft.fillCircle(x, y, 2, color);
  tft.fillTriangle(x - 3, y + 2, x + 3, y + 2, x, y + 9, color);
}

void drawClockIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x, y, 6, color);
  tft.drawLine(x, y, x, y - 3, color);
  tft.drawLine(x, y, x + 3, y + 2, color);
  tft.fillCircle(x, y, 1, color);
}

void drawSunIcon(int x, int y, uint16_t color) {
  tft.fillCircle(x, y, 4, color);
  tft.drawLine(x, y - 8, x, y - 6, color);
  tft.drawLine(x, y + 6, x, y + 8, color);
  tft.drawLine(x - 8, y, x - 6, y, color);
  tft.drawLine(x + 6, y, x + 8, y, color);
  tft.drawLine(x - 6, y - 6, x - 4, y - 4, color);
  tft.drawLine(x + 4, y - 4, x + 6, y - 6, color);
  tft.drawLine(x - 6, y + 6, x - 4, y + 4, color);
  tft.drawLine(x + 4, y + 4, x + 6, y + 6, color);
}

void drawThermometerIcon(int x, int y, uint16_t color) {
  tft.drawRoundRect(x - 4, y - 10, 6, 12, 2, color);
  tft.fillCircle(x - 1, y + 4, 5, color);
  tft.fillRect(x - 3, y - 8, 4, 10, color);
  drawSunIcon(x + 9, y - 2, TFT_YELLOW);
}

void drawDropletIcon(int x, int y, uint16_t color) {
  tft.fillTriangle(x, y - 7, x - 5, y + 1, x + 5, y + 1, color);
  tft.fillCircle(x, y + 2, 4, color);
  tft.drawPixel(x - 1, y - 1, TFT_WHITE);
  tft.drawPixel(x, y - 2, TFT_WHITE);
}

void drawGaugeIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x, y, 7, color);
  tft.drawFastHLine(x - 5, y + 5, 11, color);
  tft.drawLine(x, y, x + 4, y - 4, color);
  tft.fillCircle(x, y, 2, color);
}

void drawHeatWaveIcon(int x, int y, uint16_t sunColor, uint16_t waveColor) {
  drawSunIcon(x, y - 1, sunColor);
  tft.drawLine(x + 7, y - 3, x + 11, y - 5, waveColor);
  tft.drawLine(x + 11, y - 5, x + 15, y - 3, waveColor);
  tft.drawLine(x + 15, y - 3, x + 19, y - 5, waveColor);
  tft.drawLine(x + 7, y + 1, x + 11, y - 1, waveColor);
  tft.drawLine(x + 11, y - 1, x + 15, y + 1, waveColor);
  tft.drawLine(x + 15, y + 1, x + 19, y - 1, waveColor);
}

void drawMountainIcon(int x, int y, uint16_t color) {
  tft.fillTriangle(x - 8, y + 6, x - 1, y - 4, x + 5, y + 6, color);
  tft.fillTriangle(x, y + 6, x + 7, y - 1, x + 14, y + 6, color);
  tft.fillTriangle(x - 3, y - 1, x - 1, y - 4, x + 1, y - 1, TFT_WHITE);
  tft.fillTriangle(x + 5, y + 1, x + 7, y - 1, x + 9, y + 1, TFT_WHITE);
}

void drawLeafIcon(int x, int y, uint16_t color) {
  tft.drawLine(x, y + 4, x, y - 2, color);
  tft.fillTriangle(x, y - 4, x - 6, y, x, y + 2, color);
  tft.fillTriangle(x, y - 3, x + 6, y, x, y + 2, color);
  tft.fillRoundRect(x - 4, y + 4, 8, 4, 1, ICON_SOIL_BROWN);
}

void drawSmileIcon(int x, int y, uint16_t color) {
  tft.fillCircle(x, y, 7, color);
  tft.fillCircle(x - 2, y - 1, 1, TFT_BLACK);
  tft.fillCircle(x + 2, y - 1, 1, TFT_BLACK);
  tft.drawLine(x - 3, y + 2, x, y + 4, TFT_BLACK);
  tft.drawLine(x, y + 4, x + 3, y + 2, TFT_BLACK);
}

void drawWarningTriangleIcon(int x, int y, uint16_t fillColor, uint16_t markColor) {
  tft.fillTriangle(x, y - 7, x - 7, y + 6, x + 7, y + 6, fillColor);
  tft.drawFastVLine(x, y - 2, 5, markColor);
  tft.drawPixel(x, y + 4, markColor);
}

void drawSignalIcon(int x, int y, uint16_t color) {
  tft.fillRect(x, y + 4, 2, 4, color);
  tft.fillRect(x + 4, y + 2, 2, 6, color);
  tft.fillRect(x + 8, y, 2, 8, color);
}

void drawStatusIcon(int x, int y, const DashboardStatus &status) {
  if (status.color == TFT_GREENYELLOW) {
    drawSmileIcon(x, y, status.color);
    return;
  }

  drawWarningTriangleIcon(x, y, status.color, status.color == TFT_RED ? TFT_WHITE : TFT_BLACK);
}

void drawSoilActionIcon(int x, int y, int soilPercent) {
  if (soilPercent <= SOIL_WARNING_PERCENT) {
    drawDropletIcon(x, y, TFT_ORANGE);
    return;
  }

  if (soilPercent <= 70) {
    drawLeafIcon(x, y, TFT_GREENYELLOW);
    return;
  }

  drawDropletIcon(x, y, TFT_CYAN);
}

void formatAgeText(char *buffer, size_t bufferSize) {
  if (!hasReading) {
    snprintf(buffer, bufferSize, "AGE --");
    return;
  }

  const unsigned long ageSeconds = getPacketAgeMs() / 1000;
  snprintf(buffer, bufferSize, "AGE %lus", ageSeconds);
}

void drawPanelFrame(int x, int y, int width, int height, const char* title, uint16_t titleColor = TFT_LIGHTGREY) {
  tft.drawRoundRect(x, y, width, height, 8, TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(titleColor, TFT_BLACK);
  tft.drawString(title, x + 10, y + 8);
}

void drawValueRow(int x, int y, const String &label, const String &value, uint16_t valueColor = TFT_WHITE) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(label, x, y);
  tft.setTextColor(valueColor, TFT_BLACK);
  tft.drawString(value, x + 82, y);
}

void drawStaticLayout() {
  tft.fillScreen(TFT_BLACK);
  tft.drawFastHLine(12, 48, 296, TFT_DARKGREY);
  tft.drawRoundRect(12, 58, 144, 70, 8, TFT_DARKGREY);
  tft.drawRoundRect(164, 58, 144, 70, 8, TFT_DARKGREY);
  tft.drawRoundRect(12, 136, 296, 58, 8, TFT_DARKGREY);
  tft.drawRoundRect(12, 204, 296, 24, 8, TFT_DARKGREY);

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  drawSunIcon(22, 69, TFT_YELLOW);
  drawGaugeIcon(174, 69, TFT_GREENYELLOW);
  drawLeafIcon(22, 144, TFT_GREENYELLOW);
  tft.drawString("CLIMATE", 34, 64);
  tft.drawString("ATMOSPHERE", 186, 64);
  tft.drawString("SOIL MOISTURE", 34, 142);
}

void drawHeader() {
  char timeText[16];
  getClockText(timeText, sizeof(timeText));

  tft.fillRect(12, 8, 296, 36, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("WEATHER STATION", 12, 10);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  drawLocationIcon(16, 34, TFT_LIGHTGREY);
  tft.drawString(PROJECT_LOCATION, 26, 32);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(getScreenLabel(currentScreen), 308, 32);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawClockIcon(226, 19, TFT_WHITE);
  tft.drawString(timeText, 308, 18);
  tft.setTextDatum(TL_DATUM);
}

void drawClimateCard() {
  tft.fillRect(20, 80, 128, 46, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  if (!hasReading) {
    tft.drawString("Waiting for", 22, 88);
    tft.drawString("sensor data", 22, 102);
    return;
  }

  const HeatComfortInfo comfort = getHeatComfortInfo(data.heatIndex);

  drawThermometerIcon(28, 91, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(data.temperature, 1) + " C", 40, 82);

  drawDropletIcon(28, 104, TFT_SKYBLUE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.drawString(String("Humidity ") + String(data.humidity, 1) + " %", 40, 98);
  drawHeatWaveIcon(20, 112, TFT_ORANGE, TFT_ORANGE);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(String("HeatIdx ") + String(data.heatIndex, 1) + " C", 40, 109);
  tft.setTextColor(comfort.color, TFT_BLACK);
  tft.drawString(String("Comfort ") + String(comfort.label), 40, 120);
}

void drawAtmosphereCard() {
  tft.fillRect(172, 80, 128, 40, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  if (!hasReading) {
    tft.drawString("Waiting for", 174, 88);
    tft.drawString("sensor data", 174, 102);
    return;
  }

  drawGaugeIcon(182, 89, TFT_GREENYELLOW);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(data.pressure, 1) + " hPa", 194, 82);

  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  drawMountainIcon(182, 109, TFT_MAGENTA);
  tft.drawString(String("Est Alt ") + String(data.altitude, 1) + " m", 194, 102);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(String("Ref ") + String(data.seaLevelPressureHpa, 1) + " hPa", 194, 113);
}

void drawSoilProgressBar(int soilPercent) {
  const int barX = 24;
  const int barY = 180;
  const int barWidth = 272;
  const int barHeight = 8;
  const int fillWidth = map(soilPercent, 0, 100, 0, barWidth);
  const uint16_t fillColor = getSoilStatusColor(soilPercent);

  tft.drawRoundRect(barX, barY, barWidth, barHeight, 4, TFT_DARKGREY);
  tft.fillRect(barX + 1, barY + 1, barWidth - 2, barHeight - 2, TFT_BLACK);

  if (fillWidth > 0) {
    tft.fillRoundRect(barX + 1, barY + 1, fillWidth, barHeight - 2, 3, fillColor);
  }
}

void drawSoilCard() {
  tft.fillRect(20, 150, 280, 40, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);

  if (!hasReading) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Waiting for soil updates", 22, 156);
    drawSoilProgressBar(0);
    return;
  }

  const int soilPercent = computeSoilPercent(data.soilMoisture);
  const uint16_t badgeColor = getSoilStatusColor(soilPercent);
  const DashboardStatus status = evaluateDashboardStatus(data);
  const uint16_t statusTextColor = getDashboardStatusTextColor(status.color);
  const char* soilActionText = getSoilActionText(soilPercent);
  const uint16_t soilActionColor = getSoilActionColor(soilPercent);
  constexpr int statusBoxX = 92;
  constexpr int statusBoxY = 148;
  constexpr int statusBoxWidth = 134;
  constexpr int statusBoxHeight = 26;

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(soilPercent) + "%", 22, 150);

  tft.fillRoundRect(statusBoxX, statusBoxY, statusBoxWidth, statusBoxHeight, 6, status.color);
  drawStatusIcon(statusBoxX + 12, statusBoxY + 13, status);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(statusTextColor, status.color);
  tft.drawString("Status", statusBoxX + 24, statusBoxY + 3);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(status.label, statusBoxX + 82, statusBoxY + 18);
  tft.setTextDatum(TL_DATUM);

  tft.fillRoundRect(238, 150, 58, 18, 6, badgeColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, badgeColor);
  tft.drawString(getSoilStatusBadge(soilPercent), 267, 159);
  tft.setTextDatum(TL_DATUM);

  drawSoilActionIcon(28, 172, soilPercent);
  tft.setTextColor(soilActionColor, TFT_BLACK);
  tft.drawString(soilActionText, 40, 171);

  drawSoilProgressBar(soilPercent);
}

void drawDashboardScreen() {
  drawStaticLayout();
  drawClimateCard();
  drawAtmosphereCard();
  drawSoilCard();
}

void drawWeatherScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader();

  drawPanelFrame(12, 58, 144, 82, "WEATHER", TFT_YELLOW);
  drawPanelFrame(164, 58, 144, 82, "ATMOSPHERE", TFT_GREENYELLOW);
  drawPanelFrame(12, 148, 296, 46, "STATUS", TFT_CYAN);

  if (!hasReading) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Waiting for data...", 28, 100);
    return;
  }

  const DashboardStatus status = evaluateDashboardStatus(data);
  const HeatComfortInfo comfort = getHeatComfortInfo(data.heatIndex);

  drawThermometerIcon(24, 89, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(data.temperature, 1) + " C", 36, 80);

  tft.setTextSize(1);
  drawDropletIcon(24, 107, TFT_SKYBLUE);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.drawString(String("Humidity ") + String(data.humidity, 1) + " %", 36, 100);
  drawHeatWaveIcon(16, 122, TFT_ORANGE, TFT_ORANGE);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(String("HeatIdx ") + String(data.heatIndex, 1) + " C", 36, 113);
  tft.setTextColor(comfort.color, TFT_BLACK);
  tft.drawString(String("Comfort ") + String(comfort.label), 36, 126);

  drawGaugeIcon(176, 89, TFT_GREENYELLOW);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(data.pressure, 1) + " hPa", 188, 80);

  tft.setTextSize(1);
  drawMountainIcon(176, 110, TFT_MAGENTA);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString(String("Est Alt ") + String(data.altitude, 1) + " m", 188, 101);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(String("Ref ") + String(data.seaLevelPressureHpa, 1) + " hPa", 188, 114);

  drawStatusIcon(28, 171, status);
  tft.setTextColor(status.color, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(status.label, 42, 160);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(status.reason, 42, 179);
}

void drawSoilScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader();

  drawPanelFrame(12, 58, 296, 94, "SOIL VIEW", TFT_CYAN);
  drawPanelFrame(12, 160, 296, 34, "SOIL ACTION", TFT_GREENYELLOW);

  if (!hasReading) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Waiting for soil data...", 22, 102);
    return;
  }

  const int soilPercent = computeSoilPercent(data.soilMoisture);
  const char* soilActionText = getSoilActionText(soilPercent);
  const uint16_t soilActionColor = getSoilActionColor(soilPercent);
  const uint16_t badgeColor = getSoilStatusColor(soilPercent);

  drawLeafIcon(24, 91, TFT_GREENYELLOW);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString(String(soilPercent) + "%", 42, 74);

  tft.fillRoundRect(220, 76, 70, 22, 8, badgeColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, badgeColor);
  tft.drawString(getSoilStatusBadge(soilPercent), 255, 87);
  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(String("Raw ") + String(data.soilMoisture), 42, 110);
  tft.drawString(String("Sensor ") + (data.soilOk ? "OK" : "Fallback"), 42, 124);
  drawSoilProgressBar(soilPercent);

  drawSoilActionIcon(28, 175, soilPercent);
  tft.setTextColor(soilActionColor, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(soilActionText, 42, 172);
}

void drawSystemScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader();

  drawPanelFrame(12, 58, 296, 62, "CONNECTIONS", TFT_CYAN);
  drawPanelFrame(12, 128, 296, 66, "SYSTEM INFO", TFT_MAGENTA);

  const String wifiStatus = WiFi.status() == WL_CONNECTED ? "Connected" : "Offline";
  const String wifiIp = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "No IP";
  const String espnowStatus = String(getEspNowStatusText());
  const String packetAge = hasReading ? String(getPacketAgeMs() / 1000) + " s" : "--";
  const String sensorFlags = String(data.dhtOk ? "D" : "-") + "/" + String(data.bmpOk ? "B" : "-") + "/" + String(data.soilOk ? "S" : "-");
  const String backendTarget = BACKEND_HOST.toString() + ":" + String(BACKEND_PORT);

  drawSignalIcon(24, 80, getEspNowStatusColor());
  drawValueRow(40, 76, "ESP-NOW", espnowStatus, getEspNowStatusColor());
  drawValueRow(40, 90, "Wi-Fi", wifiStatus, WiFi.status() == WL_CONNECTED ? TFT_GREENYELLOW : TFT_ORANGE);
  drawValueRow(40, 104, "IP", wifiIp, TFT_WHITE);

  drawValueRow(22, 146, "SSID", String(WIFI_SSID), TFT_WHITE);
  drawValueRow(22, 158, "Backend", backendTarget, TFT_WHITE);
  drawValueRow(22, 170, "Packet age", packetAge, TFT_LIGHTGREY);
  drawValueRow(22, 182, "Flags D/B/S", sensorFlags, TFT_WHITE);
}

void drawFooter() {
  char ageText[16];
  formatAgeText(ageText, sizeof(ageText));

  tft.fillRect(18, 208, 284, 16, TFT_BLACK);
  tft.fillRoundRect(20, 208, 54, 16, 8, 0x18C3);
  tft.fillCircle(30, 216, 4, getLiveDotColor());

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, 0x18C3);
  tft.drawString("LIVE", 38, 212);

  drawSignalIcon(78, 212, getEspNowStatusColor());
  tft.setTextColor(getEspNowStatusColor(), TFT_BLACK);
  tft.drawString(String("ESP-NOW: ") + String(getEspNowStatusText()), 94, 212);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(ageText, 300, 212);
  tft.setTextDatum(TL_DATUM);
}

void drawCurrentScreen() {
  switch (currentScreen) {
    case SCREEN_SOIL:
      drawSoilScreen();
      break;
    case SCREEN_WEATHER:
      drawWeatherScreen();
      break;
    case SCREEN_SYSTEM:
      drawSystemScreen();
      break;
    case SCREEN_DASHBOARD:
    default:
      drawDashboardScreen();
      drawHeader();
      break;
  }

  drawFooter();
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    ensureClockConfigured();
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
    ensureClockConfigured();
    return true;
  }

  Serial.println("Wi-Fi connection failed");
  return false;
}

void postReadingToBackend() {
  if (!ensureWiFiConnected()) {
    return;
  }

  const int soilPercent = computeSoilPercent(data.soilMoisture);

  WiFiClient client;
  Serial.print("Testing TCP connection to ");
  Serial.print(BACKEND_HOST);
  Serial.print(":");
  Serial.println(BACKEND_PORT);

  if (!client.connect(BACKEND_HOST, BACKEND_PORT)) {
    Serial.println("TCP connect failed");
    Serial.print("Gateway local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway default gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Gateway RSSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("Configured backend URL: ");
    Serial.println(BACKEND_URL);
    Serial.print("Wi-Fi status: ");
    Serial.println(WiFi.status());
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

  char payload[384];
  snprintf(
      payload,
      sizeof(payload),
      "{\"id\":\"%s\",\"type\":\"weather\",\"temperature\":%.1f,\"humidity\":%.1f,\"heatIndex\":%.1f,\"pressure\":%.1f,\"altitude\":%.1f,\"seaLevelPressureHpa\":%.1f,\"altitudeEstimated\":%u,\"soilMoisture\":%d,\"soilRaw\":%d,\"soilMoisturePercent\":%d,\"dhtOk\":%u,\"bmpOk\":%u,\"soilOk\":%u,\"sampleMillis\":%lu}",
      DEVICE_ID,
      data.temperature,
      data.humidity,
      data.heatIndex,
      data.pressure,
      data.altitude,
      data.seaLevelPressureHpa,
      data.altitudeEstimated,
      data.soilMoisture,
      data.soilMoisture,
      soilPercent,
      data.dhtOk,
      data.bmpOk,
      data.soilOk,
      static_cast<unsigned long>(data.sampleMillis));

  int httpCode = http.POST(payload);
  Serial.print("Backend HTTP code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println("Backend forward: OK");
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
  if (len == sizeof(SensorData)) {
    memcpy(&data, incomingData, sizeof(data));
    printGatewayReading(data);

    hasReading = true;
    lastPacketReceivedMs = millis();
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
  initializeTouchNavigation();

  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  drawCurrentScreen();

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
  handleTouchNavigation();

  if (dashboardDirty || millis() - lastUiRefreshMs >= UI_REFRESH_INTERVAL_MS) {
    drawCurrentScreen();
    dashboardDirty = false;
    lastUiRefreshMs = millis();
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
