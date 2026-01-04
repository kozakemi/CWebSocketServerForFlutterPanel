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

#include "wifi_scheduler.h"
#include "cJSON.h"
#include "fun/wifi_connect.h"
#include "fun/wifi_disconnect.h"
#include "fun/wifi_enable.h"
#include "fun/wifi_scan.h"
#include "fun/wifi_status.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief wifi调度数组定义
 *
 */
wifi_dispatch wifi_dispatch_table[] = {
    {"wifi_enable_request", "wifi_enable_response", wifi_enable}, // 开关wifi
    {"wifi_status_request", "wifi_status_response", wifi_status}, // 获取wifi已连接wifi状态
    {"wifi_scan_request", "wifi_scan_response", wifi_scan},       // 扫描wifi
    {"wifi_connect_request", "wifi_connect_response", wifi_connect},          // 连接wifi
    {"wifi_disconnect_request", "wifi_disconnect_response", wifi_disconnect}, // 断开wifi
};

#define wifi_dispatch_table_LEN (sizeof(wifi_dispatch_table) / sizeof(wifi_dispatch_table[0]))

/**
 * @brief 获取wifi调度
 *
 * @param index
 * @return wifi_dispatch*
 */
wifi_dispatch *wifi_dispatch_get_by_index(size_t index)
{
    // index is 0..LEN-1; LEN is out-of-bounds
    if (index >= wifi_dispatch_table_LEN)
    {
        return NULL;
    }
    return &wifi_dispatch_table[index];
}

/**
 * @brief wifi 任务调度器
 *
 * @details 不需要释放root，外层释放
 *
 * @param conn
 * @param root
 */
void wifi_scheduler(struct mg_connection *conn, cJSON *root)
{
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "type");

    if (!cJSON_IsString(type_item) || !type_item->valuestring)
    {
        fprintf(stderr, "缺少或无效的 'type' 字段\n");
        return;
    }

    for (size_t i = 0; i < wifi_dispatch_table_LEN; i++)
    {
        if (strcmp(type_item->valuestring, wifi_dispatch_table[i].request) == 0)
        {
            if (wifi_dispatch_table[i].handler != NULL)
            {
                wifi_dispatch_table[i].handler(conn, i, root);
            }
            return;
        }
    }
}