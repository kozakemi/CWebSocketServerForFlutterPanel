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

#ifndef BRIGHTNESS_IMPL_H
#define BRIGHTNESS_IMPL_H

#include "../brightness_def.h"
// 亮度设备目录可配置：默认 /sys/class/backlight/backlight
// 可在此处修改，或在编译时通过 -DBRIGHTNESS_SYSFS_DIR="\"/sys/class/backlight/intel_backlight\""
// 覆盖
#ifndef BRIGHTNESS_SYSFS_DIR
#define BRIGHTNESS_SYSFS_DIR "/sys/class/backlight/backlight"
#endif

// 派生的具体文件路径宏
#define BRIGHTNESS_SYSFS_BRIGHTNESS_PATH BRIGHTNESS_SYSFS_DIR "/brightness"
#define BRIGHTNESS_SYSFS_MAX_BRIGHTNESS_PATH BRIGHTNESS_SYSFS_DIR "/max_brightness"

/**
 * @brief 设置屏幕亮度
 *
 * @param percent 亮度百分比（0-100）
 * @return brightness_error_t 错误码
 */
brightness_error_t brightness_impl_set(int percent);

/**
 * @brief 获取屏幕亮度
 *
 * @param percent 输出参数，亮度百分比（0-100）
 * @return brightness_error_t 错误码
 */
brightness_error_t brightness_impl_get_status(int *percent);

#endif