/*
Copyright 2025 kozakemi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
void brightness_scheduler(struct lws *wsi,cJSON *root);
#endif