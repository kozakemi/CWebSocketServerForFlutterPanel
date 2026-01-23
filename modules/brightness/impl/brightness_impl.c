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

#include "brightness_impl.h"
#include "../brightness_def.h"
#include <errno.h>
#include <stdio.h>

/**
 * @brief 读取值转int
 */
static brightness_error_t read_int_file(const char *path, int *out)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        if (errno == EACCES || errno == EPERM)
        {
            return BRIGHTNESS_ERR_PERMISSION;
        }
        if (errno == ENOENT)
        {
            return BRIGHTNESS_ERR_NOT_SUPPORTED;
        }
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
 */
static brightness_error_t write_int_file(const char *path, int value)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        if (errno == EACCES || errno == EPERM)
        {
            return BRIGHTNESS_ERR_PERMISSION;
        }
        if (errno == ENOENT)
        {
            return BRIGHTNESS_ERR_NOT_SUPPORTED;
        }
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

brightness_error_t brightness_impl_set(int percent)
{
    if (percent < 0 || percent > 100)
    {
        return BRIGHTNESS_ERR_INVALID_VALUE;
    }
    int max = 0;
    brightness_error_t err = read_int_file(BRIGHTNESS_SYSFS_MAX_BRIGHTNESS_PATH, &max);
    if (err != BRIGHTNESS_ERR_OK)
    {
        return err;
    }
    if (max <= 0)
    {
        return BRIGHTNESS_ERR_DEVICE_ERROR;
    }
    int raw = (int)((double)percent * (double)max / 100.0 + 0.5);
    if (raw < 0)
    {
        raw = 0;
    }
    if (raw > max)
    {
        raw = max;
    }
    err = write_int_file(BRIGHTNESS_SYSFS_BRIGHTNESS_PATH, raw);
    return err;
}

brightness_error_t brightness_impl_get_status(int *percent)
{
    if (!percent)
    {
        return BRIGHTNESS_ERR_BAD_REQUEST;
    }

    int raw = 0;
    int max = 0;
    brightness_error_t err = read_int_file(BRIGHTNESS_SYSFS_BRIGHTNESS_PATH, &raw);
    if (err != BRIGHTNESS_ERR_OK)
    {
        return err;
    }
    err = read_int_file(BRIGHTNESS_SYSFS_MAX_BRIGHTNESS_PATH, &max);
    if (err != BRIGHTNESS_ERR_OK)
    {
        return err;
    }
    if (max <= 0)
    {
        return BRIGHTNESS_ERR_DEVICE_ERROR;
    }

    int percent_value = (int)((double)raw * 100.0 / (double)max + 0.5);
    if (percent_value < 0)
    {
        percent_value = 0;
    }
    if (percent_value > 100)
    {
        percent_value = 100;
    }
    *percent = percent_value;
    return BRIGHTNESS_ERR_OK;
}