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

#include "wifi_enable.h"
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
/**
 * @brief wifi开关数据结构体
 *
 */
typedef struct
{
    bool enable;
} wifi_enable_data;

/**
 * @brief wifi开关请求结构体
 *
 */
typedef struct
{
    char *type;
    char *request_id;
    wifi_enable_data data;
} wifi_enable_req;

/************* 响应 ***************/
/**
 * @brief wifi开关响应结构体
 *
 */
typedef struct
{
    char *type;
    char *request_id;
    bool success;
    int error;
    wifi_enable_data data;
} wifi_enable_res;

/**
 * @brief 启用或禁用Wi-Fi功能（不保证连接成功）
 *
 * @param is_enable true：启用Wi-Fi射频并允许连接；false：断开并禁用自动连接
 * @return wifi_error_t WIFI_ERR_OK 表示操作成功，不代表已联网
 */
static wifi_error_t wifi_enable_execution(bool is_enable)
{
    char command[256];
    int result;

    if (is_enable)
    {
        // 启用所有已保存的网络（允许自动连接）
        snprintf(command, sizeof(command), "wpa_cli -i %s enable_network all", WIFI_DEVICE);
        result = system(command);
        if (result != 0)
        {
            return WIFI_ERR_TOOL_ERROR;
        }

        // 触发重新连接（即使无网络也不报错）
        snprintf(command, sizeof(command), "wpa_cli -i %s reconnect", WIFI_DEVICE);
        result = system(command);
        // 注意：reconnect 在无网络时也返回 0，这是正常的
        if (result != 0)
        {
            return WIFI_ERR_TOOL_ERROR;
        }
    }
    else
    {
        // 禁用所有网络
        snprintf(command, sizeof(command), "wpa_cli -i %s disable_network all", WIFI_DEVICE);
        system(command); // 忽略返回值

        // 断开当前连接
        snprintf(command, sizeof(command), "wpa_cli -i %s disconnect", WIFI_DEVICE);
        system(command);
    }

    printf("wifi_enable: Wi-Fi %s\n", is_enable ? "enabled" : "disabled");
    return WIFI_ERR_OK; // 操作成功，不管是否连上
}

/**
 * @brief wifi开关协议处理
 *
 * @param conn WebSocket 连接指针
 * @param index 调度数组索引值
 * @param root json对象
 */
void wifi_enable(struct mg_connection *conn, size_t index, cJSON *root)
{
    int ret = 0;
    // 使用局部变量避免多线程竞争
    wifi_enable_req req_instance = {0};
    wifi_enable_res res_instance = {0};

    /*
     * {
     *    "type": "wifi_enable_request",
     *    "data": {
     *        "enable": true
     *    }
     * }
     */
    cJSON *type = cJSON_GetObjectItem(root, "type");
    req_instance.type = (cJSON_IsString(type) && type->valuestring) ? type->valuestring : NULL;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    req_instance.request_id =
        (cJSON_IsString(request_id) && request_id->valuestring) ? request_id->valuestring : "";
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *enable_item = data ? cJSON_GetObjectItem(data, "enable") : NULL;
    if (!enable_item || !cJSON_IsBool(enable_item))
    {
        ret = WIFI_ERR_BAD_REQUEST;
        req_instance.data.enable = false;
    }
    else
    {
        req_instance.data.enable = cJSON_IsTrue(enable_item);
        // 执行wifi开关操作
        ret = wifi_enable_execution(req_instance.data.enable);
    }

    // 根据执行结果构建响应数据
    res_instance.type = wifi_dispatch_get_by_index(index)->response; // 使用响应类型
    res_instance.success = (ret == WIFI_ERR_OK);                     // 设置成功标志
    res_instance.error = ret;                                        // 设置错误码
    res_instance.data.enable = req_instance.data.enable;             // 设置数据
    res_instance.request_id = req_instance.request_id;               // 回显request_id

    /**
     * {
     *  "type": "wifi_enable_response",
     *  "success": true,
     *  "error": 0,
     *  "data": { "enabled": true }
     *}
     *
     */
    cJSON *response = cJSON_CreateObject();
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", res_instance.type);
    cJSON_AddStringToObject(response, "request_id", res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", res_instance.success);
    cJSON_AddNumberToObject(response, "error", res_instance.error);
    cJSON_AddBoolToObject(res_data, "enable", res_instance.data.enable);
    cJSON_AddItemToObject(response, "data", res_data);

    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_enable: Failed to print response\n");
        return;
    }
    else
    {
        printf("wifi_enable: %s\n", response_str);
        int n = ws_send_text(conn, response_str);
        if (n < 0)
        {
            printf("wifi_enable: Failed to write response\n");
        }
        cJSON_free(response_str);
    }
    cJSON_Delete(response);
}