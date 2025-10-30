#ifndef __WIFI_SCAN_H__
#define __WIFI_SCAN_H__
#include "../../lib/cJSON/cJSON.h"
#include <libwebsockets.h>
#include <stddef.h>

void wifi_scan(struct lws *wsi, size_t index, cJSON *root);

#endif