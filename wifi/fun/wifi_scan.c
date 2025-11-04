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
#include "wifi_scan.h"

/**
 * @brief wifi扫描错误枚举
 *
 */

/**
 * @brief 解码UTF-8转义序列
 * 
 * 将形如 \xe4\xbc\x9a\xe8\xae\xae001 的转义序列转换为正确的UTF-8字符串
 * 
 * @param input 输入的包含转义序列的字符串
 * @param output 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @return int 成功返回0，失败返回-1
 */
static int decode_utf8_escape_sequence(const char *input, char *output, size_t output_size)
{
    if (!input || !output || output_size == 0) {
        return -1;
    }
    
    const char *src = input;
    char *dst = output;
    size_t dst_len = 0;
    
    while (*src && dst_len < output_size - 1) {
        if (*src == '\\' && *(src + 1) == 'x' && 
            src + 3 < input + strlen(input)) {
            // 检查是否是有效的十六进制转义序列
            char hex_str[3] = {src[2], src[3], '\0'};
            char *endptr;
            unsigned long hex_val = strtoul(hex_str, &endptr, 16);
            
            if (*endptr == '\0' && hex_val <= 0xFF) {
                // 有效的十六进制值，转换为字节
                *dst++ = (char)hex_val;
                dst_len++;
                src += 4; // 跳过 \xXX
            } else {
                // 无效的转义序列，直接复制
                *dst++ = *src++;
                dst_len++;
            }
        } else {
            // 普通字符，直接复制
            *dst++ = *src++;
            dst_len++;
        }
    }
    
    *dst = '\0';
    return 0;
}


/************* 请求 ***************/

typedef struct
{
    char *type;
    char *request_id;
    bool rescan;
} wifi_scan_req;

/************* 响应 ***************/
typedef struct
{
    char *ssid;
    char *bssid;
    int signal;
    char *security;
    int channel;
    int frequency_mhz;
    bool recorded;
} wifi_network_info;

typedef struct
{
    char *type;
    char *request_id;
    bool success;
    int error;
    wifi_network_info *networks;
    size_t network_count;
} wifi_scan_res;

wifi_scan_req wifi_scan_req_instance;
wifi_scan_res wifi_scan_res_instance;

/**
 * @brief 执行WiFi扫描操作
 *
 * @param rescan 是否强制重新扫描
 * @return int 错误码
 */
