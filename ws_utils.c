#include "ws_utils.h"

#include <stdlib.h>
#include <string.h>

int ws_send_text(struct lws *wsi, const char *text)
{
    if (!wsi || !text)
    {
        return -1;
    }

    const size_t len = strlen(text);
    unsigned char *buf = (unsigned char *)malloc(LWS_PRE + len);
    if (!buf)
    {
        return -1;
    }

    memcpy(&buf[LWS_PRE], text, len);
    const int n = lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
    free(buf);
    return n;
}

