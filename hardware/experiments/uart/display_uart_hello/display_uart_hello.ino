String data;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("Display ESP32 ready");
}

void loop() {
  if (Serial2.available()) {
    data = Serial2.readStringUntil('\n');

    Serial.print("Received: ");
    Serial.println(data);
  }
}
