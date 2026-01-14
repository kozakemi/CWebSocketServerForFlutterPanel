# CWebSocketServerForFlutterPanel

这是一个为 [Flutter Linux Panel](https://github.com/kozakemi/flutter_linux_panel) 项目提供后端支持的WebSocket服务器，使用C语言实现。

## 项目概述

本项目实现了与Flutter前端通信的WebSocket服务器，主要用于控制和监控Linux系统上的各种功能，包括WiFi管理等。

## 功能特性

### WiFi管理
- WiFi扫描
- WiFi连接状态查询
- WiFi开关控制

## 技术栈

- C语言
- civetweb - WebSocket和HTTP服务器
- cJSON - JSON解析和生成
- wpa_supplicant - WiFi管理工具

## 许可证与合规

- 项目许可证：Apache License 2.0（详见根目录 `LICENSE`）。
- NOTICE：详见根目录 `NOTICE`，包含第三方依赖的许可与致谢信息。
- 第三方许可与合规要点：
  - civetweb（MIT）：源代码路径 `lib/civetweb/`，作为子项目编译。保留其 `LICENSE.md` 文件与源文件头部的 MIT 许可声明。
  - cJSON（MIT）：源代码路径 `lib/cJSON/`，保留其 `LICENSE` 与源文件头许可。
  - wpa_supplicant（GPLv2）：本项目不分发该工具，仅作为系统运行时依赖调用。

## 发行与部署建议

- 保留并随二进制分发 `LICENSE` 与 `NOTICE` 文件；
- 保留 `lib/cJSON/LICENSE` 与 `lib/civetweb/LICENSE.md` 等第三方许可文件；
- 保留相关第三方库的版权与许可声明。
