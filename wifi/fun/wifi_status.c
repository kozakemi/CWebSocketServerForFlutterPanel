#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libwebsockets.h>
#include "../../lib/cJSON/cJSON.h"
#include "../wifi_def.h"
#include "../wifi_scheduler.h"
#include "wifi_status.h"

/**
 * @brief wifi状态获取错误枚举
 * 
 */


/************* 请求 ***************/

typedef struct 
{
    char * type;
    char * request_id;
}wifi_status_req;

/************* 响应 ***************/
typedef struct
{
    bool enaable;
    bool connected;
    char * ssid;
    char * bssid;
    char * interface;
    char * ip;
    int signal;
    char * security;
    int channel;
    int frequency_mhz;

}wifi_status_data;

typedef struct
{
    char * type;
    char * request_id;
    bool success;
    int error;
    wifi_status_data data;
}wifi_status_res;


wifi_status_req wifi_status_req_instance;
wifi_status_res wifi_status_res_instance;
/**
 * @brief 执行连接状态操作
 * 
 * @return int 
 */
static wifi_error_t wifi_status_execution(void)
{
    // 初始化数据结构
    wifi_status_res_instance.data.enaable = false;
    wifi_status_res_instance.data.connected = false;
    wifi_status_res_instance.data.ssid = "";
    wifi_status_res_instance.data.bssid = "";
    wifi_status_res_instance.data.interface = WIFI_DEVICE;
    wifi_status_res_instance.data.ip = "";
    wifi_status_res_instance.data.signal = 0;
    wifi_status_res_instance.data.security = "";
    wifi_status_res_instance.data.channel = 0;
    wifi_status_res_instance.data.frequency_mhz = 0;

    FILE *fp;
    char buffer[256];
    char command[256];

    // 检查WiFi是否启用
    snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // 如果能获取到状态信息，则认为WiFi已启用
            wifi_status_res_instance.data.enaable = (strstr(buffer, "COMPLETED") != NULL ||
                                                     strstr(buffer, "ASSOCIATED") != NULL ||
                                                     strstr(buffer, "ASSOCIATING") != NULL ||
                                                     strstr(buffer, "SCANNING") != NULL);
            // 如果已连接
            wifi_status_res_instance.data.connected = (strstr(buffer, "COMPLETED") != NULL);
        }
        pclose(fp);
    }

    // 获取SSID
    if (wifi_status_res_instance.data.enaable) {
        snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep '^ssid=' | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                wifi_status_res_instance.data.ssid = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取BSSID
        snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep bssid | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                wifi_status_res_instance.data.bssid = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取信号强度
        snprintf(command, sizeof(command), "wpa_cli -i %s signal_poll 2>/dev/null | grep RSSI | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                wifi_status_res_instance.data.signal = atoi(buffer);
            }
            pclose(fp);
        }

        // 获取IP地址
        snprintf(command, sizeof(command), "ip addr show %s 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d/ -f1", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                wifi_status_res_instance.data.ip = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取安全信息
        snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep key_mgmt | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                // 移除换行符
                buffer[strcspn(buffer, "\n")] = 0;
                wifi_status_res_instance.data.security = strdup(buffer);
            }
            pclose(fp);
        }

        // 获取频道
        snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep channel | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                wifi_status_res_instance.data.channel = atoi(buffer);
            }
            pclose(fp);
        }

        // 获取频率
        snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep freq | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                wifi_status_res_instance.data.frequency_mhz = atoi(buffer);
            }
            pclose(fp);
        }
    }

    return WIFI_ERR_OK;
}

/**
 * @brief 获取wifi状态
 * 
 * @param wsi 
 * @param index 
 * @param root 
 */
