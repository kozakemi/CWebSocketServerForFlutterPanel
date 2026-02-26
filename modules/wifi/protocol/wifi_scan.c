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

#include "wifi_scan.h"
#include "../../../protocol/protocol_utils.h"
#include "../wifi_def.h"
#include "../impl/wifi_impl.h"
#include "../wifi_scheduler.h"
#include "cJSON.h"
#include "civetweb.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief 扫描WiFi网络协议处理
 *
 * @param conn WebSocket 连接指针
 * @param index 调度索引
 * @param root JSON 根对象指针
 */
void wifi_scan(struct mg_connection *conn, size_t index, cJSON *root)
{
    wifi_error_t ret = WIFI_ERR_BAD_REQUEST;
    bool rescan = false;
    wifi_scan_result scan_result = {0};

    // 解析请求
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *rescan_item = data ? cJSON_GetObjectItem(data, "rescan") : NULL;
    rescan = (rescan_item && cJSON_IsBool(rescan_item)) ? cJSON_IsTrue(rescan_item) : false;

    // 调用业务层执行操作
    ret = wifi_impl_scan(rescan, &scan_result);

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
            // 添加网络列表
            cJSON *networks_array = cJSON_CreateArray();
            for (size_t i = 0; i < scan_result.network_count; i++)
            {
                cJSON *network_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(
                    network_obj, "ssid",
                    (scan_result.networks[i].ssid && strlen(scan_result.networks[i].ssid) > 0)
                        ? scan_result.networks[i].ssid
                        : "");
                cJSON_AddStringToObject(network_obj, "bssid", scan_result.networks[i].bssid);
                cJSON_AddNumberToObject(network_obj, "signal", scan_result.networks[i].signal);
                cJSON_AddStringToObject(network_obj, "security", scan_result.networks[i].security);
                cJSON_AddNumberToObject(network_obj, "channel", scan_result.networks[i].channel);
                cJSON_AddNumberToObject(network_obj, "frequency_mhz",
                                        scan_result.networks[i].frequency_mhz);
                cJSON_AddBoolToObject(network_obj, "recorded", scan_result.networks[i].recorded);
                cJSON_AddItemToArray(networks_array, network_obj);
            }
            cJSON_AddItemToObject(res_data, "networks", networks_array);
        }

        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }

    // 释放业务层分配的内存
    wifi_impl_scan_result_free(&scan_result);
}