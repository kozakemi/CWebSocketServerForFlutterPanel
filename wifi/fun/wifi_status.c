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

#include "wifi_status.h"
#include "../../ws_utils.h"
#include "../wifi_def.h"
#include "../wifi_scheduler.h"
#include "cJSON.h"
#include "civetweb.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************* 请求 ***************/

typedef struct
{
    char *type;
    char *request_id;
} wifi_status_req;

/************* 响应 ***************/
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

} wifi_status_data;

typedef struct
{
    char *type;
    char *request_id;
    bool success;
    int error;
    wifi_status_data data;
} wifi_status_res;

/**
 * @brief 执行连接状态操作
 *
 * @param res_instance 响应实例指针
 * @return wifi_error_t
 */
static wifi_error_t wifi_status_execution(wifi_status_res *res_instance)
{
    // 初始化数据结构
    res_instance->data.enable = false;
    res_instance->data.connected = false;
    res_instance->data.ssid = NULL;
    res_instance->data.bssid = NULL;
    res_instance->data.interface = WIFI_DEVICE;
    res_instance->data.ip = NULL;
    res_instance->data.signal = 0;
    res_instance->data.security = NULL;
    res_instance->data.channel = 0;
    res_instance->data.frequency_mhz = 0;

    FILE *fp;
    char buffer[256];
    char command[256];

    // 检查WiFi是否启用
    snprintf(command, sizeof(command),
             "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL)
    {
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            // 如果能获取到状态信息，则认为WiFi已启用
            res_instance->data.enable =
                (strstr(buffer, "COMPLETED") != NULL || strstr(buffer, "ASSOCIATED") != NULL ||
                 strstr(buffer, "ASSOCIATING") != NULL || strstr(buffer, "SCANNING") != NULL);
            // 如果已连接
            res_instance->data.connected = (strstr(buffer, "COMPLETED") != NULL);
        }
        pclose(fp);
    }

    // 获取SSID
    if (res_instance->data.enable)
    {
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep '^ssid=' | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                res_instance->data.ssid = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取BSSID
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep bssid | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                res_instance->data.bssid = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取信号强度
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s signal_poll 2>/dev/null | grep RSSI | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                res_instance->data.signal = atoi(buffer);
            }
            pclose(fp);
        }

        // 获取IP地址
        snprintf(command, sizeof(command),
                 "ip addr show %s 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d/ -f1",
                 WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                res_instance->data.ip = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取安全信息
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep key_mgmt | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                res_instance->data.security = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取频道
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep channel | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                res_instance->data.channel = atoi(buffer);
            }
            pclose(fp);
        }

        // 获取频率
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep freq | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                res_instance->data.frequency_mhz = atoi(buffer);
            }
            pclose(fp);
        }
    }

    return WIFI_ERR_OK;
}

/**
 * @brief 获取wifi状态
 *
 * @param conn WebSocket 连接指针
 * @param index 调度索引
 * @param root JSON 根对象指针
 */
void wifi_status(struct mg_connection *conn, size_t index, cJSON *root)
{
    int ret = 0;
    // 使用局部变量避免多线程竞争
    wifi_status_req req_instance = {0};
    wifi_status_res res_instance = {0};

    // { "type": "wifi_status_request", "data": {} }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    req_instance.type = (cJSON_IsString(type) && type->valuestring) ? type->valuestring : NULL;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    req_instance.request_id =
        (cJSON_IsString(request_id) && request_id->valuestring) ? request_id->valuestring : "";

    ret = wifi_status_execution(&res_instance); // 执行状态获取操作

    // 根据执行结果构建响应数据
    res_instance.type = wifi_dispatch_get_by_index(index)->response; // 使用响应类型
    res_instance.success = (ret == WIFI_ERR_OK);                     // 设置成功标志
    res_instance.error = ret;                                        // 设置错误码
    res_instance.request_id = req_instance.request_id;               // 回显request_id

    /**
     * {
     *  "type": "wifi_status_response",
     *  "success": true,
     *  "error": 0,
     *  "data": {
     *      "enable": true,
     *      "connected": true,
     *      "ssid": "test_wifi",
     *      "bssid": "xx:xx:xx:xx:xx:xx",
     *      "interface": "wlan0",
     *      "ip": "192.168.1.100",
     *      "signal": -50,
     *      "security": "WPA2-PSK",
     *      "channel": 6,
     *      "frequency_mhz": 2437
     *  }
     */
    cJSON *response = cJSON_CreateObject();
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", res_instance.type);
    cJSON_AddStringToObject(response, "request_id", res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", res_instance.success);
    cJSON_AddNumberToObject(response, "error", res_instance.error);

    cJSON_AddBoolToObject(res_data, "enable", res_instance.data.enable);
    cJSON_AddBoolToObject(res_data, "connected", res_instance.data.connected);

    if (res_instance.data.ssid)
    {
        cJSON_AddStringToObject(res_data, "ssid", res_instance.data.ssid);
    }
    else
    {
        cJSON_AddStringToObject(res_data, "ssid", "");
    }

    if (res_instance.data.bssid)
    {
        cJSON_AddStringToObject(res_data, "bssid", res_instance.data.bssid);
    }
    else
    {
        cJSON_AddStringToObject(res_data, "bssid", "");
    }

    cJSON_AddStringToObject(res_data, "interface", res_instance.data.interface);

    if (res_instance.data.ip)
    {
        cJSON_AddStringToObject(res_data, "ip", res_instance.data.ip);
    }
    else
    {
        cJSON_AddStringToObject(res_data, "ip", "");
    }

    cJSON_AddNumberToObject(res_data, "signal", res_instance.data.signal);

    if (res_instance.data.security)
    {
        cJSON_AddStringToObject(res_data, "security", res_instance.data.security);
    }
    else
    {
        cJSON_AddStringToObject(res_data, "security", "");
    }

    cJSON_AddNumberToObject(res_data, "channel", res_instance.data.channel);
    cJSON_AddNumberToObject(res_data, "frequency_mhz", res_instance.data.frequency_mhz);

    cJSON_AddItemToObject(response, "data", res_data);

    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_status: Failed to print response\n");
        cJSON_Delete(response);
        return;
    }
    else
    {
        printf("wifi_status: %s\n", response_str);
        int n = ws_send_text(conn, response_str);
        if (n < 0)
        {
            printf("wifi_status: Failed to write response\n");
        }
        cJSON_free(response_str);
    }

    // 释放动态分配的内存
    if (res_instance.data.ssid)
    {
        free(res_instance.data.ssid);
    }
    if (res_instance.data.bssid)
    {
        free(res_instance.data.bssid);
    }
    if (res_instance.data.ip)
    {
        free(res_instance.data.ip);
    }
    if (res_instance.data.security)
    {
        free(res_instance.data.security);
    }
    cJSON_Delete(response);
}