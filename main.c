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

#include "brightness/brightness_scheduler.h"
#include "cJSON.h"
#include "ws_utils.h"
#include "wifi/wifi_scheduler.h"
#include "civetweb.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// ------------------------ 配置 ------------------------
#define SERVER_PORT 8080
// ----------------------------------------------------

// 用户连接数据
struct per_session_data
{
    char path[256]; // 存储WebSocket连接的路径
};

typedef struct
{
    char *patch;
    void (*scheduler)(struct mg_connection *conn, cJSON *root);
} websocket_path_scheduling;

static websocket_path_scheduling websocket_path_scheduling_table[] = {
    {"/wifi", wifi_scheduler},
    {"/brightness", brightness_scheduler},
};
#define WEBSOCKET_PATH_SCHEDULING_TABLE_SIZE                                                       \
    (sizeof(websocket_path_scheduling_table) / sizeof(websocket_path_scheduling))

// 全局服务器上下文
static struct mg_context *g_ctx = NULL;
volatile int g_exit = 0;

/* WebSocket 连接处理器：客户端尝试建立连接时调用 */
static int ws_connect_handler(const struct mg_connection *conn, void *user_data)
{
    (void)user_data; /* unused */
    
    // 分配连接数据
    struct per_session_data *pss = (struct per_session_data *)calloc(1, sizeof(struct per_session_data));
    if (!pss)
    {
        return 1; // 拒绝连接
    }
    
    // 获取请求路径
    const struct mg_request_info *ri = mg_get_request_info(conn);
    if (ri && ri->local_uri)
    {
        strncpy(pss->path, ri->local_uri, sizeof(pss->path) - 1);
        pss->path[sizeof(pss->path) - 1] = '\0';
    }
    else
            {
                strcpy(pss->path, "/"); // 默认路径
            }
    
    // 设置用户数据（注意：需要将 const 转换为非 const）
    mg_set_user_connection_data((struct mg_connection *)conn, pss);
    
            printf("客户端连接已建立，路径: %s\n", pss->path);
    return 0; // 接受连接
}

/* WebSocket 就绪处理器：握手完成后调用 */
static void ws_ready_handler(struct mg_connection *conn, void *user_data)
{
    (void)conn; /* unused */
    (void)user_data; /* unused */
    printf("WebSocket 连接就绪\n");
        }

/* WebSocket 数据处理器：收到数据时调用 */
static int ws_data_handler(struct mg_connection *conn,
                            int opcode,
                            char *data,
                            size_t datasize,
                            void *user_data)
{
    (void)user_data; /* unused */
    
    // 只处理文本消息
    if ((opcode & 0xf) != MG_WEBSOCKET_OPCODE_TEXT)
    {
        return 1; // 保持连接
    }
    
    printf("收到消息 (长度 %zu): %.*s\n", datasize, (int)datasize, data);
    
    // 获取连接数据
    struct per_session_data *pss = (struct per_session_data *)mg_get_user_connection_data(conn);
    if (!pss)
    {
        return 0; // 关闭连接
    }

            // 使用 cJSON 解析 JSON
    cJSON *root = cJSON_ParseWithLength(data, datasize);
            if (!root)
            {
                const char *error_ptr = cJSON_GetErrorPtr();
                fprintf(stderr, "JSON 解析失败! %s\n", error_ptr ? error_ptr : "未知错误");

                // 发送解析错误响应
                const char *err_resp = "{\"data\": {\"success\": false, \"message\": \"Invalid "
                                       "JSON format\", \"error\": \"JSON_PARSE_ERROR\"}}";
        ws_send_text(conn, err_resp);
        return 1; // 保持连接
            }

            // 根据路径查找对应的调度器
            int found = 0;
            for (size_t i = 0; i < WEBSOCKET_PATH_SCHEDULING_TABLE_SIZE; i++)
            {
                if (strcmp(pss->path, websocket_path_scheduling_table[i].patch) == 0)
                {
                    found = 1;
                    if (websocket_path_scheduling_table[i].scheduler != NULL)
                    {
                        printf("调用 %s 路径的调度器\n", pss->path);
                websocket_path_scheduling_table[i].scheduler(conn, root);
                    }
                    else
                    {
                        printf("路径 %s 的调度器尚未实现\n", pss->path);
                        // 发送未实现响应
                        const char *not_impl_resp = "{\"success\": false, \"error\": -1, "
                                                    "\"message\": \"功能尚未实现\", \"data\": {}}";
                ws_send_text(conn, not_impl_resp);
                    }
                    break;
                }
            }

            if (!found)
            {
                printf("未知路径: %s\n", pss->path);
                // 发送路径不支持响应
                const char *unsupported_resp = "{\"success\": false, \"error\": -1, \"message\": "
                                               "\"不支持的路径\", \"data\": {}}";
        ws_send_text(conn, unsupported_resp);
            }

            // 释放解析的 JSON 树
            cJSON_Delete(root);
    return 1; // 保持连接
}