static wifi_error_t wifi_scan_execution(bool rescan)
{
    FILE *fp;
    char buffer[512];
    char command[256];

    // 释放之前分配的内存（如果有）
    if (wifi_scan_res_instance.networks)
    {
        for (size_t i = 0; i < wifi_scan_res_instance.network_count; i++)
        {
            free(wifi_scan_res_instance.networks[i].ssid);
            free(wifi_scan_res_instance.networks[i].bssid);
            free(wifi_scan_res_instance.networks[i].security);
        }
        free(wifi_scan_res_instance.networks);
        wifi_scan_res_instance.networks = NULL;
        wifi_scan_res_instance.network_count = 0;
    }

    // 执行扫描命令
    // 使用wpa_cli扫描WiFi网络
    if (rescan)
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s scan 2>/dev/null", WIFI_DEVICE);
        system(command); // 触发扫描
        // 等待扫描完成
        snprintf(command, sizeof(command), "wpa_cli -i %s scan_results 2>/dev/null", WIFI_DEVICE);
    }
    else
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s scan_results 2>/dev/null", WIFI_DEVICE);
    }

    fp = popen(command, "r");
    if (fp == NULL)
    {
        return WIFI_ERR_TOOL_ERROR;
    }

    // 第一行是标题，跳过
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        pclose(fp);
        return WIFI_ERR_TOOL_ERROR;
    }

    // 预分配网络列表空间
    wifi_network_info *temp_networks = malloc(sizeof(wifi_network_info) * 64); // 初始分配64个
    if (!temp_networks)
    {
        pclose(fp);
        return WIFI_ERR_INTERNAL;
    }

    size_t count = 0;
    size_t max_networks = 64;

    // 解析扫描结果
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        /**
         * neons@neonsboard:~/websocket_test$ sudo wpa_cli -i wlan0 scan_results
         * bssid / frequency / signal level / flags / ssid
         * d4:35:38:ff:b5:ce       2462    -34     [WPA-PSK-CCMP+TKIP][WPA2-PSK-CCMP+TKIP][WPS][ESS]       Xiaomi_B5CD
         * 16:9b:08:81:04:07       2437    -42     [WPA2-PSK-CCMP][ESS]    iPhone
         *
         */
        // 如果需要更多空间
        if (count >= max_networks)
        {
            max_networks *= 2;
            wifi_network_info *new_networks = realloc(temp_networks, sizeof(wifi_network_info) * max_networks);
            if (!new_networks)
            {
                // 释放已分配的内存
                for (size_t i = 0; i < count; i++)
                {
                    free(temp_networks[i].ssid);
                    free(temp_networks[i].bssid);
                    free(temp_networks[i].security);
                }
                free(temp_networks);
                pclose(fp);
                return WIFI_ERR_INTERNAL;
            }
            temp_networks = new_networks;
        }

        // 解析每行数据
        int frequency, signal;
        char bssid[32], security[128], ssid[128];

        // 提取频率、BSSID、信号强度和SSID/安全信息
        // 修改解析方式，正确处理各字段
        char *token = strtok(buffer, "\t");
        if (token)
        {
            strncpy(bssid, token, sizeof(bssid) - 1);
            bssid[sizeof(bssid) - 1] = '\0';
            token = strtok(NULL, "\t");
            if (token)
            {
                frequency = atoi(token);
                token = strtok(NULL, "\t");
                if (token)
                {
                    signal = atoi(token);
                    token = strtok(NULL, "\t");
                    if (token)
                    {
                        // 安全信息
                        char *security_start = token;
                        // if (security_start[0] == '[')
                        //     security_start++;
                        char *security_end = security_start + strlen(security_start) - 1;
                        // if (security_end > security_start && *security_end == ']')
                        //     *security_end = '\0';
                        strcpy(security, security_start);

                        // SSID是最后一个字段
                        token = strtok(NULL, "\t");
                        if (token)
                        {
                            // 移除末尾的换行符
                            token[strcspn(token, "\n")] = 0;
                            
                            // 处理SSID中的UTF-8转义字符
                            char decoded_ssid[128];
                            if (decode_utf8_escape_sequence(token, decoded_ssid, sizeof(decoded_ssid)) == 0) {
                                strcpy(ssid, decoded_ssid);
                            } else {
                                // 解码失败，使用原始字符串
                                strcpy(ssid, token);
                            }
                        }
                        else
                        {
                            ssid[0] = '\0';
                        }

                        // 存储网络信息
                        temp_networks[count].frequency_mhz = frequency;
                        temp_networks[count].bssid = strdup(bssid);
                        temp_networks[count].signal = signal;
                        temp_networks[count].ssid = strdup(ssid[0] ? ssid : "\\x00"); // 处理隐藏SSID

                        // 如果是空的，设为"Open"
                        if (strlen(security) == 0)
                        {
                            temp_networks[count].security = strdup("Open");
                        }
                        else
                        {
                            temp_networks[count].security = strdup(security);
                        }

                        // 计算频道号 (2.4GHz: channel = (freq - 2407) / 5)
                        if (frequency >= 2412 && frequency <= 2484)
                        {
                            temp_networks[count].channel = (frequency - 2407) / 5;
                        }
                        else if (frequency >= 5035 && frequency <= 5895)
                        {
                            // 5GHz: channel = (freq - 5000) / 5
                            temp_networks[count].channel = (frequency - 5000) / 5;
                        }
                        else
                        {
                            temp_networks[count].channel = 0;
                        }

                        // 默认设置为未记录
                        temp_networks[count].recorded = false;

                        count++;
                    }
                }
            }
        }
    }

    pclose(fp);

    // 检查哪些网络已被记录
    snprintf(command, sizeof(command), "wpa_cli -i %s list_networks 2>/dev/null", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL) {
        // 跳过标题行
        fgets(buffer, sizeof(buffer), fp);
        
        // 解析已记录的网络
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // 解析SSID字段（第2列）
            char *token = strtok(buffer, "\t");
            if (token) {
                token = strtok(NULL, "\t"); // 跳过network id
                if (token) {
                    // 移除SSID末尾的换行符
                    token[strcspn(token, "\n")] = 0;
                    
                    // 解码已记录网络的SSID
                    char decoded_recorded_ssid[128];
                    if (decode_utf8_escape_sequence(token, decoded_recorded_ssid, sizeof(decoded_recorded_ssid)) == 0) {
                        // 使用解码后的SSID进行比较
                        for (size_t i = 0; i < count; i++) {
                            if (strcmp(temp_networks[i].ssid, decoded_recorded_ssid) == 0) {
                                temp_networks[i].recorded = true;
                                break;
                            }
                        }
                    } else {
                        // 解码失败，使用原始字符串比较
                        for (size_t i = 0; i < count; i++) {
                             if (strcmp(temp_networks[i].ssid, token) == 0) {
                                 temp_networks[i].recorded = true;
                                 break;
                             }
                         }
                     }
                 }
             }
         }
        pclose(fp);
    }

    // 调整到实际大小
    if (count > 0)
    {
        wifi_network_info *final_networks = realloc(temp_networks, sizeof(wifi_network_info) * count);
        if (final_networks)
        {
            temp_networks = final_networks;
        }
    }
    else
    {
        free(temp_networks);
        temp_networks = NULL;
    }

    wifi_scan_res_instance.networks = temp_networks;
    wifi_scan_res_instance.network_count = count;

    return WIFI_ERR_OK;
}

