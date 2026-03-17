#include <TFT_eSPI.h>

// Known-good display wiring for this board:
// TFT_BL   -> GPIO 21
// TFT_MISO -> GPIO 12
// TFT_MOSI -> GPIO 13
// TFT_SCLK -> GPIO 14
// TFT_CS   -> GPIO 15
// TFT_DC   -> GPIO 2
// TFT_RST  -> -1
//
// Note: TFT_eSPI uses the pin mapping from its User_Setup configuration.

#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();

struct WeatherSample {
  float temperatureC;
  int humidityPercent;
  float pressureHpa;
  const char* statusText;
};

WeatherSample sample = {29.4f, 78, 1008.2f, "Sunny intervals"};

void drawHeader() {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Weather Station", 14, 12);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Display layout test", 14, 36);

  tft.drawFastHLine(12, 48, 296, TFT_DARKGREY);
}

void drawCard(int y, const char* label, const char* value, uint16_t valueColor) {
  tft.drawRoundRect(12, y, 296, 46, 6, TFT_DARKGREY);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(label, 24, y + 8);

  tft.setTextColor(valueColor, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(value, 24, y + 20);
}

void drawDashboard(const WeatherSample& data) {
  char tempText[24];
  char humidityText[24];
  char pressureText[24];

  snprintf(tempText, sizeof(tempText), "%.1f C", data.temperatureC);
  snprintf(humidityText, sizeof(humidityText), "%d %%", data.humidityPercent);
  snprintf(pressureText, sizeof(pressureText), "%.1f hPa", data.pressureHpa);

  tft.fillScreen(TFT_BLACK);

  drawHeader();
  drawCard(62, "Temperature", tempText, TFT_YELLOW);
  drawCard(116, "Humidity", humidityText, TFT_GREENYELLOW);
  drawCard(170, "Pressure", pressureText, TFT_ORANGE);

  tft.drawRoundRect(12, 224, 296, 16, 4, TFT_DARKGREY);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(data.statusText, 24, 228);
}

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);

  drawDashboard(sample);
}

void loop() {
}
