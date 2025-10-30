#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/cJSON/cJSON.h"
#include "wifi_def.h"
#include "wifi_scheduler.h"
#include "fun/wifi_enable.h"
#include "fun/wifi_status.h"
#include "fun/wifi_scan.h"
#include "fun/wifi_connect.h"
#include "fun/wifi_disconnect.h"
/**
 * @brief wifi调度数组定义
 *
 */
wifi_dispatch wifi_dispatch_array[] =
    {
        {"wifi_enable_request", "wifi_enable_response", wifi_enable},  // 开关wifi
        {"wifi_status_request", "wifi_status_response", wifi_status},  // 获取wifi已连接wifi状态
        {"wifi_scan_request", "wifi_scan_response", wifi_scan},        // 扫描wifi
        {"wifi_connect_request", "wifi_connect_response", wifi_connect},       // 连接wifi
        {"wifi_disconnect_request", "wifi_disconnect_response", wifi_disconnect}, // 断开wifi
};

#define WIFI_DISPATCH_ARRAY_LEN (sizeof(wifi_dispatch_array) / sizeof(wifi_dispatch_array[0]))

/**
 * @brief 获取wifi调度
 *
 * @param index
 * @return wifi_dispatch*
 */
wifi_dispatch *wifi_dispatch_get_by_index(size_t index)
{
    if (index > WIFI_DISPATCH_ARRAY_LEN)
    {
        return NULL;
    }
    return &wifi_dispatch_array[index];
}

/**
 * @brief wifi 任务调度器
 *
 * @details 不需要释放root，外层释放
 */
void wifi_scheduler(struct lws *wsi,cJSON *root)
{
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "type");

    if (!cJSON_IsString(type_item) || !type_item->valuestring)
    {
        fprintf(stderr, "缺少或无效的 'type' 字段\n");
        return;
    }

    for (size_t i = 0; i < WIFI_DISPATCH_ARRAY_LEN; i++)
    {
        if (strcmp(type_item->valuestring, wifi_dispatch_array[i].request) == 0)
        {
            if (wifi_dispatch_array[i].handler != NULL)
            {
                wifi_dispatch_array[i].handler(wsi,i, root);
            }
            return;
        }
    }
}