/**
 * @brief 扫描WiFi网络
 *
 * @param wsi
 * @param index
 * @param root
 */
void wifi_scan(struct lws *wsi, size_t index, cJSON *root)
{
    int ret = 0;

    // { "type": "wifi_scan_request", "data": { "rescan": true } }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    wifi_scan_req_instance.type = type->valuestring;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    wifi_scan_req_instance.request_id = request_id->valuestring;

    cJSON *data = cJSON_GetObjectItem(root, "data");
    wifi_scan_req_instance.rescan = cJSON_IsTrue(cJSON_GetObjectItem(data, "rescan"));

    ret = wifi_scan_execution(wifi_scan_req_instance.rescan); // 执行扫描操作

    // 根据执行结果构建响应数据
    wifi_scan_res_instance.type = wifi_dispatch_get_by_index(index)->response; // 使用响应类型
    wifi_scan_res_instance.success = (ret == WIFI_ERR_OK);                     // 设置成功标志
    wifi_scan_res_instance.error = ret;                                        // 设置错误码
    wifi_scan_res_instance.request_id = wifi_scan_req_instance.request_id;     // 回显request_id

    /**
     * {
     *  "type": "wifi_scan_response",
     *  "success": true,
     *  "error": 0,
     *  "data": {
     *    "networks": [
     *      {
     *        "ssid": "MyHomeNetwork",
     *        "bssid": "aa:bb:cc:dd:ee:ff",
     *        "signal": 78,
     *        "security": "WPA2",
     *        "channel": 6,
     *        "frequency_mhz": 2437
     *      }
     *    ]
     *  }
     * }
     */
    cJSON *response = cJSON_CreateObject();
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", wifi_scan_res_instance.type);
    cJSON_AddStringToObject(response, "request_id", wifi_scan_res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", wifi_scan_res_instance.success);
    cJSON_AddNumberToObject(response, "error", wifi_scan_res_instance.error);

    // 添加网络列表
    cJSON *networks_array = cJSON_CreateArray();
    for (size_t i = 0; i < wifi_scan_res_instance.network_count; i++)
    {
        cJSON *network_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(network_obj, "ssid",
                                (wifi_scan_res_instance.networks[i].ssid && strlen(wifi_scan_res_instance.networks[i].ssid) > 0) ? wifi_scan_res_instance.networks[i].ssid : "");
        cJSON_AddStringToObject(network_obj, "bssid", wifi_scan_res_instance.networks[i].bssid);
        cJSON_AddNumberToObject(network_obj, "signal", wifi_scan_res_instance.networks[i].signal);
        cJSON_AddStringToObject(network_obj, "security", wifi_scan_res_instance.networks[i].security);
        cJSON_AddNumberToObject(network_obj, "channel", wifi_scan_res_instance.networks[i].channel);
        cJSON_AddNumberToObject(network_obj, "frequency_mhz", wifi_scan_res_instance.networks[i].frequency_mhz);
        cJSON_AddBoolToObject(network_obj, "recorded", wifi_scan_res_instance.networks[i].recorded);
        cJSON_AddItemToArray(networks_array, network_obj);
    }

    cJSON_AddItemToObject(res_data, "networks", networks_array);
    cJSON_AddItemToObject(response, "data", res_data);

    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_scan: Failed to print response\n");
        cJSON_Delete(response);
        return;
    }
    else
    {
        printf("wifi_scan: %s\n", response_str);
        unsigned char buf[LWS_PRE + strlen(response_str)];
        memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
        int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        if (n < 0)
        {
            printf("wifi_scan: Failed to write response\n");
        }
        cJSON_free(response_str);
    }

    cJSON_Delete(response);

    // 释放动态分配的内存
    if (wifi_scan_res_instance.networks)
    {
        for (size_t i = 0; i < wifi_scan_res_instance.network_count; i++)
        {
            if (wifi_scan_res_instance.networks[i].ssid)
                free(wifi_scan_res_instance.networks[i].ssid);
            if (wifi_scan_res_instance.networks[i].bssid)
                free(wifi_scan_res_instance.networks[i].bssid);
            if (wifi_scan_res_instance.networks[i].security)
                free(wifi_scan_res_instance.networks[i].security);
        }
        free(wifi_scan_res_instance.networks);
        wifi_scan_res_instance.networks = NULL;
        wifi_scan_res_instance.network_count = 0;
    }
}