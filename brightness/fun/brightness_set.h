#ifndef __BRIGHTNESS_SET_H__
#define __BRIGHTNESS_SET_H__

#include <libwebsockets.h>
#include "../../lib/cJSON/cJSON.h"

void brightness_set(struct lws *wsi, size_t index, cJSON *root);

#endif