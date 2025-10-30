#ifndef __WIFI_ENABLE_H__
#define __WIFI_ENABLE_H__ 
#include "../../lib/cJSON/cJSON.h"
void wifi_enable(struct lws *wsi,size_t index,cJSON *root);
#endif