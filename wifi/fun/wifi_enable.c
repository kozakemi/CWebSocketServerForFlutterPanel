#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libwebsockets.h>
#include "../../lib/cJSON/cJSON.h"
#include "../wifi_def.h"
#include "../wifi_scheduler.h"
#include "wifi_enable.h"



/************* 请求 ***************/
/**
 * @brief wifi开关数据结构体
 *
 */
typedef struct
{
    bool enable;
} wifi_enable_data;

/**
 * @brief wifi开关请求结构体
 *
 */
typedef struct
{
    char *type;
    char *request_id;
    wifi_enable_data data;
} wifi_enable_req;

/************* 响应 ***************/
/**
 * @brief wifi开关响应结构体
 *
 */
typedef struct
{
    char *type;
    char *request_id;
    bool success;
    int error;
    wifi_enable_data data;
} wifi_enable_res;

wifi_enable_req wifi_enable_req_instance;
wifi_enable_res wifi_enable_res_instance;

/**
 * @brief 执行wifi开关操作
 *
 * @param is_enable true：开启，false：关闭
 * @return int
 */
static wifi_error_t wifi_enable_execution(bool is_enable)
{
    // 获取当前状态
    FILE *fp;
    char buffer[128];
    bool current_state = false;

    // 获取WiFi接口当前状态
    char command[256];
    // 使用wpa_cli获取指定WiFi接口的状态信息，过滤出wpa_state字段的值
    // wpa_cli -i {interface} status: 获取WiFi接口状态
    // 2>/dev/null: 将错误输出重定向到空设备，避免显示错误信息
    // grep wpa_state: 过滤出包含wpa_state的行
    // cut -d= -f2: 以等号为分隔符，取出第二个字段（状态值）
    snprintf(command, sizeof(command), "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", WIFI_DEVICE);

    fp = popen(command, "r");
    if (fp != NULL)
    {
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            // 如果能获取到状态信息，则认为WiFi已启用
            current_state = (strstr(buffer, "COMPLETED") != NULL ||
                             strstr(buffer, "ASSOCIATED") != NULL ||
                             strstr(buffer, "ASSOCIATING") != NULL ||
                             strstr(buffer, "SCANNING") != NULL);
        }
        pclose(fp);
    }

    // 不等于当前状态
    if (current_state != is_enable)
    {
        // 执行wifi开关操作
        int result;
        if (is_enable)
        {
            // 启用WiFi
            // 重新配置wpa_supplicant，使其重新加载配置文件
            snprintf(command, sizeof(command), "wpa_cli -i %s reconfigure", WIFI_DEVICE);
            result = system(command);
            if (result != 0)
            {
                return WIFI_ERR_TOOL_ERROR;
            }
        }
        else
        {
            // 禁用WiFi
            // 禁用所有网络配置
            snprintf(command, sizeof(command), "wpa_cli -i %s disable_network all", WIFI_DEVICE);
            system(command);
            // 断开当前连接
            snprintf(command, sizeof(command), "wpa_cli -i %s disconnect", WIFI_DEVICE);
            result = system(command);
        }

        // 检查返回值
        if (result != 0)
        {
            printf("wifi_enable: Failed to %s WiFi\n", is_enable ? "enable" : "disable");
            return WIFI_ERR_TOOL_ERROR;
        }
    }

    // 检查状态
    printf("wifi_enable: %s WiFi\n", is_enable ? "enable" : "disable");
    return WIFI_ERR_OK;
}

/**
 * @brief wifi开关协议处理
 *
 * @param index 调度数组索引值
 * @param root json对象
 */
void wifi_enable(struct lws *wsi, size_t index, cJSON *root)
{
    int ret = 0;
    /*
     * {
     *    "type": "wifi_enable_request",
     *    "data": {
     *        "enable": true
     *    }
     * }
     */
    cJSON *type = cJSON_GetObjectItem(root, "type");
    wifi_enable_req_instance.type = type->valuestring;
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    wifi_enable_req_instance.request_id = request_id->valuestring;
    cJSON *data = cJSON_GetObjectItem(root, "data");
    wifi_enable_req_instance.data.enable = cJSON_IsTrue(cJSON_GetObjectItem(data, "enable"));

    // 执行wifi开关操作
    ret = wifi_enable_execution(wifi_enable_req_instance.data.enable);

    // 根据执行结果构建响应数据
    wifi_enable_res_instance.type = wifi_dispatch_get_by_index(index)->response; // 使用响应类型
    wifi_enable_res_instance.success = (ret == WIFI_ERR_OK);                     // 设置成功标志
    wifi_enable_res_instance.error = ret;                                        // 设置错误码
    wifi_enable_res_instance.data.enable = wifi_enable_req_instance.data.enable; // 设置数据
    wifi_enable_res_instance.request_id = wifi_enable_req_instance.request_id;   // 回显request_id

    /**
     * {
     *  "type": "wifi_enable_response",
     *  "success": true,
     *  "error": 0,
     *  "data": { "enabled": true }
     *}
     *
     */
    cJSON *response = cJSON_CreateObject();
    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", wifi_enable_res_instance.type);
    cJSON_AddStringToObject(response, "request_id", wifi_enable_res_instance.request_id);
    cJSON_AddBoolToObject(response, "success", wifi_enable_res_instance.success);
    cJSON_AddNumberToObject(response, "error", wifi_enable_res_instance.error);
    cJSON_AddBoolToObject(res_data, "enable", wifi_enable_res_instance.data.enable);
    cJSON_AddItemToObject(response, "data", res_data);

    char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
    if (!response_str)
    {
        printf("wifi_enable: Failed to print response\n");
        return;
    }
    else
    {
        printf("wifi_enable: %s\n", response_str);
        unsigned char buf[LWS_PRE + strlen(response_str)];
        memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
        int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
        if (n < 0)
        {
            printf("wifi_enable: Failed to write response\n");
        }
        cJSON_free(response_str);
    }
    cJSON_Delete(response);
}