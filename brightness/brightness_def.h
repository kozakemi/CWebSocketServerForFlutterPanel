#ifndef __BRIGHT_DEF_H__
#define __BRIGHT_DEF_H__

#include <stddef.h>

// 亮度设备目录可配置：默认 /sys/class/backlight/backlight
// 可在此处修改，或在编译时通过 -DBRIGHTNESS_SYSFS_DIR="\"/sys/class/backlight/intel_backlight\"" 覆盖
#ifndef BRIGHTNESS_SYSFS_DIR
#define BRIGHTNESS_SYSFS_DIR "/sys/class/backlight/backlight"
#endif

// 派生的具体文件路径宏
#define BRIGHTNESS_SYSFS_BRIGHTNESS_PATH      BRIGHTNESS_SYSFS_DIR "/brightness"
#define BRIGHTNESS_SYSFS_MAX_BRIGHTNESS_PATH  BRIGHTNESS_SYSFS_DIR "/max_brightness"

typedef enum {
  BRIGHTNESS_ERR_OK = 0,
  BRIGHTNESS_ERR_UNKNOWN = -1,
  BRIGHTNESS_ERR_BAD_REQUEST = 1,        // 数据缺失/类型不符
  BRIGHTNESS_ERR_INVALID_VALUE = 2,      // 亮度值无效（超出0-100范围）
  BRIGHTNESS_ERR_NOT_SUPPORTED = 3,      // 设备不支持亮度调节
  BRIGHTNESS_ERR_PERMISSION = 4,         // 权限不足
  BRIGHTNESS_ERR_DEVICE_ERROR = 5,       // 硬件设备错误
  BRIGHTNESS_ERR_INTERNAL = 6            // 后端内部错误
} brightness_error_t;

#endif