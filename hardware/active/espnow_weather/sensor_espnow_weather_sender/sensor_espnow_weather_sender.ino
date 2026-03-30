#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define BMP_SDA 21
#define BMP_SCL 22
#define SOIL_PIN 34
#define SOIL_RAW_DRY 720
#define SOIL_RAW_WET 160
#define ESPNOW_WIFI_CHANNEL 6
#define BMP280_PRIMARY_ADDRESS 0x76
#define BMP280_SECONDARY_ADDRESS 0x77
#define BMP280_MAX_READ_RETRIES 3
#define SOIL_SAMPLE_COUNT 10
#define SOIL_DEBUG 1
#define DHT_MAX_READ_RETRIES 2
#define SMOOTHING_WINDOW 3
#define SOIL_MIN_VALID_RAW 50
#define SOIL_MAX_VALID_RAW 4000
#define TEMP_MIN_C -40.0f
#define TEMP_MAX_C 80.0f
#define PRESSURE_MIN_HPA 300.0f
#define PRESSURE_MAX_HPA 1100.0f
#define SENSOR_TRANSPORT_DIRECT_WIFI 1
#define STRINGIFY_INNER(value) #value
#define STRINGIFY(value) STRINGIFY_INNER(value)
#define BACKEND_IP_1 172
#define BACKEND_IP_2 16
#define BACKEND_IP_3 193
#define BACKEND_IP_4 242
#define ALTITUDE_ESTIMATE_MIN_M -150.0f
#define ALTITUDE_ESTIMATE_MAX_M 4000.0f
#define ALTITUDE_ZERO_SNAP_M 5.0f

// BMP280 measures local pressure directly. Altitude is derived from pressure
// using this sea-level reference, so it should be treated as an estimate.
// Tune this constant for the deployment site if you want the derived altitude
// to better match the station's known elevation.
constexpr float SEA_LEVEL_PRESSURE_HPA = 1013.25f;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

uint8_t displayAddress[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x7B, 0xF0};
const char* WIFI_SSID = "MonaConnect";
const char* WIFI_PASSWORD = "";
const char* DEVICE_ID = "620169874";
const char* BACKEND_URL =
    "http://"
    STRINGIFY(BACKEND_IP_1) "."
    STRINGIFY(BACKEND_IP_2) "."
    STRINGIFY(BACKEND_IP_3) "."
    STRINGIFY(BACKEND_IP_4)
    ":5000/api/weather/update";
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
  float values[SMOOTHING_WINDOW];
  int count;
  int nextIndex;
} FloatSmoother;

SensorData data;
bool bmpReady = false;
uint8_t bmpAddress = BMP280_PRIMARY_ADDRESS;
FloatSmoother temperatureSmoother = {};
FloatSmoother humiditySmoother = {};
FloatSmoother pressureSmoother = {};
FloatSmoother altitudeSmoother = {};
FloatSmoother soilSmoother = {};

void addSmootherSample(FloatSmoother &smoother, float value) {
  smoother.values[smoother.nextIndex] = value;
  smoother.nextIndex = (smoother.nextIndex + 1) % SMOOTHING_WINDOW;

  if (smoother.count < SMOOTHING_WINDOW) {
    smoother.count++;
  }
}

float getSmootherAverage(const FloatSmoother &smoother) {
  if (smoother.count == 0) {
    return NAN;
  }

  float total = 0.0f;
  for (int i = 0; i < smoother.count; i++) {
    total += smoother.values[i];
  }

  return total / smoother.count;
}

bool smootherHasSamples(const FloatSmoother &smoother) {
  return smoother.count > 0;
}

int computeSoilPercent(int rawValue) {
  int percent = map(rawValue, SOIL_RAW_DRY, SOIL_RAW_WET, 0, 100);
  return constrain(percent, 0, 100);
}

int readSoilRaw() {
  long total = 0;

  for (int i = 0; i < SOIL_SAMPLE_COUNT; i++) {
    total += analogRead(SOIL_PIN);
    delay(5);
  }

  return total / SOIL_SAMPLE_COUNT;
}

const char* getSoilStatusBucket(int soilPercent) {
  if (soilPercent <= 20) {
    return "DRY";
  }

  if (soilPercent <= 70) {
    return "MID";
  }

  return "WET";
}

void printSoilDebug(int soilRaw, int soilPercent) {
#if SOIL_DEBUG
  if (soilRaw < 50 || soilRaw > 4000) {
    Serial.println("SOIL SENSOR CHECK WIRING");
    return;
  }

  Serial.print("Soil Raw: ");
  Serial.print(soilRaw);
  Serial.print(" | Soil Moisture: ");
  Serial.print(soilPercent);
  Serial.print("% | Status: ");
  Serial.println(getSoilStatusBucket(soilPercent));
#endif
}

