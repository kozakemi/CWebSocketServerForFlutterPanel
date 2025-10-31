#ifndef __WIFI_SCHEDULER_H__
#define __WIFI_SCHEDULER_H__ 
#include <stdint.h>
#include <libwebsockets.h>
#include "../lib/cJSON/cJSON.h"
#include "wifi_def.h"

/**
 * @brief wifi调度结构体定义
 * 
 */
typedef struct
{
    char * request;
    char * response;
    void (*handler)(struct lws *wsi,size_t index,cJSON *root);
}wifi_dispatch;

void wifi_scheduler(struct lws *wsi,cJSON *root);
wifi_dispatch* wifi_dispatch_get_by_index(size_t index);
#endif