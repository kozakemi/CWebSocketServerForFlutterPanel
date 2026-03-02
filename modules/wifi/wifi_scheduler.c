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
 * @file wifi_scheduler.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief WiFi模块WebSocket消息调度器
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "wifi_scheduler.h"
#include "../../protocol/protocol_utils.h"
#include "impl/wifi_impl.h"
#include "protocol/wifi_connect.h"
#include "protocol/wifi_disconnect.h"
#include "protocol/wifi_enable.h"
#include "protocol/wifi_scan.h"
#include "protocol/wifi_status.h"
#include "wifi_def.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief WiFi请求类型与桥接函数映射
 */
typedef struct
{
    const char *request;  ///< 请求类型字符串
    const char *response; ///< 响应类型字符串
    void (*bridge)(struct mg_connection *conn, const char *response_type, const char *request_id,
                   cJSON *data); ///< JSON与结构体转换桥接函数
} wifi_dispatch;

/**
 * @brief wifi_enable请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象
 */
static void bridge_wifi_enable(struct mg_connection *conn, const char *response_type,
                               const char *request_id, cJSON *data)
{
    wifi_enable_req_t req = {0};
    cJSON *enable_item = data ? cJSON_GetObjectItem(data, "enable") : NULL;
    if (enable_item && cJSON_IsBool(enable_item))
    {
        req.enable = cJSON_IsTrue(enable_item);
        req.valid = true;
    }

    wifi_enable_resp_t resp = wifi_enable(&req);

    cJSON *response = protocol_create_response(response_type, request_id,
                                               (resp.error == WIFI_ERR_OK), resp.error);
    if (response)
    {
        cJSON *res_data = cJSON_GetObjectItem(response, "data");
        if (res_data)
        {
            cJSON_AddBoolToObject(res_data, "enable", resp.enable);
        }
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}

/**
 * @brief wifi_status请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象（未使用）
 */
static void bridge_wifi_status(struct mg_connection *conn, const char *response_type,
                               const char *request_id, cJSON *data)
{
    (void)data;
    wifi_status_resp_t resp = wifi_status();

    cJSON *response = protocol_create_response(response_type, request_id,
                                               (resp.error == WIFI_ERR_OK), resp.error);
    if (response)
    {
        cJSON *res_data = cJSON_GetObjectItem(response, "data");
        if (res_data)
        {
            cJSON_AddBoolToObject(res_data, "enable", resp.status.enable);
            cJSON_AddBoolToObject(res_data, "connected", resp.status.connected);
            cJSON_AddStringToObject(res_data, "ssid", resp.status.ssid ? resp.status.ssid : "");
            cJSON_AddStringToObject(res_data, "bssid", resp.status.bssid ? resp.status.bssid : "");
            cJSON_AddStringToObject(res_data, "interface",
                                    resp.status.interface ? resp.status.interface : "");
            cJSON_AddStringToObject(res_data, "ip", resp.status.ip ? resp.status.ip : "");
            cJSON_AddNumberToObject(res_data, "signal", resp.status.signal);
            cJSON_AddStringToObject(res_data, "security",
                                    resp.status.security ? resp.status.security : "");
            cJSON_AddNumberToObject(res_data, "channel", resp.status.channel);
            cJSON_AddNumberToObject(res_data, "frequency_mhz", resp.status.frequency_mhz);
        }
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }

    wifi_impl_status_free(&resp.status);
}

/**
 * @brief wifi_scan请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象
 */
static void bridge_wifi_scan(struct mg_connection *conn, const char *response_type,
                             const char *request_id, cJSON *data)
{
    wifi_scan_req_t req = {0};
    cJSON *rescan_item = data ? cJSON_GetObjectItem(data, "rescan") : NULL;
    req.rescan = (rescan_item && cJSON_IsBool(rescan_item)) ? cJSON_IsTrue(rescan_item) : false;

    wifi_scan_resp_t resp = wifi_scan(&req);

    cJSON *response = protocol_create_response(response_type, request_id,
                                               (resp.error == WIFI_ERR_OK), resp.error);
    if (response)
    {
        cJSON *res_data = cJSON_GetObjectItem(response, "data");
        if (res_data)
        {
            cJSON *networks_array = cJSON_CreateArray();
            for (size_t i = 0; i < resp.result.network_count; i++)
            {
                cJSON *network_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(
                    network_obj, "ssid",
                    (resp.result.networks[i].ssid && strlen(resp.result.networks[i].ssid) > 0)
                        ? resp.result.networks[i].ssid
                        : "");
                cJSON_AddStringToObject(network_obj, "bssid", resp.result.networks[i].bssid);
                cJSON_AddNumberToObject(network_obj, "signal", resp.result.networks[i].signal);
                cJSON_AddStringToObject(network_obj, "security", resp.result.networks[i].security);
                cJSON_AddNumberToObject(network_obj, "channel", resp.result.networks[i].channel);
                cJSON_AddNumberToObject(network_obj, "frequency_mhz",
                                        resp.result.networks[i].frequency_mhz);
                cJSON_AddBoolToObject(network_obj, "recorded", resp.result.networks[i].recorded);
                cJSON_AddItemToArray(networks_array, network_obj);
            }
            cJSON_AddItemToObject(res_data, "networks", networks_array);
        }
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }

    wifi_impl_scan_result_free(&resp.result);
}

/**
 * @brief wifi_connect请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象
 */
static void bridge_wifi_connect(struct mg_connection *conn, const char *response_type,
                                const char *request_id, cJSON *data)
{
    wifi_connect_req_t req = {0};
    cJSON *ssid_item = data ? cJSON_GetObjectItem(data, "ssid") : NULL;
    cJSON *password_item = data ? cJSON_GetObjectItem(data, "password") : NULL;
    cJSON *timeout_item = data ? cJSON_GetObjectItem(data, "timeout_ms") : NULL;

    if (ssid_item && cJSON_IsString(ssid_item) && ssid_item->valuestring)
    {
        strncpy(req.ssid, ssid_item->valuestring, sizeof(req.ssid) - 1);
        req.ssid[sizeof(req.ssid) - 1] = '\0';
        if (password_item && cJSON_IsString(password_item) && password_item->valuestring)
        {
            strncpy(req.password, password_item->valuestring, sizeof(req.password) - 1);
            req.password[sizeof(req.password) - 1] = '\0';
        }
        req.timeout_ms =
            (timeout_item && cJSON_IsNumber(timeout_item)) ? timeout_item->valueint : 20000;
        req.valid = true;
    }

    wifi_connect_resp_t resp = wifi_connect(&req);

    cJSON *response = protocol_create_response(response_type, request_id,
                                               (resp.error == WIFI_ERR_OK), resp.error);
    if (response)
    {
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}

/**
 * @brief wifi_disconnect请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象
 */
static void bridge_wifi_disconnect(struct mg_connection *conn, const char *response_type,
                                   const char *request_id, cJSON *data)
{
    wifi_disconnect_req_t req = {0};
    cJSON *ssid_item = data ? cJSON_GetObjectItem(data, "ssid") : NULL;
    if (ssid_item && cJSON_IsString(ssid_item) && ssid_item->valuestring)
    {
        strncpy(req.ssid, ssid_item->valuestring, sizeof(req.ssid) - 1);
        req.ssid[sizeof(req.ssid) - 1] = '\0';
        req.has_ssid = true;
    }

    wifi_disconnect_resp_t resp = wifi_disconnect(&req);

    cJSON *response = protocol_create_response(response_type, request_id,
                                               (resp.error == WIFI_ERR_OK), resp.error);
    if (response)
    {
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}

/**
 * @brief WiFi请求类型与桥接函数映射表
 *
 */
static wifi_dispatch wifi_dispatch_table[] = {
    {"wifi_enable_request", "wifi_enable_response", bridge_wifi_enable},
    {"wifi_status_request", "wifi_status_response", bridge_wifi_status},
    {"wifi_scan_request", "wifi_scan_response", bridge_wifi_scan},
    {"wifi_connect_request", "wifi_connect_response", bridge_wifi_connect},
    {"wifi_disconnect_request", "wifi_disconnect_response", bridge_wifi_disconnect},
};
#define WIFI_DISPATCH_TABLE_LEN (sizeof(wifi_dispatch_table) / sizeof(wifi_dispatch_table[0]))

/**
 * @brief WiFi模块消息调度入口
 *
 * 根据JSON中的type字段分发到对应的桥接函数处理。
 *
 * @param conn WebSocket连接指针
 * @param root 解析后的JSON根对象
 */
void wifi_scheduler(struct mg_connection *conn, cJSON *root)
{
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "type");

    if (!cJSON_IsString(type_item) || !type_item->valuestring)
    {
        fprintf(stderr, "缺少或无效的 'type' 字段\n");
        return;
    }

    const char *request_id = protocol_get_request_id(root);
    cJSON *data = cJSON_GetObjectItem(root, "data");

    for (size_t i = 0; i < WIFI_DISPATCH_TABLE_LEN; i++)
    {
        if (strcmp(type_item->valuestring, wifi_dispatch_table[i].request) == 0)
        {
            if (wifi_dispatch_table[i].bridge != NULL)
            {
                wifi_dispatch_table[i].bridge(conn, wifi_dispatch_table[i].response, request_id,
                                              data);
            }
            return;
        }
    }
}
