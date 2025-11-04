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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libwebsockets.h>
#include "../../lib/cJSON/cJSON.h"
#include "../wifi_def.h"
#include "../wifi_scheduler.h"
#include "wifi_disconnect.h"



/************* 请求 ***************/
typedef struct 
{
    char * ssid;
} wifi_disconnect_data;

typedef struct
{
    char * type;
    char * request_id;
    wifi_disconnect_data data;
} wifi_disconnect_request;

/************* 响应 ***************/
typedef struct
{
    char * type;
    char * request_id;
    bool success;
    int error;
} wifi_disconnect_response;

wifi_disconnect_request wifi_disconnect_req_instance;
wifi_disconnect_response wifi_disconnect_res_instance;

/**
 * @brief 执行wifi断开连接操作
 * 
 * @return wifi_disconnect_error_code 
 */
static wifi_error_t wifi_disconnect_execution(void)
{
    FILE *fp;
    char buffer[256];
    char command[256];
    int network_id = -1;
    bool is_connected = false;
    
    // 检查当前是否连接到任何网络
    snprintf(command, sizeof(command), 
             "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", 
             WIFI_DEVICE);
    printf("wifi_disconnect: %s\n", command);
    fp = popen(command, "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // 检查是否已连接
            if (strstr(buffer, "COMPLETED") != NULL) {
                is_connected = true;
            }
        }
        pclose(fp);
    }
    
    if (!is_connected) {
        return WIFI_ERR_NOT_CONNECTED; // 未连接任何网络
    }
    
    // 如果请求中指定了SSID，则检查当前连接的是否是该网络
    if (wifi_disconnect_req_instance.data.ssid && 
        strlen(wifi_disconnect_req_instance.data.ssid) > 0) {
        
        // 获取当前连接的SSID
        char current_ssid[128] = {0};
        snprintf(command, sizeof(command), 
                 "wpa_cli -i %s status 2>/dev/null | grep '^ssid=' | head -1 | cut -d= -f2", 
                 WIFI_DEVICE);
        printf("wifi_disconnect: %s\n", command);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                strcpy(current_ssid, buffer);
            }
            pclose(fp);
        }
        
        // 如果当前连接的SSID与请求的SSID不匹配，返回错误
        if (strcmp(current_ssid, wifi_disconnect_req_instance.data.ssid) != 0) {
            return WIFI_ERR_BAD_REQUEST;
        }
    }
    
    // 执行断开连接操作
    snprintf(command, sizeof(command), 
             "wpa_cli -i %s disconnect 2>/dev/null", 
             WIFI_DEVICE);
    printf("wifi_disconnect: %s\n", command);
    int result = system(command);
    
    if (result != 0) {
        return WIFI_ERR_TOOL_ERROR;
    }
    
    return WIFI_ERR_OK;
}

void wifi_disconnect(struct lws *wsi, size_t index, cJSON *root)
{
    int ret = 0;
    
    /**
     * 解析请求
     * 
     * { "type": "wifi_disconnect_request", "data": { "ssid": "MyHomeNetwork" } }
     */
    cJSON *type = cJSON_GetObjectItem(root, "type");
    wifi_disconnect_req_instance.type = type->valuestring;
    
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    wifi_disconnect_req_instance.request_id = request_id->valuestring;
    
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *ssid_item = cJSON_GetObjectItem(data, "ssid");
    if (ssid_item) {
        wifi_disconnect_req_instance.data.ssid = ssid_item->valuestring;
    } else {
        wifi_disconnect_req_instance.data.ssid = NULL;
    }
    
    // 处理请求
    ret = wifi_disconnect_execution();

    /**
     * 响应：
     * {
     *   "type": "wifi_disconnect_response",
     *   "success": true,
     *   "error": 0,
     *   "data": { }
     * }
     */

    cJSON *response = cJSON_CreateObject();
    
    wifi_disconnect_res_instance.type = wifi_dispatch_get_by_index(index)->response;
    wifi_disconnect_res_instance.request_id = wifi_disconnect_req_instance.request_id;
    wifi_disconnect_res_instance.error = ret;
    wifi_disconnect_res_instance.success = (ret == WIFI_ERR_OK);
    
    cJSON_AddStringToObject(response, "type", wifi_disconnect_res_instance.type);
    cJSON_AddStringToObject(response, "request_id", wifi_disconnect_res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", wifi_disconnect_res_instance.success);
    cJSON_AddNumberToObject(response, "error", wifi_disconnect_res_instance.error);
    
    // 添加空的data对象
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "data", res_data);
    
    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_disconnect: Failed to print response\n");
        cJSON_Delete(response);
        return;
    }
    else
    {
        printf("wifi_disconnect: %s\n", response_str);
        unsigned char buf[LWS_PRE + strlen(response_str)];
        memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
        int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        if (n < 0)
        {
            printf("wifi_disconnect: Failed to write response\n");
        }
        cJSON_free(response_str);
    }
    cJSON_Delete(response);
}