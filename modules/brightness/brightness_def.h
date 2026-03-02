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
 * @file brightness_def.h
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 亮度模块公共定义
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#ifndef BRIGHTNESS_DEF_H
#define BRIGHTNESS_DEF_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 亮度模块错误码
 */
typedef enum
{
    BRIGHTNESS_ERR_OK = 0,            ///< 成功
    BRIGHTNESS_ERR_UNKNOWN = -1,      ///< 未知错误
    BRIGHTNESS_ERR_BAD_REQUEST = 1,   ///< 数据缺失/类型不符
    BRIGHTNESS_ERR_INVALID_VALUE = 2, ///< 亮度值无效（超出0-100范围）
    BRIGHTNESS_ERR_NOT_SUPPORTED = 3, ///< 设备不支持亮度调节
    BRIGHTNESS_ERR_PERMISSION = 4,    ///< 权限不足
    BRIGHTNESS_ERR_DEVICE_ERROR = 5,  ///< 硬件设备错误
    BRIGHTNESS_ERR_INTERNAL = 6       ///< 后端内部错误
} brightness_error_t;

/**
 * @brief 设置亮度请求结构体
 */
typedef struct
{
    int brightness; ///< 亮度值（0-100）
    bool valid;    ///< 请求是否有效
} brightness_set_req_t;

/**
 * @brief 设置亮度响应结构体
 */
typedef struct
{
    brightness_error_t error; ///< 错误码
    int brightness;          ///< 实际设置的亮度值
} brightness_set_resp_t;

/**
 * @brief 查询亮度响应结构体
 */
typedef struct
{
    brightness_error_t error; ///< 错误码
    int brightness;          ///< 当前亮度值（0-100）
} brightness_status_resp_t;

#endif
