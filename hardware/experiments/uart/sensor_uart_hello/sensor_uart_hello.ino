void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("Sensor ESP32 started");
}

void loop() {
  Serial.println("Sending message...");
  Serial2.println("HELLO_FROM_SENSOR");

  delay(1000);
}
