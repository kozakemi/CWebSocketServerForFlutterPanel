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
#include "../../lib/cJSON/cJSON.h"
#include "../wifi_def.h"
#include "../wifi_scheduler.h"
#include <libwebsockets.h>
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

wifi_enable_req wifi_enable_req_instance;
wifi_enable_res wifi_enable_res_instance;

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
            return WIFI_ERR_TOOL_ERROR;

        // 触发重新连接（即使无网络也不报错）
        snprintf(command, sizeof(command), "wpa_cli -i %s reconnect", WIFI_DEVICE);
        result = system(command);
        // 注意：reconnect 在无网络时也返回 0，这是正常的
        if (result != 0)
            return WIFI_ERR_TOOL_ERROR;
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
 * @param wsi WebSocket 实例指针
 * @param index 调度数组索引值
 * @param root json对象
 */
void wifi_enable(struct lws *wsi, size_t index, cJSON *root)
{
    int ret = 0;
    /*
     * {
     *    "type": "wifi_enable_request",
     *    "data": {
     *        "enable": true
     *    }
     * }
     */
    cJSON *type = cJSON_GetObjectItem(root, "type");
    wifi_enable_req_instance.type = type->valuestring;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    wifi_enable_req_instance.request_id = request_id->valuestring;
    cJSON *data = cJSON_GetObjectItem(root, "data");
    wifi_enable_req_instance.data.enable = cJSON_IsTrue(cJSON_GetObjectItem(data, "enable"));

    // 执行wifi开关操作
    ret = wifi_enable_execution(wifi_enable_req_instance.data.enable);

    // 根据执行结果构建响应数据
    wifi_enable_res_instance.type = wifi_dispatch_get_by_index(index)->response; // 使用响应类型
    wifi_enable_res_instance.success = (ret == WIFI_ERR_OK); // 设置成功标志
    wifi_enable_res_instance.error = ret;                    // 设置错误码
    wifi_enable_res_instance.data.enable = wifi_enable_req_instance.data.enable; // 设置数据
    wifi_enable_res_instance.request_id = wifi_enable_req_instance.request_id;   // 回显request_id

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
    cJSON_AddStringToObject(response, "type", wifi_enable_res_instance.type);
    cJSON_AddStringToObject(response, "request_id", wifi_enable_res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", wifi_enable_res_instance.success);
    cJSON_AddNumberToObject(response, "error", wifi_enable_res_instance.error);
    cJSON_AddBoolToObject(res_data, "enable", wifi_enable_res_instance.data.enable);
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
        unsigned char buf[LWS_PRE + strlen(response_str)];
        memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
        int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        if (n < 0)
        {
            printf("wifi_enable: Failed to write response\n");
        }
        cJSON_free(response_str);
    }
    cJSON_Delete(response);
}