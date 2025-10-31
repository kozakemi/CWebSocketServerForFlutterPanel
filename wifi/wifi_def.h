#ifndef __WIFI_DEF_H__
#define __WIFI_DEF_H__

#include <stddef.h>
#define WIFI_DEVICE "wlan0"
typedef enum {
  WIFI_ERR_OK = 0,
  WIFI_ERR_UNKNOWN = -1,

  WIFI_ERR_BAD_REQUEST = 1,        // 数据缺失/类型不符
  WIFI_ERR_NOT_SUPPORTED = 2,      // 操作不支持
  WIFI_ERR_WIFI_DISABLED = 3,      // Wi‑Fi 已关闭
  WIFI_ERR_ALREADY_CONNECTED = 4,  // 已连接同一 SSID
  WIFI_ERR_NETWORK_NOT_FOUND = 5,  // 扫描无该 SSID
  WIFI_ERR_AUTH_FAILED = 6,        // 认证失败/密码错误
  WIFI_ERR_TIMEOUT = 7,            // 连接/操作超时
  WIFI_ERR_INTERNAL = 8,           // 后端内部错误
  WIFI_ERR_PERMISSION = 9,         // 权限不足
  WIFI_ERR_BUSY = 10,              // 设备繁忙/正在进行其他操作
  WIFI_ERR_INVALID_SSID = 11,
  WIFI_ERR_INVALID_PASSWORD = 12,
  WIFI_ERR_INTERFACE_DOWN = 13,    // 网卡未启动/无效
  WIFI_ERR_TOOL_ERROR = 14,        // 底层工具错误（nmcli/wpa_cli）
  WIFI_ERR_CONFIG_ERROR = 15,      // 配置存储/读取失败
  WIFI_ERR_IO_ERROR = 16,          // I/O 异常
  WIFI_ERR_NOT_CONNECTED = 17      // 未连接
} wifi_error_t;
#endif