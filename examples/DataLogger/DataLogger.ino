/*
 * This example shows how to use WebSerial to act as a data logger to stream the Serial output of another ESP32
 *
 * Connect the ESP32 running this code to the other ESP32's Serial RX/TX debug pins. The serial logs and crash dumps will be sent to the browser.
 */
#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <AsyncTCP.h>
  #include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>
#include <HardwareSerial.h>
#include <MycilaWebSerial.h>

AsyncWebServer server(80);
WebSerial webSerial;

void setup() {
  Serial.begin(115200);

  WiFi.softAP("Data Logger");
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2); // Connect to the other ESP32's RX2 and TX2 pins
  webSerial.begin(&server);

  server.onNotFound([](AsyncWebServerRequest* request) { request->redirect("/webserial"); });
  server.begin();
}

void loop() {
  String line = Serial2.readStringUntil('\n');
  if (line.endsWith("\r")) {
    line.remove(line.length() - 1);
  }
  if (line.length()) {
    Serial.print(line);
    Serial.println();
    webSerial.print(line);
  }
}
