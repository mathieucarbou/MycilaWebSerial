#ifndef WebSerial_h
#define WebSerial_h

#if defined(ESP8266)
#include "ESP8266WiFi.h"
#elif defined(ESP32)
#include "WiFi.h"
#endif

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <functional>

#define WSL_VERSION          "5.0.0"
#define WSL_VERSION_MAJOR    5
#define WSL_VERSION_MINOR    0
#define WSL_VERSION_REVISION 0
#define WSL_FORK_mathieucarbou

#ifndef WSL_MAX_WS_CLIENTS
#define WSL_MAX_WS_CLIENTS DEFAULT_MAX_WS_CLIENTS
#endif

#ifndef WSL_ALLOW_NON_BUFFERED_WRITE
// By default, write(uint8_t) will fail on purpose because it is not buffered and will cause performance issues with WS
// If WebSerialLite is initialised with a log buffer, this flag will be ignored
#define WSL_FAIL_ON_NON_BUFFERED_WRITE 1
#endif

typedef std::function<void(const String& msg)> WebSerialLiteMessageCallback;

class WebSerialLiteClass : public Print {
  public:
    void begin(AsyncWebServer& server, const char* url = "/webserial", const String& username = "", const String& password = "");

    void setAuthentication(const char* username, const char* password) { setAuthentication(String(username), String(password)); }
    void setAuthentication(const String& username, const String& password);

    // A log buffer can be initialised with an initial capacity to be able to use any Print functions event those that are not buffered and would
    // create a performance impact in WS. The goal of this buffer is to be used with lines ending with '\n', like log messages.
    // The buffer size will eventually grow until a '\n' is found, then the message will be sent to the WS clients and a new buffer will be created.
    void initLogBuffer(size_t initialCapacity = 256);

    void onMessage(WebSerialLiteMessageCallback cb) { _callback = cb; }

    // Print overrides
    size_t write(uint8_t) override;
    size_t write(const uint8_t* buffer, size_t size) override;

  private:
    // Server
    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    WebSerialLiteMessageCallback _callback = nullptr;
    bool _authenticate = false;
    String _username;
    String _password;
    size_t _bufferInitialCapacity = 0;
    String _buffer;

    bool _send(const uint8_t* buffer, size_t size);
};

extern WebSerialLiteClass WebSerialLite;
#endif
