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
 * @file wifi_status.h
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 查询WiFi状态协议声明
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#ifndef WIFI_STATUS_H
#define WIFI_STATUS_H

#include "../wifi_def.h"

/**
 * @brief 处理查询WiFi状态请求
 *
 * @return wifi_status_resp_t 状态响应结构体
 */
wifi_status_resp_t wifi_status(void);

#endif
