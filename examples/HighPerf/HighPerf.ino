/*
 * This example shows how to use WebSerial variant to send data to the browser when timing, speed and latency are important.
 * WebSerial focuses on reducing latency and increasing speed by enqueueing messages and sending them in a single packet.
 *
 * The responsibility is left to the caller to ensure that the messages sent are not too large or not too small and frequent.
 * For example, use of printf(), write(c), print(c), etc are not recommended.
 *
 * This variant can allow WebSerial to support a high speed of more than 20 messages per second like in this example.
 *
 * It can be used to log data, debug, or send data to the browser in real-time without any delay.
 *
 * You might want to look at the Logging variant to see how to better use WebSerial for streaming logging.
 *
 * You might want to control these flags to control the async library performance:
 *  -D CONFIG_ASYNC_TCP_QUEUE_SIZE=64
 *  -D CONFIG_ASYNC_TCP_RUNNING_CORE=1
 *  -D WS_MAX_QUEUED_MESSAGES=128
 */
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <AsyncTCP.h>
#include <WiFi.h>
#endif
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <MycilaWebSerial.h>
#include <WString.h>

AsyncWebServer server(80);
WebSerial webSerial;

static uint32_t last = millis();
static uint32_t count = 0;

void setup() {
  Serial.begin(115200);

  WiFi.softAP("WSLDemo");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP().toString());

  webSerial.onMessage([](const std::string& msg) { Serial.println(msg.c_str()); });
  webSerial.begin(&server);

  server.onNotFound([](AsyncWebServerRequest* request) { request->redirect("/webserial"); });
  server.begin();
}

void loop() {
  if (millis() - last > 50) {
    count++;

    size_t max = random(25, 265);
    std::string buffer;
    buffer.reserve(max);
    buffer += std::to_string(count);
    buffer += " ";
    for (int i = 0; i < max; i++)
      buffer += 'a' + rand() % 26;

    // Using Print class method
    // webSerial.print(buffer);

    // Using internal websocket buffer to improve memory consumption and avoid another internal copy when enqueueing the message
    AsyncWebSocketMessageBuffer* wsBuffer = webSerial.makeBuffer(buffer.length());
    memmove(wsBuffer->get(), buffer.c_str(), buffer.length());
    webSerial.send(wsBuffer);

    last = millis();
  }
}
