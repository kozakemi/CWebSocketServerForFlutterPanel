#ifndef __BRIGHTNESS_SCHEDULER_H__
#define __BRIGHTNESS_SCHEDULER_H__
#include <stdint.h>
#include <libwebsockets.h>
#include "../lib/cJSON/cJSON.h"
#include "brightness_def.h"

typedef struct
{
    char * request;
    char * response;
    void (*handler)(struct lws *wsi,size_t index,cJSON *root);
}brightness_dispatch;
brightness_dispatch *brightness_dispatch_get_by_inedx(size_t index);
void brightness_scheduler(struct lws *wsi,size_t index,cJSON *root);
#endif