/* WebSocket 关闭处理器：连接关闭时调用 */
static void ws_close_handler(const struct mg_connection *conn, void *user_data)
{
    (void)user_data; /* unused */
    
    // 释放连接数据
    struct per_session_data *pss = (struct per_session_data *)mg_get_user_connection_data(conn);
    if (pss)
    {
        free(pss);
    }
    
    printf("客户端连接已关闭.\n");
}

/* 信号处理器 */
static void signal_handler(int sig)
{
    (void)sig;
    g_exit = 1;
}

int main(void)
{
    // 初始化 civetweb 库
    unsigned features = mg_init_library(MG_FEATURES_WEBSOCKET);
    if (features == 0)
    {
        fprintf(stderr, "mg_init_library 失败\n");
        return 1;
    }
    
    // 设置信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 配置服务器选项
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", SERVER_PORT);
    const char *server_options[] = {
        "listening_ports", port_str,
        "num_threads", "10",
        NULL, NULL
    };
    
    // 启动服务器
    struct mg_callbacks callbacks = {0};
    void *user_data = NULL;
    
    struct mg_init_data mg_start_init_data = {0};
    mg_start_init_data.callbacks = &callbacks;
    mg_start_init_data.user_data = user_data;
    mg_start_init_data.configuration_options = server_options;
    
    struct mg_error_data mg_start_error_data = {0};
    char errtxtbuf[256] = {0};
    mg_start_error_data.text = errtxtbuf;
    mg_start_error_data.text_buffer_size = sizeof(errtxtbuf);
    
    g_ctx = mg_start2(&mg_start_init_data, &mg_start_error_data);
    if (!g_ctx)
    {
        fprintf(stderr, "无法启动服务器: %s\n", errtxtbuf);
        mg_exit_library();
        return 1;
    }

    // 注册 WebSocket 处理器（为每个路径注册）
    for (size_t i = 0; i < WEBSOCKET_PATH_SCHEDULING_TABLE_SIZE; i++)
    {
        mg_set_websocket_handler(g_ctx,
                                  websocket_path_scheduling_table[i].patch,
                                  ws_connect_handler,
                                  ws_ready_handler,
                                  ws_data_handler,
                                  ws_close_handler,
                                  user_data);
    }
    
    printf("WebSocket 服务器启动，监听端口 %d...\n", SERVER_PORT);
    printf("等待客户端连接...\n");
    printf("支持的路径:\n");
    for (size_t i = 0; i < WEBSOCKET_PATH_SCHEDULING_TABLE_SIZE; i++)
    {
        printf("  - %s\n", websocket_path_scheduling_table[i].patch);
    }
    
    // 运行服务器
    while (!g_exit)
    {
        sleep(1);
    }
    
    printf("WebSocket 服务器正在停止...\n");
    
    // 停止服务器并清理
    mg_stop(g_ctx);
    mg_exit_library();
    
    return 0;
}
