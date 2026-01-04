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

#include "brightness_scheduler.h"
#include "cJSON.h"
#include "fun/brightness_set.h"
#include "fun/brightness_status.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/**
 * @brief 亮度调度数组定义
 *
 */
brightness_dispatch brightness_dispatch_table[] = {
    {"brightness_status_request", "brightness_status_response", brightness_status},
    {"brightness_set_request", "brightness_set_response", brightness_set},
};
#define BRIGHTNESS_DISPATCH_TABLE_SIZE                                                             \
    (sizeof(brightness_dispatch_table) / sizeof(brightness_dispatch_table[0]))

/**
 * @brief 根据索引获取亮度调度项
 *
 * @param index 调度项索引
 * @return brightness_dispatch* 调度项指针
 */
brightness_dispatch *brightness_dispatch_get_by_index(size_t index)
{
    if (index >= BRIGHTNESS_DISPATCH_TABLE_SIZE)
    {
        return NULL;
    }
    return &brightness_dispatch_table[index];
}
/**
 * @brief 亮度调度器
 *
 * @param conn WebSocket连接指针
 * @param root JSON根对象指针
 */
void brightness_scheduler(struct mg_connection *conn, cJSON *root)
{
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "type");

    if (!cJSON_IsString(type_item) || !type_item->valuestring)
    {
        fprintf(stderr, "缺少或无效的 'type' 字段\n");
        return;
    }

    for (size_t i = 0; i < BRIGHTNESS_DISPATCH_TABLE_SIZE; i++)
    {
        if (strcmp(type_item->valuestring, brightness_dispatch_table[i].request) == 0)
        {
            if (brightness_dispatch_table[i].handler != NULL)
            {
                brightness_dispatch_table[i].handler(conn, i, root);
            }
            return;
        }
    }
}