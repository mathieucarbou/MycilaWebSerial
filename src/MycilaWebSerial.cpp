// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "MycilaWebSerial.h"
#include <assert.h>
#include <string>
#include <utility>

void WebSerial::setAuthentication(std::string username, std::string password) {
  _username = std::move(username);
  _password = std::move(password);
  _authenticate = !_username.empty() && !_password.empty();
  if (_ws) {
    _ws->setAuthentication(_username.c_str(), _password.c_str());
  }
}

void WebSerial::begin(AsyncWebServer* server, const char* urlHtmlPage, const char* urlWebSocket) {
  if (urlWebSocket == nullptr) {
    return;
  }

  _server = server;
  _ws = new AsyncWebSocket(urlWebSocket);

  if (_authenticate) {
    _ws->setAuthentication(_username.c_str(), _password.c_str());
  }

  if (urlHtmlPage != nullptr && _htmlPage != nullptr) {
    _server->on(urlHtmlPage, HTTP_GET, [&](AsyncWebServerRequest* request) {
      if (_authenticate) {
        if (!request->authenticate(_username.c_str(), _password.c_str()))
          return request->requestAuthentication();
      }

      AsyncWebServerResponse* response = request->beginResponse(200, "text/html", _htmlPage, _htmlPageSize);
      if (_htmlPageEncoding != nullptr) {
        response->addHeader("Content-Encoding", _htmlPageEncoding);
      }

      request->send(response);
    });
  }

  _ws->onEvent([&](__unused AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, __unused void* arg, uint8_t* data, __unused size_t len) -> void {
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
        if (strcmp((char*)data, "ping") == 0)
          client->text("pong");
        else if (_recv)
          _recv(data, len);
      }
    }
  });

  _server->addHandler(_ws);
}

#ifdef WSL_CUSTOM_PAGE
bool WebSerial::setCustomHtmlPage(const uint8_t* ptr, size_t size, const char* encoding) {
  if (ptr == nullptr) {
    return false;
  }

  _htmlPage = ptr;
  _htmlPageSize = size;
  _htmlPageEncoding = encoding;

  return true;
}

bool WebSerial::setCustomHtmlPage(const char* ptr, const char* encoding) {
  if (ptr == nullptr) {
    return false;
  }

  return setCustomHtmlPage((const uint8_t*)ptr, strlen(ptr), encoding);
}
#endif

void WebSerial::onMessage(WSLMessageHandler recv) {
  _recv = std::move(recv);
}

void WebSerial::onMessage(WSLStringMessageHandler callback) {
  _recvString = std::move(callback);
  _recv = [&](uint8_t* data, size_t len) {
    if (data && len) {
      std::string msg;
      msg.reserve(len);
      msg.append((char*)data);
      _recvString(msg);
    }
  };
}

size_t WebSerial::write(uint8_t m) {
  if (!_ws)
    return 0;

  // We do not support non-buffered write on webserial
  // we fail with a stack trace allowing the user to change the code to use write(const uint8_t* buffer, size_t size) instead
  if (!_initialBufferCapacity) {
#ifdef ESP8266
    ets_printf("Non-buffered write is not supported: use webSerial.setBuffer(size_t)");
#else
    log_e("Non-buffered write is not supported: use webSerial.setBuffer(size_t)");
#endif
    assert(false);
    return 0;
  }

  write(&m, 1);
  return (1);
}

size_t WebSerial::write(const uint8_t* buffer, size_t size) {
  if (!_ws || size == 0)
    return 0;

  // No buffer, send directly (i.e. use case for log streaming)
  if (!_initialBufferCapacity) {
    size = buffer[size - 1] == '\n' ? size - 1 : size;
    _send(buffer, size);
    return size;
  }

  // fill the buffer while sending data for each EOL
  size_t start = 0, end = 0;
  while (end < size) {
    if (buffer[end] == '\n') {
      if (end > start) {
        _buffer.append(reinterpret_cast<const char*>(buffer + start), end - start);
      }
      _send(reinterpret_cast<const uint8_t*>(_buffer.c_str()), _buffer.length());
      start = end + 1;
    }
    end++;
  }
  if (end > start) {
    _buffer.append(reinterpret_cast<const char*>(buffer + start), end - start);
  }
  return size;
}

void WebSerial::_send(const uint8_t* buffer, size_t size) {
  if (_ws && size > 0) {
    _ws->cleanupClients(WSL_MAX_WS_CLIENTS);
    if (_ws->count()) {
      _ws->textAll((const char*)buffer, size);
    }
  }

  // if buffer grew too much, free it, otherwise clear it
  if (_initialBufferCapacity) {
    if (_buffer.length() > _initialBufferCapacity) {
      setBuffer(_initialBufferCapacity);
    } else {
      _buffer.clear();
    }
  }
}

void WebSerial::setBuffer(size_t initialCapacity) {
  assert(initialCapacity <= UINT16_MAX);
  _initialBufferCapacity = initialCapacity;
  _buffer = std::string();
  _buffer.reserve(initialCapacity);
}
