#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libwebsockets.h>
#include "../../lib/cJSON/cJSON.h"
#include "../wifi_def.h"
#include "../wifi_scheduler.h"
#include "wifi_connect.h"


/************* 请求 ***************/
typedef struct 
{
    char * ssid;
    char * password;
    int timeout_ms;

} wifi_connect_data;

typedef struct
{
    char * type;
    char * request_id;
    wifi_connect_data data;
} wifi_connect_request;


/************* 响应 ***************/

typedef struct
{
    char * type;
    char * request_id;
    bool success;
    int error;
} wifi_connect_response;

wifi_connect_request wifi_connect_req_instance;
wifi_connect_response wifi_connect_res_instance;

/**
 * @brief 执行wifi连接操作
 * 
 * @return wifi_connect_error_code 
 */
static wifi_error_t wifi_connect_execution(void)
{
    FILE *fp;
    char buffer[256];
    char command[256];
    int network_id = -1;
    int timeout;
    int wait_time;
    bool connected = false;
    
    // 当密码为空时，尝试使用已保存的网络配置
    if (!wifi_connect_req_instance.data.password || 
        strlen(wifi_connect_req_instance.data.password) == 0) {
        
        // 查找已保存的网络ID
        snprintf(command, sizeof(command), 
                 "wpa_cli -i %s list_networks 2>/dev/null | grep -E '\t%s\t' | cut -f1", 
                 WIFI_DEVICE, wifi_connect_req_instance.data.ssid);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                network_id = atoi(buffer);
            }
            pclose(fp);
        }
        
        // 如果找到已保存的网络配置，直接选择该网络
        if (network_id >= 0) {
            // 启用网络
            snprintf(command, sizeof(command), 
                     "wpa_cli -i %s enable_network %d 2>/dev/null", 
                     WIFI_DEVICE, network_id);
            system(command);
            
            // 选择网络
            snprintf(command, sizeof(command), 
                     "wpa_cli -i %s select_network %d 2>/dev/null", 
                     WIFI_DEVICE, network_id);
            system(command);
            
            // 跳转到等待连接结果的部分
            goto wait_for_connection;
        }
    }
    
    // 添加一个新的网络配置
    snprintf(command, sizeof(command), "wpa_cli -i %s add_network 2>/dev/null", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            network_id = atoi(buffer);
        }
        pclose(fp);
    }
    
    if (network_id < 0) {
        return WIFI_ERR_TOOL_ERROR;
    }
    
    // 设置SSID
    snprintf(command, sizeof(command), 
             "wpa_cli -i %s set_network %d ssid '\"%s\"' 2>/dev/null", 
             WIFI_DEVICE, network_id, wifi_connect_req_instance.data.ssid);
    system(command);
    
    // 设置密码（如果提供了密码）
    if (wifi_connect_req_instance.data.password && 
        strlen(wifi_connect_req_instance.data.password) > 0) {
        snprintf(command, sizeof(command), 
                 "wpa_cli -i %s set_network %d psk '\"%s\"' 2>/dev/null", 
                 WIFI_DEVICE, network_id, wifi_connect_req_instance.data.password);
    } else {
        // 开放网络（无密码）
        snprintf(command, sizeof(command), 
                 "wpa_cli -i %s set_network %d key_mgmt NONE 2>/dev/null", 
                 WIFI_DEVICE, network_id);
    }
    system(command);
    
    // 启用网络
    snprintf(command, sizeof(command), 
             "wpa_cli -i %s enable_network %d 2>/dev/null", 
             WIFI_DEVICE, network_id);
    system(command);
    
    // 选择网络
    snprintf(command, sizeof(command), 
             "wpa_cli -i %s select_network %d 2>/dev/null", 
             WIFI_DEVICE, network_id);
    system(command);
    
wait_for_connection:
    // 等待连接结果（最多等待timeout_ms毫秒）
    timeout = wifi_connect_req_instance.data.timeout_ms > 0 ?  wifi_connect_req_instance.data.timeout_ms : 20000; // 默认20秒
    wait_time = 0;
    connected = false;
    
    while (wait_time < timeout) {
        snprintf(command, sizeof(command), 
                 "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", 
                 WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                // 检查是否连接成功
                if (strstr(buffer, "COMPLETED") != NULL) {
                    connected = true;
                    pclose(fp);
                    break;
                }
            }
            pclose(fp);
        }
        
        // 等待1秒再检查
        sleep(1);
        wait_time += 1000;
    }
    
    if (!connected) {
        // 连接失败，禁用网络
        if (network_id >= 0) {
            snprintf(command, sizeof(command), 
                     "wpa_cli -i %s disable_network %d 2>/dev/null", 
                     WIFI_DEVICE, network_id);
            system(command);
        }
        return WIFI_ERR_TIMEOUT;
    }
    
    // 连接成功，保存配置
    snprintf(command, sizeof(command), 
             "wpa_cli -i %s save_config 2>/dev/null", 
             WIFI_DEVICE);
    system(command);
    
    return WIFI_ERR_OK;
}

void wifi_connect(struct lws *wsi, size_t index, cJSON *root)
{
    int ret = 0;
    /**
     *  解析请求
     * 
     * {
     *   "type": "wifi_connect_request",
     *   "data": {
     *     "ssid": "MyHomeNetwork",
     *     "password": "mysecretpassword",
     *     "timeout_ms": 20000
     *   }
     * }
     * 
     */
    cJSON *type = cJSON_GetObjectItem(root, "type");
    wifi_connect_req_instance.type = type->valuestring;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    wifi_connect_req_instance.request_id = request_id->valuestring;
    cJSON *data = cJSON_GetObjectItem(root, "data");
    wifi_connect_req_instance.data.ssid = cJSON_GetObjectItem(data, "ssid")->valuestring;
    wifi_connect_req_instance.data.password = cJSON_GetObjectItem(data, "password")->valuestring;
    wifi_connect_req_instance.data.timeout_ms = cJSON_GetObjectItem(data, "timeout_ms")->valueint;
    
    // 处理请求
    ret = wifi_connect_execution();

    /**
     * {
     *   "type": "wifi_connect_response",
     *   "success": true,
     *   "error": 0,
     *   "data": { 
     *   }
     * }
     * 
     * {
     *   "type": "wifi_connect_response",
     *   "success": false,
     *   "error": 1,
     *   "data": {  }
     * }
     */

    cJSON *response = cJSON_CreateObject();
    
    wifi_connect_res_instance.type = wifi_dispatch_get_by_index(index)->response;
    wifi_connect_res_instance.request_id = wifi_connect_req_instance.request_id;
    wifi_connect_res_instance.error = ret;
    wifi_connect_res_instance.success = (ret == WIFI_ERR_OK);
    cJSON_AddStringToObject(response, "type", wifi_connect_res_instance.type);
    cJSON_AddStringToObject(response, "request_id", wifi_connect_res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", wifi_connect_res_instance.success);
    cJSON_AddNumberToObject(response, "error", wifi_connect_res_instance.error);
    // 添加空的data对象，保持统一结构
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "data", res_data);
    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_connect: Failed to print response\n");
        return;
    }
    else
    {
        printf("wifi_connect: %s\n", response_str);
        unsigned char buf[LWS_PRE + strlen(response_str)];
        memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
        int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        if (n < 0)
        {
            printf("wifi_connect: Failed to write response\n");
        }
        cJSON_free(response_str);
    }
    cJSON_Delete(response);
}
