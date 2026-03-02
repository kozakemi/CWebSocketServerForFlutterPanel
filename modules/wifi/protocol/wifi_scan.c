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
 * @file wifi_scan.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief WiFi扫描协议处理
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "wifi_scan.h"
#include "../impl/wifi_impl.h"

/**
 * @brief 处理WiFi扫描请求
 *
 * @param req 扫描请求结构体（可为NULL）
 * @return wifi_scan_resp_t 扫描结果响应
 */
wifi_scan_resp_t wifi_scan(const wifi_scan_req_t *req)
{
    wifi_scan_resp_t resp = {0};
    resp.error = wifi_impl_scan(req ? req->rescan : false, &resp.result);
    return resp;
}
