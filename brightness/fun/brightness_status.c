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

#include "../../lib/cJSON/cJSON.h"
#include "../brightness_def.h"
#include "../brightness_scheduler.h"
#include <libwebsockets.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/************* 请求 ***************/

typedef struct
{
  char *type;
  char *request_id;
} brightness_status_req;

/************* 响应 ***************/
typedef struct
{
  int brightness;
} brightness_status_res_data;

typedef struct
{
  char *type;
  char *request_id;
  int success;
  int error;
  brightness_status_res_data data;
} brightness_status_res;

brightness_status_req brightness_status_req_instance; ///< 亮度状态请求实例
brightness_status_res brightness_status_res_instance; ///< 亮度状态响应实例

static brightness_error_t read_int_file(const char *path, int *out)
{
  FILE *fp = fopen(path, "r");
  if (!fp)
  {
    if (errno == EACCES || errno == EPERM)
      return BRIGHTNESS_ERR_PERMISSION;
    if (errno == ENOENT)
      return BRIGHTNESS_ERR_NOT_SUPPORTED;
    return BRIGHTNESS_ERR_DEVICE_ERROR;
  }
  int value = -1;
  if (fscanf(fp, "%d", &value) != 1)
  {
    fclose(fp);
    return BRIGHTNESS_ERR_DEVICE_ERROR;
  }
  fclose(fp);
  *out = value;
  return BRIGHTNESS_ERR_OK;
}

/**
 * @brief 执行亮度状态请求
 *
 * @param req 亮度状态请求结构体指针
 * @param res 亮度状态响应结构体指针
 * @return brightness_error_t 执行结果
 */
static brightness_error_t brightness_status_execution(void)
{
  int raw = 0;
  int max = 0;
  brightness_error_t err = read_int_file(BRIGHTNESS_SYSFS_BRIGHTNESS_PATH, &raw);
  if (err != BRIGHTNESS_ERR_OK)
    return err;
  err = read_int_file(BRIGHTNESS_SYSFS_MAX_BRIGHTNESS_PATH, &max);
  if (err != BRIGHTNESS_ERR_OK)
    return err;
  if (max <= 0)
    return BRIGHTNESS_ERR_DEVICE_ERROR;

  // 计算百分比（四舍五入到 0-100）
  int percent = (int)((double)raw * 100.0 / (double)max + 0.5);
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  brightness_status_res_instance.data.brightness = percent;
  return BRIGHTNESS_ERR_OK;
}

/**
 * @brief 处理亮度状态请求
 *
 * @param wsi WebSocket 实例指针
 * @param index 调度索引
 * @param root JSON 根对象指针
 */
void brightness_status(struct lws *wsi, size_t index, cJSON *root)
{
  int ret = 0;
  // 解析请求 { "type": "brightness_status_request", "request_id": "req-1", "data": {} }
  cJSON *type = cJSON_GetObjectItem(root, "type");
  brightness_status_req_instance.type = type->valuestring;
  cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
  brightness_status_req_instance.request_id = request_id->valuestring;

  // 执行请求
  ret = brightness_status_execution();

  // 构建响应
  brightness_status_res_instance.type = brightness_dispatch_get_by_inedx(index)->response;
  brightness_status_res_instance.request_id = brightness_status_req_instance.request_id;
  brightness_status_res_instance.success = (ret == BRIGHTNESS_ERR_OK);
  brightness_status_res_instance.error = ret;
  /**
   *{
   *  "type": "brightness_status_response",
   *  "request_id": "req-1",
   *  "success": true,
   *  "error": 0,
   *  "data": {
   *    "brightness": 75,           // 当前亮度百分比 0-100
   *  }
   *}
   */
  cJSON *response = cJSON_CreateObject();
  cJSON *res_data = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "type", brightness_status_res_instance.type);
  cJSON_AddStringToObject(response, "request_id", brightness_status_res_instance.request_id);
  cJSON_AddBoolToObject(response, "success", brightness_status_res_instance.success);
  cJSON_AddNumberToObject(response, "error", brightness_status_res_instance.error);
  cJSON_AddNumberToObject(res_data, "brightness", brightness_status_res_instance.data.brightness);
  cJSON_AddItemToObject(response, "data", res_data);

  char *response_str = cJSON_PrintUnformatted(response); // 紧凑格式
  if (!response_str)
  {
    printf("brig_statue: Failed to print response\n");
    return;
  }
  else
  {
    printf("brig_statue: %s\n", response_str);
    unsigned char buf[LWS_PRE + strlen(response_str)];
    memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
    int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
    if (n < 0)
    {
      printf("brig_statue: Failed to write response\n");
    }
    cJSON_free(response_str);
  }
  cJSON_Delete(response);
}
