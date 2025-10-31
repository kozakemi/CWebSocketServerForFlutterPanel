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
  int brightness; // percent 0-100
} brightness_set_req_data;

typedef struct
{
  char *type;
  char *request_id;
  brightness_set_req_data data;
} brightness_set_req;
/************* 响应 ***************/
typedef struct
{
  char *type;
  char *request_id;
  bool success;
  int error;
} brightness_set_res;

static brightness_set_req brightness_set_req_instance; ///< 请求实例
static brightness_set_res brightness_set_res_instance; ///< 响应实例

/**
 * @brief 读取值转int
 * 
 * @param path 
 * @param out 
 * @return brightness_error_t 
 */
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
 * @brief 写值为int
 * 
 * @param path 
 * @param value 
 * @return brightness_error_t 
 */
static brightness_error_t write_int_file(const char *path, int value)
{
  FILE *fp = fopen(path, "w");
  if (!fp)
  {
    if (errno == EACCES || errno == EPERM)
      return BRIGHTNESS_ERR_PERMISSION;
    if (errno == ENOENT)
      return BRIGHTNESS_ERR_NOT_SUPPORTED;
    return BRIGHTNESS_ERR_DEVICE_ERROR;
  }
  int rc = fprintf(fp, "%d", value);
  if (rc < 0)
  {
    fclose(fp);
    return BRIGHTNESS_ERR_DEVICE_ERROR;
  }
  fclose(fp);
  return BRIGHTNESS_ERR_OK;
}

/**
 * @brief 执行设置亮度
 * 
 * @param percent 
 * @return brightness_error_t 
 */
static brightness_error_t brightness_set_execution(int percent)
{
  if (percent < 0 || percent > 100)
    return BRIGHTNESS_ERR_INVALID_VALUE;
  int max = 0;
  brightness_error_t err = read_int_file(BRIGHTNESS_SYSFS_MAX_BRIGHTNESS_PATH, &max);
  if (err != BRIGHTNESS_ERR_OK)
    return err;
  if (max <= 0)
    return BRIGHTNESS_ERR_DEVICE_ERROR;
  int raw = (int)((double)percent * (double)max / 100.0 + 0.5); // round
  if (raw < 0)
    raw = 0;
  if (raw > max)
    raw = max;
  err = write_int_file(BRIGHTNESS_SYSFS_BRIGHTNESS_PATH, raw);
  return err;
}

/**
 * @brief 设置亮度
 * 
 * @param wsi 
 * @param index 
 * @param root 
 */
void brightness_set(struct lws *wsi, size_t index, cJSON *root)
{
  int ret = 0;
  // 解析 { "type": "brightness_set_request", "request_id": "req-2", "data": { "brightness": 80 } }
  cJSON *type = cJSON_GetObjectItem(root, "type");
  brightness_set_req_instance.type = type ? type->valuestring : NULL;
  cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
  brightness_set_req_instance.request_id = request_id ? request_id->valuestring : NULL;
  cJSON *data = cJSON_GetObjectItem(root, "data");
  cJSON *brightness_item = data ? cJSON_GetObjectItem(data, "brightness") : NULL;
  if (!brightness_item || !cJSON_IsNumber(brightness_item))
  {
    ret = BRIGHTNESS_ERR_BAD_REQUEST;
  }
  else
  {
    brightness_set_req_instance.data.brightness = brightness_item->valueint;
    // 执行设置亮度
    ret = brightness_set_execution(brightness_set_req_instance.data.brightness);
  }

  // 构建响应
  brightness_set_res_instance.type = brightness_dispatch_get_by_inedx(index)->response;
  brightness_set_res_instance.request_id = brightness_set_req_instance.request_id;
  brightness_set_res_instance.error = ret;
  brightness_set_res_instance.success = (ret == BRIGHTNESS_ERR_OK);

  cJSON *response = cJSON_CreateObject();
  cJSON *res_data = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "type", brightness_set_res_instance.type);
  cJSON_AddStringToObject(response, "request_id", brightness_set_res_instance.request_id);
  cJSON_AddBoolToObject(response, "success", brightness_set_res_instance.success);
  cJSON_AddNumberToObject(response, "error", brightness_set_res_instance.error);
  
  if (ret == BRIGHTNESS_ERR_OK)
  {
    cJSON_AddNumberToObject(res_data, "brightness", brightness_set_req_instance.data.brightness);
  }
  cJSON_AddItemToObject(response, "data", res_data);

  char *response_str = cJSON_PrintUnformatted(response);
  if (!response_str)
  {
    printf("brightness_set: Failed to print response\n");
    cJSON_Delete(response);
    return;
  }
  printf("brightness_set: %s\n", response_str);
  unsigned char buf[LWS_PRE + strlen(response_str)];
  memcpy(&buf[LWS_PRE], response_str, strlen(response_str));
  int n = lws_write(wsi, &buf[LWS_PRE], strlen(response_str), LWS_WRITE_TEXT);
  if (n < 0)
  {
    printf("brightness_set: Failed to write response\n");
  }
  cJSON_free(response_str);
  cJSON_Delete(response);
}