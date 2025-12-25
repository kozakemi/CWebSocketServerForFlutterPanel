#ifndef WS_UTILS_H
#define WS_UTILS_H

#include <libwebsockets.h>

/**
 * @brief Send a UTF-8 text message via libwebsockets.
 *
 * Allocates a temporary buffer sized LWS_PRE + strlen(text) and frees it after
 * sending. Returns the value from lws_write(), or -1 on allocation failure.
 */
int ws_send_text(struct lws *wsi, const char *text);

#endif

