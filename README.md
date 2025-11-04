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
- libwebsockets - WebSocket协议实现
- cJSON - JSON解析和生成
- wpa_supplicant - WiFi管理工具

## 许可证与合规

- 项目许可证：Apache License 2.0（详见根目录 `LICENSE`）。
- NOTICE：详见根目录 `NOTICE`，包含第三方依赖的许可与致谢信息。
- 第三方许可与合规要点：
  - libwebsockets（LGPL-2.1）：本项目通过编译参数 `-lwebsockets` 动态链接系统库。动态链接通常满足 LGPL 的再链接要求；请保留库的版权与许可说明。若改为静态链接，请确保提供可再链接的目标文件或等效措施以满足 LGPL 要求。
  - cJSON（MIT）：源代码路径 `lib/cJSON/`，保留其 `LICENSE` 与源文件头许可。
  - wpa_supplicant（GPLv2）：本项目不分发该工具，仅作为系统运行时依赖调用。

## 发行与部署建议

- 保留并随二进制分发 `LICENSE` 与 `NOTICE` 文件；
- 保留 `lib/cJSON/LICENSE` 与相关第三方许可文件；
- 如在不同发行版使用 `libwebsockets`，请按系统提供者要求保留并提供该库的许可文档；
- 如需替换或使用修改版 `libwebsockets`，可设置环境变量 `LD_LIBRARY_PATH` 指向自定义库路径或按系统方式安装库以实现再链接。
