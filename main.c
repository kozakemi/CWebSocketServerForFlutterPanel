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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>
#include "lib/cJSON/cJSON.h"
#include "wifi/wifi_scheduler.h"
#include "brightness/brightness_scheduler.h"

// ------------------------ 配置 ------------------------
#define SERVER_PORT 8080
// ----------------------------------------------------

struct per_session_data
{
    char path[256];  // 存储WebSocket连接的路径
};

typedef struct
{
    char * patch;
    void (*scheduler)(struct lws *wsi,cJSON *root);
} websocket_path_scheduling;

static websocket_path_scheduling websocket_path_scheduling_table[] = {
    {"/wifi",wifi_scheduler},
    {"/brightness",brightness_scheduler},
};
#define WEBSOCKET_PATH_SCHEDULING_TABLE_SIZE (sizeof(websocket_path_scheduling_table) / sizeof(websocket_path_scheduling))

/**
 * @brief ws服务器回调函数
 * 
 * @param wsi 
 * @param reason 
 * @param user 
 * @param in 
 * @param len 
 * @return int 
 */
static int
callback_server(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len)
{
    struct per_session_data *pss = (struct per_session_data *)user;

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
    {
        // 获取WebSocket连接的路径
        int path_len = lws_hdr_copy(wsi, pss->path, sizeof(pss->path), WSI_TOKEN_GET_URI);
        if (path_len < 0) {
            strcpy(pss->path, "/");  // 默认路径
        }
        printf("客户端连接已建立，路径: %s\n", pss->path);
        break;
    }

    case LWS_CALLBACK_RECEIVE:
    {
        printf("收到消息 (长度 %zu): %.*s\n", len, (int)len, (char *)in);

        // 使用 cJSON 解析 JSON
        cJSON *root = cJSON_ParseWithLength((char *)in, len);
        if (!root)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            fprintf(stderr, "JSON 解析失败! %s\n", error_ptr ? error_ptr : "未知错误");

            // 发送解析错误响应
            const char *err_resp = "{\"data\": {\"success\": false, \"message\": \"Invalid JSON format\", \"error\": \"JSON_PARSE_ERROR\"}}";
            unsigned char buf[LWS_PRE + strlen(err_resp)];
            memcpy(&buf[LWS_PRE], err_resp, strlen(err_resp));
            lws_write(wsi, &buf[LWS_PRE], strlen(err_resp), LWS_WRITE_TEXT);
            break;
        }
        
        // 根据路径查找对应的调度器
        int found = 0;
        for (int i = 0; i < WEBSOCKET_PATH_SCHEDULING_TABLE_SIZE; i++) {
            if (strcmp(pss->path, websocket_path_scheduling_table[i].patch) == 0) {
                found = 1;
                if (websocket_path_scheduling_table[i].scheduler != NULL) {
                    printf("调用 %s 路径的调度器\n", pss->path);
                    websocket_path_scheduling_table[i].scheduler(wsi, root);
                } else {
                    printf("路径 %s 的调度器尚未实现\n", pss->path);
                    // 发送未实现响应
                    const char *not_impl_resp = "{\"success\": false, \"error\": -1, \"message\": \"功能尚未实现\", \"data\": {}}";
                    unsigned char buf[LWS_PRE + strlen(not_impl_resp)];
                    memcpy(&buf[LWS_PRE], not_impl_resp, strlen(not_impl_resp));
                    lws_write(wsi, &buf[LWS_PRE], strlen(not_impl_resp), LWS_WRITE_TEXT);
                }
                break;
            }
        }
        
        if (!found) {
            printf("未知路径: %s\n", pss->path);
            // 发送路径不支持响应
            const char *unsupported_resp = "{\"success\": false, \"error\": -1, \"message\": \"不支持的路径\", \"data\": {}}";
            unsigned char buf[LWS_PRE + strlen(unsupported_resp)];
            memcpy(&buf[LWS_PRE], unsupported_resp, strlen(unsupported_resp));
            lws_write(wsi, &buf[LWS_PRE], strlen(unsupported_resp), LWS_WRITE_TEXT);
        }
        
        // 释放解析的 JSON 树
        cJSON_Delete(root);
    }
    break;

    case LWS_CALLBACK_CLOSED:
        printf("客户端连接已关闭.\n");
        break;

    case LWS_CALLBACK_PROTOCOL_INIT:
        printf("协议初始化.\n");
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        .name = "ws_protocol",
        .callback = callback_server,
        .per_session_data_size = sizeof(struct per_session_data),
        .rx_buffer_size = 1024,
    },
    {NULL, NULL, 0, 0}};

int main(void)
{
    struct lws_context_creation_info info;
    struct lws_context *context;

    memset(&info, 0, sizeof info);
    info.port = SERVER_PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    context = lws_create_context(&info);
    if (!context)
    {
        fprintf(stderr, "lws_create_context 失败\n");
        return 1;
    }

    printf("WiFi 测试服务器启动，监听端口 %d...\n", SERVER_PORT);
    printf("等待客户端连接...\n");

    while (1)
    {
        int n = lws_service(context, 1000);
        if (n < 0)
        {
            fprintf(stderr, "lws_service 返回错误\n");
            break;
        }
    }

    lws_context_destroy(context);
    return 0;
}
