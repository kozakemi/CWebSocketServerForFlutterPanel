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
 * @file brightness_set.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 设置亮度协议处理
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "brightness_set.h"
#include "../impl/brightness_impl.h"

/**
 * @brief 处理设置亮度请求
 *
 * @param req 设置亮度请求结构体
 * @return brightness_set_resp_t 设置亮度响应
 */
brightness_set_resp_t brightness_set(const brightness_set_req_t *req)
{
    brightness_set_resp_t resp = {0};
    if (!req || !req->valid)
    {
        resp.error = BRIGHTNESS_ERR_BAD_REQUEST;
        return resp;
    }
    resp.error = brightness_impl_set(req->brightness);
    if (resp.error == BRIGHTNESS_ERR_OK)
    {
        resp.brightness = req->brightness;
    }
    return resp;
}
