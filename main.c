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

/**
 * 处理新的 WebSocket 连接：为连接分配并初始化每连接数据，提取请求路径并将其附加到连接上。
 *
 * 如果请求中包含本地 URI，则将其复制到每连接数据的 path 字段；否则将 path 设为 "/"。
 *
 * @param conn 指向正在建立的 CivetWeb 连接对象的指针。
 * @param user_data 传入的用户数据（本函数未使用，可为 NULL）。
 * @returns `0` 表示接受连接并将每连接数据附加到连接上；非零值表示拒绝连接（分配失败时返回）。 
 */
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

/**
         * 在 WebSocket 握手完成后记录连接已就绪。
         *
         * @param conn 触发就绪事件的 CivetWeb 连接对象。
         * @param user_data 在注册回调时传入的用户数据（可为 NULL）。
         */
static void ws_ready_handler(struct mg_connection *conn, void *user_data)
{
    (void)conn; /* unused */
    (void)user_data; /* unused */
    printf("WebSocket 连接就绪\n");
        }

/**
 * 处理收到的 WebSocket 数据并将文本格式的 JSON 消息按连接路径分发给对应的调度器。
 *
 * 仅处理文本帧：将接收的数据解析为 JSON，解析失败时发送解析错误响应；解析成功后根据连接的路径查找并调用对应的调度器，
 * 若路径未注册或调度器未实现则发送相应错误响应。函数不接管连接数据的生命周期；调用后连接可继续保持或关闭（见返回值）。
 *
 * @param conn 当前的 CivetWeb 连接对象，用于发送响应并传递给路径调度器。
 * @param opcode WebSocket 帧的操作码，用以判断是否为文本帧。
 * @param data 指向接收到的数据缓冲区（未以 NUL 终止，长度由 datasize 指定）。
 * @param datasize data 缓冲区的字节长度。
 * @param user_data 用户数据（未使用）。
 * @returns `1` 表示保持连接，`0` 表示关闭连接。
 */
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

/**
 * 释放并清理与已关闭 WebSocket 连接关联的会话数据。
 *
 * 如果连接上挂载了 per_session_data，则释放其内存并记录连接关闭信息。
 *
 * @param conn 指向已关闭连接的 CivetWeb 连接对象，用于检索并释放其关联的会话数据。
 */
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

/**
 * 设置退出标志以触发服务器优雅关闭。
 *
 * 接收到终止类信号时将全局变量 `g_exit` 设为 1，通知主循环退出并开始停止服务器。
 *
 * @param sig 触发处理器的信号编号（未使用）。
 */
static void signal_handler(int sig)
{
    (void)sig;
    g_exit = 1;
}

/**
 * 启动并运行基于 CivetWeb 的 WebSocket 服务器，注册路径对应的处理器，等待终止信号后优雅关闭服务器。
 *
 * 初始化 CivetWeb 库，配置并启动服务器，按 websocket_path_scheduling_table 中的路径为每个路径注册
 * 连接/就绪/数据/关闭回调，进入由 signal_handler 控制的主循环以保持服务器运行，接收到终止信号后停止服务器并清理资源。
 *
 * @returns 0 表示服务器正常启动并在接收到终止信号后成功退出；非零表示启动或初始化失败。
 */
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