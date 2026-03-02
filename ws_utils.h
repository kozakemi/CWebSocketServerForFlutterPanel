/*
Copyright 2025 kozakemi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * @file ws_utils.h
 * @author kozakemi (kozakemi@gmail.com)
 * @brief WebSocket工具函数声明
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#ifndef WS_UTILS_H
#define WS_UTILS_H

#include "civetweb.h"

/**
 * @brief 通过WebSocket发送UTF-8文本消息
 *
 * 使用mg_websocket_write()发送文本消息。
 *
 * @param conn WebSocket连接指针
 * @param text 要发送的文本
 * @return int mg_websocket_write()的返回值，失败返回-1
 */
int ws_send_text(struct mg_connection *conn, const char *text);

#endif
