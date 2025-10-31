#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../lib/cJSON/cJSON.h"
#include "brightness_scheduler.h"
#include "fun/brightness_status.h"
#include "fun/brightness_set.h"

/**
 * @brief 亮度调度数组定义
 * 
 */
brightness_dispatch brightness_dispatch_table[] = {
    {"brightness_status_request","brightness_status_response",brightness_status},
    {"brightness_set_request","brightness_set_response",brightness_set},
};
#define BRIGHTNESS_DISPATCH_TABLE_SIZE (sizeof(brightness_dispatch_table)/sizeof(brightness_dispatch_table[0]))

/**
 * @brief 根据索引获取亮度调度项
 * 
 * @param index 调度项索引
 * @return brightness_dispatch* 调度项指针
 */
brightness_dispatch *brightness_dispatch_get_by_inedx(size_t index)
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
 * @param wsi WebSocket实例指针
 * @param index 调度项索引
 * @param root JSON根对象指针
 */
void brightness_scheduler(struct lws *wsi,size_t index,cJSON *root)
{
    for (size_t i = 0; i < BRIGHTNESS_DISPATCH_TABLE_SIZE; i++)
    {
        if (strcmp(brightness_dispatch_table[i].request,root->string) == 0)
        {
            if (brightness_dispatch_table[i].handler != NULL)
            {
                brightness_dispatch_table[i].handler(wsi,index,root);
            }
            return;
        }
    }
}