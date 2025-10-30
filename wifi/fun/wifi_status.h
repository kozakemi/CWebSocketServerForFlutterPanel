#ifndef __WIFI_STATUS_H__
#define __WIFI_STATUS_H__
#include "../../lib/cJSON/cJSON.h"
void wifi_status(struct lws *wsi,size_t index,cJSON *root);
#endif