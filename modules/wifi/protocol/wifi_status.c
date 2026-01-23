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
#include "../../../protocol/protocol_utils.h"
#include "../wifi_def.h"
#include "../impl/wifi_impl.h"
#include "../wifi_scheduler.h"
#include "cJSON.h"
#include "civetweb.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief 获取wifi状态协议处理
 *
 * @param conn WebSocket 连接指针
 * @param index 调度索引
 * @param root JSON 根对象指针
 */
void wifi_status(struct mg_connection *conn, size_t index, cJSON *root)
{
    wifi_error_t ret = WIFI_ERR_OK;
    wifi_status_info status = {0};

    // 调用业务层执行操作
    ret = wifi_impl_get_status(&status);

    // 获取request_id
    const char *request_id = protocol_get_request_id(root);
    const char *response_type = wifi_dispatch_get_by_index(index)->response;

    // 构建响应
    cJSON *response = protocol_create_response(response_type, request_id, (ret == WIFI_ERR_OK), ret);
    if (response)
    {
        cJSON *res_data = cJSON_GetObjectItem(response, "data");
        if (res_data)
        {
            cJSON_AddBoolToObject(res_data, "enable", status.enable);
            cJSON_AddBoolToObject(res_data, "connected", status.connected);
            cJSON_AddStringToObject(res_data, "ssid", status.ssid ? status.ssid : "");
            cJSON_AddStringToObject(res_data, "bssid", status.bssid ? status.bssid : "");
            cJSON_AddStringToObject(res_data, "interface", status.interface ? status.interface : "");
            cJSON_AddStringToObject(res_data, "ip", status.ip ? status.ip : "");
            cJSON_AddNumberToObject(res_data, "signal", status.signal);
            cJSON_AddStringToObject(res_data, "security", status.security ? status.security : "");
            cJSON_AddNumberToObject(res_data, "channel", status.channel);
            cJSON_AddNumberToObject(res_data, "frequency_mhz", status.frequency_mhz);
        }

        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }

    // 释放业务层分配的内存
    wifi_impl_status_free(&status);
}