// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#ifdef ESP8266
  #include "ESP8266WiFi.h"
#elif defined(ESP32)
  #include "WiFi.h"
#endif

#ifndef WSL_CUSTOM_PAGE
  #include "MycilaWebSerialPage.h"
#endif

#include <ESPAsyncWebServer.h>
#include <functional>
#include <string>
#include <utility>

#define WSL_VERSION          "8.2.1"
#define WSL_VERSION_MAJOR    8
#define WSL_VERSION_MINOR    2
#define WSL_VERSION_REVISION 1

#ifndef WSL_MAX_WS_CLIENTS
  #define WSL_MAX_WS_CLIENTS DEFAULT_MAX_WS_CLIENTS
#endif

// High performance mode:
// - Low memory footprint (no stack allocation, no global buffer by default)
// - Low latency (messages sent immediately to the WebSocket queue)
// - High throughput (up to 20 messages per second, no locking mechanism)
// Also recommended to tweak AsyncTCP and ESPAsyncWebServer settings, for example:
//  -D CONFIG_ASYNC_TCP_QUEUE_SIZE=64  // AsyncTCP queue size
//  -D CONFIG_ASYNC_TCP_RUNNING_CORE=1  // core for the async_task
//  -D WS_MAX_QUEUED_MESSAGES=128       // WS message queue size

typedef std::function<void(uint8_t* data, size_t len)> WSLMessageHandler;
typedef std::function<void(const std::string& msg)> WSLStringMessageHandler;

class WebSerial : public Print {
  public:
    void begin(AsyncWebServer* server, const char* urlHtmlPage = "/webserial", const char* urlWebSocket = "/webserialws");
#ifdef WSL_CUSTOM_PAGE
    bool setCustomHtmlPage(const uint8_t* ptr, size_t size, const char* encoding = nullptr);
    bool setCustomHtmlPage(const char* ptr, const char* encoding = nullptr);
#endif
    void setAuthentication(std::string username, std::string password);
    void onMessage(WSLMessageHandler recv);
    void onMessage(WSLStringMessageHandler recv);
    size_t write(uint8_t) override;
    size_t write(const uint8_t* buffer, size_t size) override;
    size_t getConnectionCount() const {
      return _ws ? _ws->count() : 0;
    }

    // A buffer (shared across cores) can be initialised with an initial capacity to be able to use any Print functions event those that are not buffered and would
    // create a performance impact for WS calls. The goal of this buffer is to be used with lines ending with '\n', like log messages.
    // The buffer size will eventually grow until a '\n' is found, then the message will be sent to the WS clients and a new buffer will be created.
    // Set initialCapacity to 0 to disable buffering.
    // Must be called before begin(): calling it after will erase the buffer and its content will be lost.
    // The buffer is not enabled by default.
    void setBuffer(size_t initialCapacity);

    // Expose the internal WebSocket makeBuffer to even improve memory consumption on client-side
    // 1. make a AsyncWebSocketMessageBuffer
    // 2. put the data inside
    // 3. send the buffer
    // This method avoids a buffer copy when creating the WebSocket message
    AsyncWebSocketMessageBuffer* makeBuffer(size_t size = 0) {
      if (!_ws)
        return nullptr;
      return _ws->makeBuffer(size);
    }

    void send(AsyncWebSocketMessageBuffer* buffer) {
      if (!_ws || !buffer)
        return;
      _ws->cleanupClients(WSL_MAX_WS_CLIENTS);
      if (_ws->count())
        _ws->textAll(buffer);
    }

  private:
    // Server
    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    WSLMessageHandler _recv = nullptr;
    WSLStringMessageHandler _recvString = nullptr;
    bool _authenticate = false;
    std::string _username;
    std::string _password;
    size_t _initialBufferCapacity = 0;
    std::string _buffer;
#ifdef WSL_CUSTOM_PAGE
    const uint8_t* _htmlPage = nullptr;
    size_t _htmlPageSize = 0;
    const char* _htmlPageEncoding = nullptr;
#else
    const uint8_t* _htmlPage = WEBSERIAL_HTML;
    size_t _htmlPageSize = WEBSERIAL_HTML_SIZE;
    const char* _htmlPageEncoding = "gzip";
#endif
    void _send(const uint8_t* buffer, size_t size);
};
