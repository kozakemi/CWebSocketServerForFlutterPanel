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
 * @file protocol_utils.c
 * @author kozakemi (kozakemi@gmail.com)
 * @brief 协议工具函数实现
 * @date 2026-03-02
 *
 * @copyright Copyright (c) 2026 kozakemi
 *
 */
#include "protocol_utils.h"
#include "../ws_utils.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 从JSON对象中获取request_id
 *
 * @param root JSON根对象
 * @return const char* request_id字符串，不存在则返回空字符串
 */
const char *protocol_get_request_id(cJSON *root)
{
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    if (request_id && cJSON_IsString(request_id) && request_id->valuestring)
    {
        return request_id->valuestring;
    }
    return "";
}

/**
 * @brief 创建标准响应JSON对象
 *
 * @param response_type 响应类型字符串
 * @param request_id 请求ID字符串
 * @param success 是否成功
 * @param error_code 错误码
 * @return cJSON* 响应JSON对象，需调用者释放
 */
cJSON *protocol_create_response(const char *response_type, const char *request_id, bool success,
                                int error_code)
{
    cJSON *response = cJSON_CreateObject();
    if (!response)
    {
        return NULL;
    }

    cJSON_AddStringToObject(response, "type", response_type);
    cJSON_AddStringToObject(response, "request_id", request_id ? request_id : "");
    cJSON_AddBoolToObject(response, "success", success);
    cJSON_AddNumberToObject(response, "error", error_code);

    // 添加空的data对象
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "data", data);

    return response;
}

/**
 * @brief 发送响应到WebSocket连接
 *
 * @param conn WebSocket连接指针
 * @param response JSON响应对象
 * @return int 成功返回0，失败返回-1
 */
int protocol_send_response(struct mg_connection *conn, cJSON *response)
{
    if (!conn || !response)
    {
        return -1;
    }

    char *response_str = cJSON_PrintUnformatted(response);
    if (!response_str)
    {
        printf("protocol_send_response: Failed to print response\n");
        return -1;
    }

    int n = ws_send_text(conn, response_str);
    if (n < 0)
    {
        printf("protocol_send_response: Failed to send response\n");
        cJSON_free(response_str);
        return -1;
    }

    cJSON_free(response_str);
    return 0;
}

/**
 * @brief 创建并发送标准响应
 *
 * @param conn WebSocket连接指针
 * @param response_type 响应类型字符串
 * @param request_id 请求ID字符串
 * @param success 是否成功
 * @param error_code 错误码
 * @return int 成功返回0，失败返回-1
 */
int protocol_send_standard_response(struct mg_connection *conn, const char *response_type,
                                    const char *request_id, bool success, int error_code)
{
    cJSON *response = protocol_create_response(response_type, request_id, success, error_code);
    if (!response)
    {
        return -1;
    }

    int ret = protocol_send_response(conn, response);
    cJSON_Delete(response);
    return ret;
}