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

#ifndef PROTOCOL_UTILS_H
#define PROTOCOL_UTILS_H

#include "cJSON.h"
#include "civetweb.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 从JSON对象中获取request_id
 *
 * @param root JSON根对象
 * @return const char* request_id字符串，如果不存在则返回空字符串
 */
const char *protocol_get_request_id(cJSON *root);

/**
 * @brief 创建标准响应JSON对象
 *
 * @param response_type 响应类型字符串
 * @param request_id 请求ID字符串
 * @param success 是否成功
 * @param error_code 错误码
 * @return cJSON* 响应JSON对象，需要调用者释放
 */
cJSON *protocol_create_response(const char *response_type, const char *request_id, bool success,
                                int error_code);

/**
 * @brief 发送响应到WebSocket连接
 *
 * @param conn WebSocket连接指针
 * @param response JSON响应对象
 * @return int 成功返回0，失败返回-1
 */
int protocol_send_response(struct mg_connection *conn, cJSON *response);

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
                                    const char *request_id, bool success, int error_code);

#endif