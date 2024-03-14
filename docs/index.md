# WebSerialLite

[![License: LGPL 3.0](https://img.shields.io/badge/License-GPL%203.0-yellow.svg)](https://opensource.org/license/gpl-3-0/)
[![Continuous Integration](https://github.com/mathieucarbou/WebSerialLite/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/WebSerialLite/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/WebSerialLite.svg)](https://registry.platformio.org/libraries/mathieucarbou/WebSerialLite)

WebSerial is a Serial Monitor for ESP32 that can be accessed remotely via a web browser. Webpage is stored in program memory of the microcontroller.

This fork is based on [asjdf/WebSerialLite](https://github.com/asjdf/WebSerialLite).

## Changes in this fork

- Removed ESP8266 support
- Simplified callbacks
- Fixes UI
- Fixes for web socket message cleanup
- Fixed to reconnect logic in the UI
- Command history (up/down arrow keys)

## Preview

![Preview](https://s2.loli.net/2022/08/27/U9mnFjI7frNGltO.png)

[DemoVideo](https://www.bilibili.com/video/BV1Jt4y1E7kj)

## Features

- Works on WebSockets
- Realtime logging
- Any number of Serial Monitors can be opened on the browser
- Uses Async Webserver for better performance
- Light weight (<3k)
- Timestamp
- Event driven

## Dependencies

- [mathieucarbou/ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer) - v2.7.0
