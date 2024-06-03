#include "WebSerialLite.h"

#include "WebSerialLiteWebPage.h"

void WebSerialLiteClass::setAuthentication(const String& username, const String& password) {
  _username = username;
  _password = password;
  _authenticate = !_username.isEmpty() && !_password.isEmpty();
  if (_ws) {
    _ws->setAuthentication(_username.c_str(), _password.c_str());
  }
}

void WebSerialLiteClass::begin(AsyncWebServer* server, const char* url) {
  _server = server;

  String backendUrl = url;
  backendUrl.concat("ws");
  _ws = new AsyncWebSocket(backendUrl);

  if (_authenticate) {
    _ws->setAuthentication(_username, _password);
  }

  _server->on(url, HTTP_GET, [&](AsyncWebServerRequest* request) {
    if (_authenticate) {
      if (!request->authenticate(_username.c_str(), _password.c_str()))
        return request->requestAuthentication();
    }
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", WEBSERIAL_HTML, sizeof(WEBSERIAL_HTML));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  _ws->onEvent([&](__unused AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) -> void {
    if (type == WS_EVT_CONNECT) {
      client->setCloseClientOnQueueFull(false);
      return;
    }
    if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
        }
        const String msg = reinterpret_cast<const char*>(data);
        if (msg == "ping")
          client->text("pong");
        else if (_callback)
          _callback(msg);
      }
    }
  });

  _server->addHandler(_ws);
}

void WebSerialLiteClass::initLogBuffer(size_t initialCapacity) {
  _bufferInitialCapacity = initialCapacity;
  _buffer = String();
  _buffer.reserve(initialCapacity);
}

size_t WebSerialLiteClass::write(uint8_t m) {
  if (!_ws)
    return 0;

  if (_bufferInitialCapacity) {
    if (m == '\n')
      _send(reinterpret_cast<const uint8_t*>(_buffer.c_str()), _buffer.length());
    else
      _buffer.concat(static_cast<char>(m));
    return 1;

  } else {
#if WSL_FAIL_ON_NON_BUFFERED_WRITE == 1
#ifdef ESP8266
    ets_printf("'%s' is set: non-buffered write is not supported. Please use write(const uint8_t* buffer, size_t size) instead.\n", "WSL_FAIL_ON_NON_BUFFERED_WRITE");
#else
    log_e("'-D WSL_FAIL_ON_NON_BUFFERED_WRITE' is set: non-buffered write is not supported. Please use write(const uint8_t* buffer, size_t size) instead.");
#endif
    assert(false);
    return 0;
#else
    return write(&m, 1);
#endif
  }
}

size_t WebSerialLiteClass::write(const uint8_t* buffer, size_t size) {
  if (_bufferInitialCapacity) {
    if (!_ws || size == 0)
      return 0;

    size_t start = 0, end = 0;

    while (end < size) {
      if (buffer[end] == '\n') {
        if (end > start)
          _buffer.concat(reinterpret_cast<const char*>(buffer + start), end - start);
        _send(reinterpret_cast<const uint8_t*>(_buffer.c_str()), _buffer.length());
        start = end + 1;
      }
      end++;
    }

    if (end > start)
      _buffer.concat(reinterpret_cast<const char*>(buffer + start), end - start);

    return size;

  } else {
    return _send(buffer, size);
  }
}

bool WebSerialLiteClass::_send(const uint8_t* buffer, size_t size) {
  bool queued = false;

  if (_ws && size > 0) {
    _ws->cleanupClients(WSL_MAX_WS_CLIENTS);

    if (_ws->count()) {
      _ws->textAll((const char*)buffer, size);
      queued = true;
    }
  }

  // re-use or recreated new buffer if needed
  if (_bufferInitialCapacity) {
    if (_buffer.length() > _bufferInitialCapacity)
      initLogBuffer(_bufferInitialCapacity);
    else
      _buffer.clear();
  }

  return queued;
}

WebSerialLiteClass WebSerialLite;
