#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>
#include "lib/cJSON/cJSON.h"
#include "wifi/wifi_scheduler.h"

// ------------------------ 配置 ------------------------
#define SERVER_PORT 8080
// ----------------------------------------------------

struct per_session_data
{
    int dummy;
};

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
callback_wifi_server(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len)
{
    struct per_session_data *pss = (struct per_session_data *)user;

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        printf("客户端连接已建立.\n");
        break;

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
            const char *err_resp = "{\"type\": \"wifi_connect_response\", \"data\": {\"success\": false, \"message\": \"Invalid JSON format\", \"error\": \"JSON_PARSE_ERROR\"}}";
            unsigned char buf[LWS_PRE + strlen(err_resp)];
            memcpy(&buf[LWS_PRE], err_resp, strlen(err_resp));
            lws_write(wsi, &buf[LWS_PRE], strlen(err_resp), LWS_WRITE_TEXT);
            break;
        }

        wifi_scheduler(wsi,root);
        // 检查 "type" 字段



        // if (strcmp(type_item->valuestring, "wifi_connect_request") == 0) {
        //     printf("识别到 wifi_connect_request\n");

        //     // 获取 data 对象
        //     cJSON *data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
        //     if (!cJSON_IsObject(data_obj)) {
        //         fprintf(stderr, "缺少 'data' 对象\n");
        //         cJSON_Delete(root);

        //         const char *err_resp = "{\"type\": \"wifi_connect_response\", \"data\": {\"success\": false, \"message\": \"Missing 'data' field\", \"error\": \"MISSING_DATA\"}}";
        //         unsigned char buf[LWS_PRE + strlen(err_resp)];
        //         memcpy(&buf[LWS_PRE], err_resp, strlen(err_resp));
        //         lws_write(wsi, &buf[LWS_PRE], strlen(err_resp), LWS_WRITE_TEXT);
        //         break;
        //     }

        //     // 获取 ssid 和 password
        //     cJSON *ssid_item = cJSON_GetObjectItemCaseSensitive(data_obj, "ssid");
        //     cJSON *pass_item = cJSON_GetObjectItemCaseSensitive(data_obj, "password");

        //     if (!cJSON_IsString(ssid_item) || !ssid_item->valuestring ||
        //         !cJSON_IsString(pass_item) || !pass_item->valuestring) {
        //         fprintf(stderr, "缺少或无效的 SSID 或 Password\n");
        //         cJSON_Delete(root);

        //         const char *err_resp = "{\"type\": \"wifi_connect_response\", \"data\": {\"success\": false, \"message\": \"SSID or Password missing/invalid\", \"error\": \"INVALID_CREDENTIALS\"}}";
        //         unsigned char buf[LWS_PRE + strlen(err_resp)];
        //         memcpy(&buf[LWS_PRE], err_resp, strlen(err_resp));
        //         lws_write(wsi, &buf[LWS_PRE], strlen(err_resp), LWS_WRITE_TEXT);
        //         break;
        //     }

        //     char ssid[64];
        //     char password[64];
        //     strncpy(ssid, ssid_item->valuestring, sizeof(ssid) - 1);
        //     strncpy(password, pass_item->valuestring, sizeof(password) - 1);
        //     ssid[sizeof(ssid) - 1] = '\0';
        //     password[sizeof(password) - 1] = '\0';

        //     printf("请求连接到 SSID: '%s', Password: '%s'\n", ssid, password);

        //     // --- 模拟连接逻辑 ---
        //     int connect_success = 1;
        //     const char *message = "Connected successfully";
        //     const char *error = NULL;

        //     // 模拟失败场景
        //     if (strcmp(ssid, "FailTest") == 0) {
        //         connect_success = 0;
        //         message = "Simulated connection failure";
        //         error = "SIMULATED_ERROR";
        //     }
        //     // --------------------

        //     // 构造响应 JSON
        //     cJSON *response = cJSON_CreateObject();
        //     cJSON *data = cJSON_CreateObject();
        //     cJSON_AddStringToObject(response, "type", "wifi_connect_response");
        //     cJSON_AddItemToObject(response, "data", data);
        //     cJSON_AddBoolToObject(data, "success", connect_success);
        //     cJSON_AddStringToObject(data, "message", message);
        //     cJSON_AddStringToObject(data, "error", error);
        //     // 		    if (error) {
        //     //     cJSON_AddStringToObject(data, "error", error);
        //     // } else {
        //     //     cJSON_AddNullToObject(data, "error");
        //     // }
        //     char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
        //     if (!response_str) {
        //         fprintf(stderr, "cJSON_Print 失败\n");
        //     } else {
        //         printf("发送响应: %s\n", response_str);

        //         unsigned char buf[LWS_PRE + strlen(response_str)];
        //         memcpy(&buf[LWS_PRE], response_str, strlen(response_str));

        //         int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        //         if (n < strlen(response_str)) {
        //             fprintf(stderr, "lws_write 失败 (返回 %d)\n", n);
        //         }

        //         free(response_str); // 释放 cJSON_Print 分配的内存
        //     }

        //     // 清理
        //     cJSON_Delete(response);
        // } else {
        //     printf("未知消息类型: %s\n", type_item->valuestring);
        //     // 可以选择发送错误响应
        // }

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
        .name = "wifi-test-protocol",
        .callback = callback_wifi_server,
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
