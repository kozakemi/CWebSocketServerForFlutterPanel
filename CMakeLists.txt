cmake_minimum_required(VERSION 3.10)
project(CWebSocketServerForFlutterPanel
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "WebSocket Server for Flutter Panel"
)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/cJSON
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/civetweb/include
)

# 添加 cJSON 子项目
add_subdirectory(lib/cJSON)

# 配置 civetweb：启用 WebSocket 支持
set(CIVETWEB_ENABLE_WEBSOCKETS ON CACHE BOOL "Enable websockets connections" FORCE)
set(CIVETWEB_ENABLE_SERVER_EXECUTABLE OFF CACHE BOOL "Disable building of the server executable" FORCE)
set(CIVETWEB_ENABLE_CXX OFF CACHE BOOL "Disable C++ wrappers" FORCE)
set(CIVETWEB_BUILD_TESTING OFF CACHE BOOL "Disable automated testing of civetweb" FORCE)

# 添加 civetweb 子项目
add_subdirectory(lib/civetweb)

# 源文件列表
set(SOURCES
    main.c
    ws_utils.c
    wifi/wifi_scheduler.c
    wifi/fun/wifi_enable.c
    wifi/fun/wifi_status.c
    wifi/fun/wifi_scan.c
    wifi/fun/wifi_connect.c
    wifi/fun/wifi_disconnect.c
    brightness/brightness_scheduler.c
    brightness/fun/brightness_status.c
    brightness/fun/brightness_set.c
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    cjson
    civetweb-c-library
    pthread
    rt
    m
)

# 安装规则（可选）
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

