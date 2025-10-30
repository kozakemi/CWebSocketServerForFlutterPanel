#ifndef __WIFI_CONNECT_H__
#define __WIFI_CONNECT_H__
#include "../../lib/cJSON/cJSON.h"
void wifi_connect(struct lws *wsi, size_t index, cJSON *root);
#endif