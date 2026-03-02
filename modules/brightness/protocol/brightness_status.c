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
 * @file brightness_status.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 查询亮度状态协议处理
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "brightness_status.h"
#include "../impl/brightness_impl.h"

/**
 * @brief 处理查询亮度状态请求
 *
 * @return brightness_status_resp_t 亮度状态响应
 */
brightness_status_resp_t brightness_status(void)
{
    brightness_status_resp_t resp = {0};
    resp.error = brightness_impl_get_status(&resp.brightness);
    return resp;
}
