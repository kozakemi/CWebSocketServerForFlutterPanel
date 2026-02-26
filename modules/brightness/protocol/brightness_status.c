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

#include "brightness_status.h"
#include "../../../protocol/protocol_utils.h"
#include "../brightness_def.h"
#include "../impl/brightness_impl.h"
#include "../brightness_scheduler.h"
#include "cJSON.h"
#include "civetweb.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief 处理亮度状态请求
 *
 * @param conn WebSocket 连接指针
 * @param index 调度索引
 * @param root JSON 根对象指针
 */
void brightness_status(struct mg_connection *conn, size_t index, cJSON *root)
{
    brightness_error_t ret = BRIGHTNESS_ERR_OK;
    int brightness = 0;

    // 调用业务层执行操作
    ret = brightness_impl_get_status(&brightness);

    // 获取request_id
    const char *request_id = protocol_get_request_id(root);
    const char *response_type = brightness_dispatch_get_by_index(index)->response;

    // 构建响应
    cJSON *response = protocol_create_response(response_type, request_id, (ret == BRIGHTNESS_ERR_OK), ret);
    if (response)
    {
        cJSON *res_data = cJSON_GetObjectItem(response, "data");
        if (res_data)
        {
            cJSON_AddNumberToObject(res_data, "brightness", brightness);
        }
        protocol_send_response(conn, response);
        cJSON_Delete(response);
    }
}