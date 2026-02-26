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

#include "wifi_disconnect.h"
#include "../../../protocol/protocol_utils.h"
#include "../wifi_def.h"
#include "../impl/wifi_impl.h"
#include "../wifi_scheduler.h"
#include "cJSON.h"
#include "civetweb.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief wifi断开连接协议处理
 *
 * @param conn WebSocket 连接指针
 * @param index 调度数组索引值
 * @param root json对象
 */
void wifi_disconnect(struct mg_connection *conn, size_t index, cJSON *root)
{
    wifi_error_t ret = WIFI_ERR_BAD_REQUEST;
    const char *ssid = NULL;

    // 解析请求
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *ssid_item = data ? cJSON_GetObjectItem(data, "ssid") : NULL;
    if (ssid_item && cJSON_IsString(ssid_item) && ssid_item->valuestring)
    {
        ssid = ssid_item->valuestring;
    }

    // 调用业务层执行操作（ssid可为NULL，表示断开当前连接）
    ret = wifi_impl_disconnect(ssid);

    // 获取request_id
    const char *request_id = protocol_get_request_id(root);
    const char *response_type = wifi_dispatch_get_by_index(index)->response;

    // 构建响应
    cJSON *response = protocol_create_response(response_type, request_id, (ret == WIFI_ERR_OK), ret);
    if (response)
    {
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}