void printSenderReading(const SensorData &reading, int soilPercent, bool bmpFallbackUsed) {
  Serial.println("Sender sample");
  Serial.print("Temperature: ");
  Serial.print(reading.temperature, 1);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(reading.humidity, 1);
  Serial.println(" %");
  Serial.print("Heat Index: ");
  Serial.print(reading.heatIndex, 1);
  Serial.println(" C");
  if (reading.bmpOk) {
    Serial.print("Pressure: ");
    Serial.print(reading.pressure, 1);
    Serial.println(" hPa");
    Serial.print("Estimated Altitude: ");
    Serial.print(reading.altitude, 1);
    Serial.println(" m");
    Serial.print("Sea-Level Ref: ");
    Serial.print(reading.seaLevelPressureHpa, 1);
    Serial.println(" hPa");
  } else if (bmpFallbackUsed) {
    Serial.print("Pressure: ");
    Serial.print(reading.pressure, 1);
    Serial.println(" hPa (stale fallback)");
    Serial.print("Estimated Altitude: ");
    Serial.print(reading.altitude, 1);
    Serial.println(" m (stale fallback)");
    Serial.println("Sea-Level Ref: fallback sample retained because current BMP read was invalid");
  } else {
    Serial.println("Pressure: INVALID");
    Serial.println("Estimated Altitude: INVALID");
    Serial.println("Sea-Level Ref: BMP unavailable");
  }
  Serial.print("Soil Raw: ");
  Serial.println(reading.soilMoisture);
  Serial.print("Soil Moisture: ");
  Serial.print(soilPercent);
  Serial.print("% (");
  Serial.print(getSoilStatusBucket(soilPercent));
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

void printMacAddress(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }

    if (mac[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("ESP-NOW delivery: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");

  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Check gateway MAC and ESPNOW_WIFI_CHANNEL.");
  }
}

bool readDhtValues(float &temperature, float &humidity, float &heatIndex) {
  for (int attempt = 1; attempt <= DHT_MAX_READ_RETRIES; attempt++) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (
      !isnan(temperature) &&
      !isnan(humidity) &&
      temperature >= TEMP_MIN_C &&
      temperature <= TEMP_MAX_C &&
      humidity >= 0.0f &&
      humidity <= 100.0f
    ) {
      heatIndex = dht.computeHeatIndex(temperature, humidity, false);

      if (!isnan(heatIndex)) {
        return true;
      }
    }

    Serial.print("DHT22 read failed on attempt ");
    Serial.println(attempt);
    delay(100);
  }

  return false;
}

bool initializeBmp280() {
  const uint8_t addresses[] = {BMP280_PRIMARY_ADDRESS, BMP280_SECONDARY_ADDRESS};

  for (uint8_t address : addresses) {
    if (bmp.begin(address)) {
      bmp.setSampling(
          Adafruit_BMP280::MODE_NORMAL,
          Adafruit_BMP280::SAMPLING_X2,
          Adafruit_BMP280::SAMPLING_X16,
          Adafruit_BMP280::FILTER_X16,
          Adafruit_BMP280::STANDBY_MS_500);

      bmpReady = true;
      bmpAddress = address;

      Serial.print("BMP280 ready at I2C address 0x");
      Serial.println(bmpAddress, HEX);
      return true;
    }
  }

  bmpReady = false;
  Serial.println("BMP280 init failed on 0x76 and 0x77");
  return false;
}

