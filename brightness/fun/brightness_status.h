#ifndef __BRIGHTNESS_STATUS_H__
#define __BRIGHTNESS_STATUS_H__
#include <libwebsockets.h>
#include "../../lib/cJSON/cJSON.h"
void brightness_status(struct lws *wsi, size_t index, cJSON *root);
#endif