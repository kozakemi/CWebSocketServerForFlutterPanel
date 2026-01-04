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

#include "ws_utils.h"

#include <stdlib.h>
#include <string.h>

/**
 * 通过给定的 mg_connection 发送一个文本 WebSocket 消息。
 *
 * @param conn 指向已连接的 WebSocket 连接对象；如果为 NULL 则不会发送。
 * @param text 要发送的以 NUL 结尾的文本负载；如果为 NULL 则不会发送。
 * @returns 发送操作返回的字节数（来自 mg_websocket_write）的整数值；在输入无效时返回 -1。
 */
int ws_send_text(struct mg_connection *conn, const char *text)
{
    if (!conn || !text)
    {
        return -1;
    }

    const size_t len = strlen(text);
    const int n = mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, text, len);
    return n;
}
