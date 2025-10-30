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
