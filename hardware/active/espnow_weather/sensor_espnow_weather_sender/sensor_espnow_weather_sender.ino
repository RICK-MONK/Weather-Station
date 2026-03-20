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

// BMP280 measures local pressure directly. Altitude is derived from pressure
// using this calibrated sea-level reference for the project location.
constexpr float SEA_LEVEL_PRESSURE_HPA = 1032.0f;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

uint8_t displayAddress[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x7B, 0xF0};

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

void printSenderReading(const SensorData &reading, int soilPercent) {
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

    // Pressure is the primary BMP280 reading. Altitude is an estimate derived
    // from that pressure using the calibrated project reference above.
    pressure = bmp.readPressure() / 100.0f;
    altitude = bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);

    if (
      !isnan(pressure) &&
      !isnan(altitude) &&
      pressure >= PRESSURE_MIN_HPA &&
      pressure <= PRESSURE_MAX_HPA &&
      altitude >= -1000.0f &&
      altitude <= 10000.0f
    ) {
      return true;
    }

    Serial.print("BMP280 read failed on attempt ");
    Serial.print(attempt);
    Serial.println(", reinitializing sensor...");
    bmpReady = false;
    delay(100);
  }

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
  WiFi.disconnect();
  WiFi.setSleep(false);
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
}

void loop() {
  float t = NAN;
  float h = NAN;
  float hi = NAN;
  float pressure = NAN;
  float altitude = NAN;
  int soilRaw = -1;
  int soilPercent = -1;

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
    Serial.println("Using last valid BMP280 readings");
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
  data.altitudeEstimated = 1;
  data.dhtOk = dhtOk ? 1 : 0;
  data.bmpOk = bmpOk ? 1 : 0;
  data.soilOk = soilOk ? 1 : 0;
  data.sampleMillis = millis();

  printSenderReading(data, soilPercent);

  esp_err_t result = esp_now_send(displayAddress, (uint8_t *)&data, sizeof(data));

  Serial.print("ESP-NOW queue result: ");
  Serial.println(result == ESP_OK ? "OK" : "ERROR");

  delay(2000);
}
