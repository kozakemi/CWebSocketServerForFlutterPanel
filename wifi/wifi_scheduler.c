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
 * 调度并路由 Wi‑Fi 相关请求到注册的处理函数。
 *
 * 从提供的 JSON 对象中读取 "type" 字段，查找并匹配内部 dispatch 表中的请求字符串；在找到匹配且对应处理器存在时调用该处理器。若 "type" 缺失或不是字符串，函数会向 stderr 输出错误并返回。该函数不释放传入的 `root`。
 *
 * @param conn 传入的连接对象指针，作为参数转发给相应的处理器。
 * @param root 已解析的 JSON 根对象，必须包含名为 "type" 的字符串字段；调用者负责释放该对象（函数不释放）。
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