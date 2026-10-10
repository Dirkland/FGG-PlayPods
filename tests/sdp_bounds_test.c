#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/sdp.c"

void log_line(const char *fmt, ...) { (void)fmt; }

int main(void)
{
    unsigned char giant[] = {0x37,0x7f,0xff,0xff,0xff};
    unsigned char negative[] = {0x37,0xff,0xff,0xff,0xff};
    unsigned char valid[] = {2,0,1,0,8,0x35,3,0x19,0x11,0x0a,0,1,0};
    unsigned char bad[] = {2,0,1,0,5,0x37,0xff,0xff,0xff,0xff};
    unsigned char response[64];
    int type, content, n;
    assert(de_header(giant, sizeof giant, &type, &content) == 0);
    assert(de_header(negative, sizeof negative, &type, &content) == 0);
    n = sdp_handle(valid, sizeof valid, response, sizeof response);
    assert(n == 14 && response[0] == PDU_SEARCH_RSP);
    n = sdp_handle(bad, sizeof bad, response, sizeof response);
    assert(n == 7 && response[0] == PDU_ERROR);
    for (n = 0; n < (int)sizeof valid; n++) {
        unsigned char *p = malloc((size_t)(n ? n : 1));
        memcpy(p, valid, (size_t)n);
        int r = sdp_handle(p, n, response, sizeof response);
        assert(r >= 0 && r <= (int)sizeof response);
        free(p);
    }
    puts("SDP length and valid-response checks passed");
    return 0;
}
