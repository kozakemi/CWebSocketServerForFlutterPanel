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

#ifndef WIFI_SCHEDULER_H
#define WIFI_SCHEDULER_H
#include "cJSON.h"
#include "civetweb.h"
#include "wifi_def.h"
#include <stdint.h>

/**
 * @brief wifi调度结构体定义
 *
 */
typedef struct
{
    char *request;
    char *response;
    void (*handler)(struct mg_connection *conn, size_t index, cJSON *root);
} wifi_dispatch;

void wifi_scheduler(struct mg_connection *conn, cJSON *root);
wifi_dispatch *wifi_dispatch_get_by_index(size_t index);
#endif