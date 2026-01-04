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

