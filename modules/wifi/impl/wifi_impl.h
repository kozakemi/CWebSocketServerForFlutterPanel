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

#ifndef WIFI_IMPL_H
#define WIFI_IMPL_H

#include "../wifi_def.h"
#include <stdbool.h>
#include <stddef.h>

#define WIFI_DEVICE "wlan0"
/**
 * @brief WiFi网络信息结构体
 */
typedef struct
{
    char *ssid;
    char *bssid;
    int signal;
    char *security;
    int channel;
    int frequency_mhz;
    bool recorded;
} wifi_network_info;

/**
 * @brief WiFi状态信息结构体
 */
typedef struct
{
    bool enable;
    bool connected;
    char *ssid;
    char *bssid;
    char *interface;
    char *ip;
    int signal;
    char *security;
    int channel;
    int frequency_mhz;
} wifi_status_info;

/**
 * @brief WiFi扫描结果结构体
 */
typedef struct
{
    wifi_network_info *networks;
    size_t network_count;
} wifi_scan_result;

/**
 * @brief 启用或禁用Wi-Fi功能（不保证连接成功）
 *
 * @param is_enable true：启用Wi-Fi射频并允许连接；false：断开并禁用自动连接
 * @return wifi_error_t WIFI_ERR_OK 表示操作成功，不代表已联网
 */
wifi_error_t wifi_impl_enable(bool is_enable);

/**
 * @brief 执行WiFi扫描操作
 *
 * @param rescan 是否强制重新扫描
 * @param result 扫描结果指针（由函数分配内存，调用者需要释放）
 * @return wifi_error_t 错误码
 */
wifi_error_t wifi_impl_scan(bool rescan, wifi_scan_result *result);

/**
 * @brief 释放扫描结果内存
 *
 * @param result 扫描结果指针
 */
void wifi_impl_scan_result_free(wifi_scan_result *result);

/**
 * @brief 获取WiFi连接状态
 *
 * @param status 状态信息指针（由函数分配内存，调用者需要释放）
 * @return wifi_error_t 错误码
 */
wifi_error_t wifi_impl_get_status(wifi_status_info *status);

/**
 * @brief 释放状态信息内存
 *
 * @param status 状态信息指针
 */
void wifi_impl_status_free(wifi_status_info *status);

/**
 * @brief 连接WiFi网络
 *
 * @param ssid 网络SSID
 * @param password 网络密码（可为NULL或空字符串）
 * @param timeout_ms 超时时间（毫秒），0表示使用默认值
 * @return wifi_error_t 错误码
 */
wifi_error_t wifi_impl_connect(const char *ssid, const char *password, int timeout_ms);

/**
 * @brief 断开WiFi连接
 *
 * @param ssid 要断开的网络SSID（可为NULL，表示断开当前连接）
 * @return wifi_error_t 错误码
 */
wifi_error_t wifi_impl_disconnect(const char *ssid);

#endif