void wifi_status(struct lws *wsi,size_t index,cJSON *root)
{ 
    int ret = 0;
    // { "type": "wifi_status_request", "data": {} }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    wifi_status_req_instance.type = type->valuestring;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    wifi_status_req_instance.request_id = request_id->valuestring;

    ret = wifi_status_execution(); //执行状态获取操作

    // 根据执行结果构建响应数据
    wifi_status_res_instance.type = wifi_dispatch_get_by_index(index)->response; // 使用响应类型
    wifi_status_res_instance.success = (ret == WIFI_ERR_OK);                     // 设置成功标志
    wifi_status_res_instance.error = ret;                                        // 设置错误码
    wifi_status_res_instance.request_id = wifi_status_req_instance.request_id;   // 回显request_id

    /**
     * {
     *  "type": "wifi_status_response",
     *  "success": true,
     *  "error": 0,
     *  "data": {
     *      "enaable": true,
     *      "connected": true,
     *      "ssid": "test_wifi",
     *      "bssid": "xx:xx:xx:xx:xx:xx",
     *      "interface": "wlan0",
     *      "ip": "192.168.1.100",
     *      "signal": -50,
     *      "security": "WPA2-PSK",
     *      "channel": 6,
     *      "frequency_mhz": 2437
     *  }
     */
    cJSON *response = cJSON_CreateObject();
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", wifi_status_res_instance.type);
    cJSON_AddStringToObject(response, "request_id", wifi_status_res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", wifi_status_res_instance.success);
    cJSON_AddNumberToObject(response, "error", wifi_status_res_instance.error);
    
    cJSON_AddBoolToObject(res_data, "enaable", wifi_status_res_instance.data.enaable);
    cJSON_AddBoolToObject(res_data, "connected", wifi_status_res_instance.data.connected);
    
    if (wifi_status_res_instance.data.ssid && strlen(wifi_status_res_instance.data.ssid) > 0) {
        cJSON_AddStringToObject(res_data, "ssid", wifi_status_res_instance.data.ssid);
    } else {
        cJSON_AddStringToObject(res_data, "ssid", "");
    }
    
    if (wifi_status_res_instance.data.bssid && strlen(wifi_status_res_instance.data.bssid) > 0) {
        cJSON_AddStringToObject(res_data, "bssid", wifi_status_res_instance.data.bssid);
    } else {
        cJSON_AddStringToObject(res_data, "bssid", "");
    }
    
    cJSON_AddStringToObject(res_data, "interface", wifi_status_res_instance.data.interface);
    
    if (wifi_status_res_instance.data.ip && strlen(wifi_status_res_instance.data.ip) > 0) {
        cJSON_AddStringToObject(res_data, "ip", wifi_status_res_instance.data.ip);
    } else {
        cJSON_AddStringToObject(res_data, "ip", "");
    }
    
    cJSON_AddNumberToObject(res_data, "signal", wifi_status_res_instance.data.signal);
    
    if (wifi_status_res_instance.data.security && strlen(wifi_status_res_instance.data.security) > 0) {
        cJSON_AddStringToObject(res_data, "security", wifi_status_res_instance.data.security);
    } else {
        cJSON_AddStringToObject(res_data, "security", "");
    }
    
    cJSON_AddNumberToObject(res_data, "channel", wifi_status_res_instance.data.channel);
    cJSON_AddNumberToObject(res_data, "frequency_mhz", wifi_status_res_instance.data.frequency_mhz);
    
    cJSON_AddItemToObject(response, "data", res_data);

    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_status: Failed to print response\n");
        cJSON_Delete(response);
        return;
    }
    else
    {
        printf("wifi_status: %s\n", response_str);
        unsigned char buf[LWS_PRE + strlen(response_str)];
        memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
        int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        if (n < 0)
        {
            printf("wifi_status: Failed to write response\n");
        }
        cJSON_free(response_str);
    }
    
    // 释放动态分配的内存
    if (wifi_status_res_instance.data.ssid && strlen(wifi_status_res_instance.data.ssid) > 0) {
        free(wifi_status_res_instance.data.ssid);
    }
    if (wifi_status_res_instance.data.bssid && strlen(wifi_status_res_instance.data.bssid) > 0) {
        free(wifi_status_res_instance.data.bssid);
    }
    if (wifi_status_res_instance.data.ip && strlen(wifi_status_res_instance.data.ip) > 0) {
        free(wifi_status_res_instance.data.ip);
    }
    if (wifi_status_res_instance.data.security && strlen(wifi_status_res_instance.data.security) > 0) {
        free(wifi_status_res_instance.data.security);
    }
    cJSON_Delete(response);
}