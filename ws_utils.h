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

#ifndef WS_UTILS_H
#define WS_UTILS_H

#include "civetweb.h"

/**
 * @brief Send a UTF-8 text message via civetweb.
 *
 * Sends a text message using mg_websocket_write().
 * Returns the value from mg_websocket_write(), or -1 on error.
 */
int ws_send_text(struct mg_connection *conn, const char *text);

#endif

