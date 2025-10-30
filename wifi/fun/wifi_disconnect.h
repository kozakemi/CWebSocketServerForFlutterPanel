#ifndef __WIFI_DISCONNECT_H__
#define __WIFI_DISCONNECT_H__

#include "../../lib/cJSON/cJSON.h"
void wifi_disconnect(struct lws *wsi, size_t index, cJSON *root);
#endif