bool readBmp280(float &pressure, float &altitude) {
  for (int attempt = 1; attempt <= BMP280_MAX_READ_RETRIES; attempt++) {
    if (!bmpReady && !initializeBmp280()) {
      delay(100);
      continue;
    }

    // Pressure is the primary BMP280 reading. Compute altitude only after the
    // pressure value itself passes validation.
    const float rawPressure = bmp.readPressure() / 100.0f;
    if (isnan(rawPressure)) {
      Serial.print("BMP280 validation failed on attempt ");
      Serial.print(attempt);
      Serial.println(": pressure is NaN");
    } else if (rawPressure < PRESSURE_MIN_HPA || rawPressure > PRESSURE_MAX_HPA) {
      Serial.print("BMP280 validation failed on attempt ");
      Serial.print(attempt);
      Serial.print(": pressure out of range raw=");
      Serial.print(rawPressure, 3);
      Serial.println(" hPa");
    } else {
      float rawAltitude = bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);
      if (isnan(rawAltitude)) {
        Serial.print("BMP280 validation failed on attempt ");
        Serial.print(attempt);
        Serial.println(": altitude is NaN");
      } else if (rawAltitude < ALTITUDE_ESTIMATE_MIN_M || rawAltitude > ALTITUDE_ESTIMATE_MAX_M) {
        Serial.print("BMP280 validation failed on attempt ");
        Serial.print(attempt);
        Serial.print(": estimated altitude out of range raw=");
        Serial.print(rawAltitude, 3);
        Serial.println(" m");
      } else {
        if (fabs(rawAltitude) < ALTITUDE_ZERO_SNAP_M) {
          rawAltitude = 0.0f;
        }

        pressure = rawPressure;
        altitude = rawAltitude;
        return true;
      }
    }

    Serial.print("BMP280 read failed on attempt ");
    Serial.print(attempt);
    Serial.print(", reinitializing sensor at 0x");
    Serial.println(bmpAddress, HEX);
    bmpReady = false;
    delay(100);
  }

  pressure = NAN;
  altitude = NAN;
  return false;
}

