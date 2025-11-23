/*
 * This example shows how to use WebSerial to act as a data logger to stream the Serial output of another ESP32
 *
 * Connect the ESP32 running this code to the other ESP32's Serial RX/TX debug pins. The serial logs and crash dumps will be sent to the browser.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <HardwareSerial.h>
#include <MycilaESPConnect.h>
#include <MycilaWebSerial.h>
#include <WiFi.h>

#define TAG "DataLogger"

static AsyncWebServer server(80);
static WebSerial webSerial;
static Mycila::ESPConnect espConnect(server);

static void startNetworkServices() {
  server.onNotFound([](AsyncWebServerRequest* request) { request->redirect("/console"); });
  server.begin();
  ESP_LOGI(TAG, "Enable mDNS");
  MDNS.begin(espConnect.getConfig().hostname.c_str());
  MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);

  Serial2.begin(115200, SERIAL_8N1, RX2, TX2); // Connect to the other ESP32's RX2 and TX2 pins

  webSerial.begin(&server, "/console", "/consolews");

  server.on("/reset", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest* request) {
    espConnect.clearConfiguration();
    ESP.restart();
    request->send(200);
  });

  espConnect.setAutoRestart(true);
  espConnect.setBlocking(false);
  espConnect.listen([](Mycila::ESPConnect::State previous, Mycila::ESPConnect::State state) {
    ESP_LOGD(TAG, "NetworkState: %s => %s", espConnect.getStateName(previous), espConnect.getStateName(state));
    switch (state) {
      case Mycila::ESPConnect::State::NETWORK_DISABLED:
        ESP_LOGW(TAG, "Disabled Network!");
        break;
      case Mycila::ESPConnect::State::AP_STARTING:
        ESP_LOGI(TAG, "Starting Access Point %s...", espConnect.getAccessPointSSID().c_str());
        break;
      case Mycila::ESPConnect::State::AP_STARTED:
        ESP_LOGI(TAG, "Access Point %s started with IP address %s", espConnect.getWiFiSSID().c_str(), espConnect.getIPAddress().toString().c_str());
        startNetworkServices();
        break;
      case Mycila::ESPConnect::State::NETWORK_CONNECTING:
        ESP_LOGI(TAG, "Connecting to network...");
        break;
      case Mycila::ESPConnect::State::NETWORK_CONNECTED:
        ESP_LOGI(TAG, "Connected with IP address %s", espConnect.getIPAddress().toString().c_str());
        startNetworkServices();
        break;
      case Mycila::ESPConnect::State::NETWORK_TIMEOUT:
        ESP_LOGW(TAG, "Unable to connect!");
        break;
      case Mycila::ESPConnect::State::NETWORK_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected!");
        break;
      case Mycila::ESPConnect::State::NETWORK_RECONNECTING:
        ESP_LOGI(TAG, "Trying to reconnect...");
        break;
      case Mycila::ESPConnect::State::PORTAL_STARTING:
        ESP_LOGI(TAG, "Starting Captive Portal %s for %" PRIu32 " seconds...", espConnect.getAccessPointSSID().c_str(), espConnect.getCaptivePortalTimeout());
        break;
      case Mycila::ESPConnect::State::PORTAL_STARTED:
        ESP_LOGI(TAG, "Captive Portal started at %s with IP address %s", espConnect.getWiFiSSID().c_str(), espConnect.getIPAddress().toString().c_str());
        break;
      case Mycila::ESPConnect::State::PORTAL_COMPLETE: {
        if (espConnect.getConfig().apMode) {
          ESP_LOGI(TAG, "Captive Portal: Access Point configured");
        } else {
          ESP_LOGI(TAG, "Captive Portal: WiFi configured");
        }
        break;
      }
      case Mycila::ESPConnect::State::PORTAL_TIMEOUT:
        ESP_LOGW(TAG, "Captive Portal: timed out.");
        break;
      default:
        break;
    }
  });
  espConnect.begin("DataLogger", "DataLogger");
}

void loop() {
  espConnect.loop();

  if (Serial2.available()) {
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
}
