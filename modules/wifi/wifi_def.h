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
 * @file wifi_def.h
 * @author kozakemi (kozakemi@gmail.com)
 * @brief WiFi模块公共定义
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#ifndef WIFI_DEF_H
#define WIFI_DEF_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief WiFi模块错误码
 */
typedef enum
{
    WIFI_ERR_OK = 0,                ///< 成功
    WIFI_ERR_UNKNOWN = -1,          ///< 未知错误
    WIFI_ERR_BAD_REQUEST = 1,       ///< 数据缺失/类型不符
    WIFI_ERR_NOT_SUPPORTED = 2,     ///< 操作不支持
    WIFI_ERR_WIFI_DISABLED = 3,     ///< Wi-Fi已关闭
    WIFI_ERR_ALREADY_CONNECTED = 4, ///< 已连接同一SSID
    WIFI_ERR_NETWORK_NOT_FOUND = 5, ///< 扫描无该SSID
    WIFI_ERR_AUTH_FAILED = 6,       ///< 认证失败/密码错误
    WIFI_ERR_TIMEOUT = 7,           ///< 连接/操作超时
    WIFI_ERR_INTERNAL = 8,          ///< 后端内部错误
    WIFI_ERR_PERMISSION = 9,        ///< 权限不足
    WIFI_ERR_BUSY = 10,             ///< 设备繁忙/正在进行其他操作
    WIFI_ERR_INVALID_SSID = 11,     ///< 无效SSID
    WIFI_ERR_INVALID_PASSWORD = 12, ///< 无效密码
    WIFI_ERR_INTERFACE_DOWN = 13,   ///< 网卡未启动/无效
    WIFI_ERR_TOOL_ERROR = 14,       ///< 底层工具错误（nmcli/wpa_cli）
    WIFI_ERR_CONFIG_ERROR = 15,     ///< 配置存储/读取失败
    WIFI_ERR_IO_ERROR = 16,         ///< I/O异常
    WIFI_ERR_NOT_CONNECTED = 17     ///< 未连接
} wifi_error_t;

/* ---- 数据结构体（供 impl 和 protocol 共用） ---- */

/**
 * @brief WiFi网络信息结构体
 */
typedef struct
{
    char *ssid;        ///< 网络SSID
    char *bssid;       ///< BSSID
    int signal;        ///< 信号强度
    char *security;    ///< 加密方式
    int channel;       ///< 信道
    int frequency_mhz; ///< 频率(MHz)
    bool recorded;     ///< 是否已保存
} wifi_network_info;

/**
 * @brief WiFi状态信息结构体
 */
typedef struct
{
    bool enable;       ///< WiFi是否启用
    bool connected;    ///< 是否已连接
    char *ssid;        ///< 当前SSID
    char *bssid;       ///< BSSID
    char *interface;   ///< 网卡接口名
    char *ip;          ///< IP地址
    int signal;        ///< 信号强度
    char *security;    ///< 加密方式
    int channel;       ///< 信道
    int frequency_mhz; ///< 频率(MHz)
} wifi_status_info;

/**
 * @brief WiFi扫描结果结构体
 */
typedef struct
{
    wifi_network_info *networks; ///< 网络列表
    size_t network_count;        ///< 网络数量
} wifi_scan_result;

/**
 * @brief 启用/禁用WiFi请求结构体
 */
typedef struct
{
    bool enable; ///< 是否启用
    bool valid;  ///< 请求是否有效
} wifi_enable_req_t;

/**
 * @brief 启用/禁用WiFi响应结构体
 */
typedef struct
{
    wifi_error_t error; ///< 错误码
    bool enable;        ///< 当前启用状态
} wifi_enable_resp_t;

/**
 * @brief 查询WiFi状态响应结构体
 */
typedef struct
{
    wifi_error_t error;      ///< 错误码
    wifi_status_info status; ///< 状态信息
} wifi_status_resp_t;

/**
 * @brief WiFi扫描请求结构体
 */
typedef struct
{
    bool rescan; ///< 是否强制重新扫描
} wifi_scan_req_t;

/**
 * @brief WiFi扫描响应结构体
 */
typedef struct
{
    wifi_error_t error;      ///< 错误码
    wifi_scan_result result; ///< 扫描结果
} wifi_scan_resp_t;

/**
 * @brief WiFi连接请求结构体
 */
typedef struct
{
    char ssid[256];     ///< 网络SSID
    char password[256]; ///< 网络密码
    int timeout_ms;     ///< 超时时间(毫秒)
    bool valid;         ///< 请求是否有效
} wifi_connect_req_t;

/**
 * @brief WiFi连接响应结构体
 */
typedef struct
{
    wifi_error_t error; ///< 错误码
} wifi_connect_resp_t;

/**
 * @brief WiFi断开连接请求结构体
 */
typedef struct
{
    char ssid[256]; ///< 要断开的SSID
    bool has_ssid;  ///< 是否指定了SSID
} wifi_disconnect_req_t;

/**
 * @brief WiFi断开连接响应结构体
 */
typedef struct
{
    wifi_error_t error; ///< 错误码
} wifi_disconnect_resp_t;

#endif