bool readSoilValue(int &soilRaw, int &soilPercent) {
  soilRaw = readSoilRaw();
  soilPercent = computeSoilPercent(soilRaw);

  if (soilRaw < SOIL_MIN_VALID_RAW || soilRaw > SOIL_MAX_VALID_RAW) {
    Serial.println("SOIL SENSOR CHECK WIRING");
    return false;
  }

  return true;
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

bool postReadingToBackend(const SensorData &reading, int soilPercent) {
  Serial.println("----- SENSOR POST -----");

  if (!ensureWiFiConnected()) {
    Serial.println("Abort: Wi-Fi not connected");
    Serial.println("-----------------------");
    return false;
  }

  Serial.print("Backend URL: ");
  Serial.println(BACKEND_URL);
  Serial.print("Wi-Fi status: ");
  Serial.println(WiFi.status());
  Serial.print("Sender local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Sender default gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Sender RSSI: ");
  Serial.println(WiFi.RSSI());

  WiFiClient client;
  Serial.print("Testing TCP connection to ");
  Serial.print(BACKEND_HOST);
  Serial.print(":");
  Serial.println(BACKEND_PORT);

  if (!client.connect(BACKEND_HOST, BACKEND_PORT)) {
    Serial.println("TCP connect failed");
    Serial.println("-----------------------");
    client.stop();
    return false;
  }

  Serial.println("TCP connect succeeded");
  client.stop();

  HTTPClient http;
  http.setTimeout(5000);
  Serial.println("Starting HTTP client...");
  const bool beginOk = http.begin(client, BACKEND_URL);
  Serial.print("http.begin: ");
  Serial.println(beginOk ? "OK" : "FAILED");
  if (!beginOk) {
    Serial.println("Abort: HTTP client could not start");
    Serial.println("-----------------------");
    return false;
  }
  http.addHeader("Content-Type", "application/json");

  char payload[384];
  snprintf(
      payload,
      sizeof(payload),
      "{\"id\":\"%s\",\"type\":\"weather\",\"temperature\":%.1f,\"humidity\":%.1f,\"heatIndex\":%.1f,\"pressure\":%.1f,\"altitude\":%.1f,\"seaLevelPressureHpa\":%.1f,\"altitudeEstimated\":%u,\"soilMoisture\":%d,\"soilRaw\":%d,\"soilMoisturePercent\":%d,\"dhtOk\":%u,\"bmpOk\":%u,\"soilOk\":%u,\"sampleMillis\":%lu}",
      DEVICE_ID,
      reading.temperature,
      reading.humidity,
      reading.heatIndex,
      reading.pressure,
      reading.altitude,
      reading.seaLevelPressureHpa,
      reading.altitudeEstimated,
      reading.soilMoisture,
      reading.soilMoisture,
      soilPercent,
      reading.dhtOk,
      reading.bmpOk,
      reading.soilOk,
      static_cast<unsigned long>(reading.sampleMillis));

  Serial.print("Payload bytes: ");
  Serial.println(strlen(payload));
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.println("Sending POST...");
  const int httpCode = http.POST(payload);
  Serial.print("Backend HTTP code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println("Backend forward: OK");
    Serial.print("Response: ");
    Serial.println(http.getString());
  } else {
    Serial.print("POST failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  Serial.println("------------------------");
  http.end();
  return httpCode > 0;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("SENSOR ESPNOW WEATHER SENDER STARTING");

  dht.begin();
  Wire.begin(BMP_SDA, BMP_SCL);
  Wire.setClock(100000);
  Wire.setTimeOut(200);
  pinMode(SOIL_PIN, INPUT);

  if (!initializeBmp280()) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

#if SENSOR_TRANSPORT_DIRECT_WIFI
  Serial.println("Transport mode: direct Wi-Fi -> backend");
  if (!ensureWiFiConnected()) {
    return;
  }
  Serial.print("Backend target: ");
  Serial.println(BACKEND_URL);
#else
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  uint8_t localMac[6];
  esp_wifi_get_mac(WIFI_IF_STA, localMac);
  Serial.print("Sender MAC: ");
  printMacAddress(localMac);
  Serial.print("Target gateway MAC: ");
  printMacAddress(displayAddress);
  Serial.print("Sender active channel: ");
  Serial.println(ESPNOW_WIFI_CHANNEL);

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
#endif
}

void loop() {
  float t = NAN;
  float h = NAN;
  float hi = NAN;
  float pressure = NAN;
  float altitude = NAN;
  int soilRaw = -1;
  int soilPercent = -1;
  bool bmpFallbackUsed = false;

  const bool dhtOk = readDhtValues(t, h, hi);
  const bool bmpOk = readBmp280(pressure, altitude);
  const bool soilOk = readSoilValue(soilRaw, soilPercent);

  if (dhtOk) {
    addSmootherSample(temperatureSmoother, t);
    addSmootherSample(humiditySmoother, h);
    t = getSmootherAverage(temperatureSmoother);
    h = getSmootherAverage(humiditySmoother);
    hi = dht.computeHeatIndex(t, h, false);
  } else if (smootherHasSamples(temperatureSmoother) && smootherHasSamples(humiditySmoother)) {
    t = getSmootherAverage(temperatureSmoother);
    h = getSmootherAverage(humiditySmoother);
    hi = dht.computeHeatIndex(t, h, false);
    Serial.println("Using last valid DHT22 readings");
  }

  if (bmpOk) {
    addSmootherSample(pressureSmoother, pressure);
    addSmootherSample(altitudeSmoother, altitude);
    pressure = getSmootherAverage(pressureSmoother);
    altitude = getSmootherAverage(altitudeSmoother);
  } else if (smootherHasSamples(pressureSmoother) && smootherHasSamples(altitudeSmoother)) {
    pressure = getSmootherAverage(pressureSmoother);
    altitude = getSmootherAverage(altitudeSmoother);
    bmpFallbackUsed = true;
    Serial.println("Current BMP280 read invalid, using last valid BMP280 baseline");
  }

  if (soilOk) {
    addSmootherSample(soilSmoother, static_cast<float>(soilRaw));
    soilRaw = static_cast<int>(getSmootherAverage(soilSmoother) + 0.5f);
    soilPercent = computeSoilPercent(soilRaw);
  } else if (smootherHasSamples(soilSmoother)) {
    soilRaw = static_cast<int>(getSmootherAverage(soilSmoother) + 0.5f);
    soilPercent = computeSoilPercent(soilRaw);
    Serial.println("Using last valid soil reading");
  }

  if (isnan(t) || isnan(h) || isnan(hi)) {
    Serial.println("No valid DHT22 baseline available");
    delay(2000);
    return;
  }

  if (isnan(pressure) || isnan(altitude)) {
    Serial.println("No valid BMP280 baseline available");
    delay(2000);
    return;
  }

  if (soilRaw < 0 || soilPercent < 0) {
    Serial.println("No valid soil sensor baseline available");
    delay(2000);
    return;
  }

  printSoilDebug(soilRaw, soilPercent);

  data.temperature = t;
  data.humidity = h;
  data.heatIndex = hi;
  data.pressure = pressure;
  data.altitude = altitude;
  data.seaLevelPressureHpa = SEA_LEVEL_PRESSURE_HPA;
  data.soilMoisture = soilRaw;
  data.altitudeEstimated = bmpOk ? 1 : 0;
  data.dhtOk = dhtOk ? 1 : 0;
  data.bmpOk = bmpOk ? 1 : 0;
  data.soilOk = soilOk ? 1 : 0;
  data.sampleMillis = millis();

  printSenderReading(data, soilPercent, bmpFallbackUsed);

#if SENSOR_TRANSPORT_DIRECT_WIFI
  postReadingToBackend(data, soilPercent);
#else
  esp_err_t result = esp_now_send(displayAddress, (uint8_t *)&data, sizeof(data));

  Serial.print("ESP-NOW queue result: ");
  Serial.println(result == ESP_OK ? "OK" : "ERROR");
#endif

  delay(2000);
}
