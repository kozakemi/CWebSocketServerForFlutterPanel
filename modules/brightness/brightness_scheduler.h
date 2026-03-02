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
 * @file brightness_scheduler.h
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 亮度模块调度器声明
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#ifndef BRIGHTNESS_SCHEDULER_H
#define BRIGHTNESS_SCHEDULER_H

#include "cJSON.h"
#include "civetweb.h"

/**
 * @brief 亮度模块消息调度入口
 *
 * @param conn WebSocket连接指针
 * @param root 解析后的JSON根对象
 */
void brightness_scheduler(struct mg_connection *conn, cJSON *root);

#endif
