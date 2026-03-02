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
 * @file wifi_connect.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief WiFi连接协议处理
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "wifi_connect.h"
#include "../impl/wifi_impl.h"
#include <string.h>

/**
 * @brief 处理WiFi连接请求
 *
 * @param req 连接请求结构体
 * @return wifi_connect_resp_t 连接响应结构体
 */
wifi_connect_resp_t wifi_connect(const wifi_connect_req_t *req)
{
    wifi_connect_resp_t resp = {0};
    if (!req || !req->valid)
    {
        resp.error = WIFI_ERR_BAD_REQUEST;
        return resp;
    }
    const char *password = (strlen(req->password) > 0) ? req->password : NULL;
    resp.error = wifi_impl_connect(req->ssid, password, req->timeout_ms);
    return resp;
}
