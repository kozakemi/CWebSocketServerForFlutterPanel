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
 * @file brightness_scheduler.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 亮度模块WebSocket消息调度器
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "brightness_scheduler.h"
#include "../../protocol/protocol_utils.h"
#include "brightness_def.h"
#include "protocol/brightness_set.h"
#include "protocol/brightness_status.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 亮度请求类型与桥接函数映射
 */
typedef struct
{
    const char *request;  ///< 请求类型字符串
    const char *response; ///< 响应类型字符串
    void (*bridge)(struct mg_connection *conn, const char *response_type, const char *request_id,
                   cJSON *data); ///< JSON与结构体转换桥接函数
} brightness_dispatch;

/**
 * @brief brightness_set请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象
 */
static void bridge_brightness_set(struct mg_connection *conn, const char *response_type,
                                  const char *request_id, cJSON *data)
{
    brightness_set_req_t req = {0};
    cJSON *brightness_item = data ? cJSON_GetObjectItem(data, "brightness") : NULL;
    if (brightness_item && cJSON_IsNumber(brightness_item))
    {
        req.brightness = brightness_item->valueint;
        req.valid = true;
    }

    brightness_set_resp_t resp = brightness_set(&req);

    cJSON *response =
        protocol_create_response(response_type, request_id, (resp.error == BRIGHTNESS_ERR_OK),
                                 resp.error);
    if (response)
    {
        if (resp.error == BRIGHTNESS_ERR_OK)
        {
            cJSON *res_data = cJSON_GetObjectItem(response, "data");
            if (res_data)
            {
                cJSON_AddNumberToObject(res_data, "brightness", resp.brightness);
            }
        }
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}

/**
 * @brief brightness_status请求的桥接函数
 *
 * @param conn 连接指针
 * @param response_type 响应类型
 * @param request_id 请求ID
 * @param data JSON数据对象（未使用）
 */
static void bridge_brightness_status(struct mg_connection *conn, const char *response_type,
                                     const char *request_id, cJSON *data)
{
    (void)data;
    brightness_status_resp_t resp = brightness_status();

    cJSON *response =
        protocol_create_response(response_type, request_id, (resp.error == BRIGHTNESS_ERR_OK),
                                 resp.error);
    if (response)
    {
        cJSON *res_data = cJSON_GetObjectItem(response, "data");
        if (res_data)
        {
            cJSON_AddNumberToObject(res_data, "brightness", resp.brightness);
        }
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}

/* ---- 调度表 ---- */

static brightness_dispatch brightness_dispatch_table[] = {
    {"brightness_status_request", "brightness_status_response", bridge_brightness_status},
    {"brightness_set_request", "brightness_set_response", bridge_brightness_set},
};
#define BRIGHTNESS_DISPATCH_TABLE_SIZE                                                              \
    (sizeof(brightness_dispatch_table) / sizeof(brightness_dispatch_table[0]))

/**
 * @brief 亮度模块消息调度入口
 *
 * 根据JSON中的type字段分发到对应的桥接函数处理。
 *
 * @param conn WebSocket连接指针
 * @param root 解析后的JSON根对象
 */
void brightness_scheduler(struct mg_connection *conn, cJSON *root)
{
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "type");

    if (!cJSON_IsString(type_item) || !type_item->valuestring)
    {
        fprintf(stderr, "缺少或无效的 'type' 字段\n");
        return;
    }

    const char *request_id = protocol_get_request_id(root);
    cJSON *data = cJSON_GetObjectItem(root, "data");

    for (size_t i = 0; i < BRIGHTNESS_DISPATCH_TABLE_SIZE; i++)
    {
        if (strcmp(type_item->valuestring, brightness_dispatch_table[i].request) == 0)
        {
            if (brightness_dispatch_table[i].bridge != NULL)
            {
                brightness_dispatch_table[i].bridge(conn, brightness_dispatch_table[i].response,
                                                    request_id, data);
            }
            return;
        }
    }
}
