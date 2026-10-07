#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "../src/capture.c"

static char messages[8192];
static size_t used;
static int init_result, open_result, start_result, read_result, cleanup_result;
static int null_handle, opens, starts, stops, closes, terms, reads;
static int sentinel;

void log_line(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(messages + used, sizeof messages - used, fmt, ap);
    va_end(ap);
    assert(n >= 0 && (size_t)n + 1 < sizeof messages - used);
    used += (size_t)n;
    messages[used++] = '\n';
    messages[used] = 0;
}
int sceKernelLoadStartModule(const char *p, size_t n, const void *v,
                            uint32_t f, void *o, int *r)
{
    (void)p; (void)n; (void)v; (void)f; (void)o; (void)r;
    assert(!"module loading must not run in host tests");
    return -1;
}
intptr_t kernel_dynlib_resolve(pid_t p, uint32_t h, const char *n)
{
    (void)p; (void)h; (void)n;
    assert(!"console symbol resolution must not run in host tests");
    return 0;
}
static int initialize(void) { return init_result; }
static int terminate(void) { terms++; return cleanup_result; }
static int open_audio(void **out, const void *p, uint8_t kind)
{
    const open_param *param = p;
    assert(kind == 0 && param->size == 0x38 && param->source == 2 && param->mode == 0);
    opens++;
    *out = (open_result < 0 || null_handle) ? NULL : &sentinel;
    return open_result;
}
static int start(void *h) { assert(h == &sentinel); starts++; return start_result; }
static int stop(void *h) { assert(h == &sentinel); stops++; return cleanup_result; }
static int close_audio(void *h) { assert(h == &sentinel); closes++; return cleanup_result; }
static int read_audio(void *h, void *b, size_t s, void *i, uint32_t f)
{
    assert(h == &sentinel && b && s && i && f == READ_NOWAIT);
    reads++;
    return read_result;
}
static void reset(void)
{
    assert(!g_handle && !g_started && !g_initialized);
    used = 0; messages[0] = 0;
    opens = starts = stops = closes = terms = reads = 0;
    init_result = open_result = start_result = cleanup_result = null_handle = 0;
}
int main(void)
{
    float buf[CAPTURE_RECORD / sizeof(float)];
    p_initialize = initialize; p_terminate = terminate; p_open_audio = open_audio;
    p_start = start; p_stop = stop; p_close = close_audio; p_read_audio = read_audio;

    reset();
    null_handle = 1;
    assert(start_capture() == 0);
    assert(strstr(messages, "success without a handle"));
    assert(opens == 1 && starts == 0 && closes == 0 && terms == 1);

    reset();
    init_result = -5;
    assert(start_capture() == 0);
    assert(opens == 0 && terms == 0);
    init_result = 0; open_result = -5;
    assert(start_capture() == 0);
    /* Identical codes at different stages must both remain visible. */
    assert(strstr(messages, "Initialize ->") && strstr(messages, "OpenAudio ->"));
    assert(closes == 0 && terms == 1);

    reset();
    start_result = -7; cleanup_result = -9;
    assert(start_capture() == 0);
    const char *primary = strstr(messages, "Start ->");
    const char *secondary = strstr(messages, "cleanup Close ->");
    assert(primary && secondary && primary < secondary);
    assert(strstr(messages, "cleanup Terminate ->"));
    assert(stops == 0 && closes == 1 && terms == 1);
    capture_close();
    assert(closes == 1 && terms == 1);

    reset();
    assert(start_capture() == 1);
    read_result = sizeof buf;
    assert(capture_read(buf, sizeof buf) == (int)sizeof buf);
    read_result = 8; assert(capture_read(buf, sizeof buf) == 8);
    read_result = 0; assert(capture_read(buf, sizeof buf) == 0);
    read_result = sizeof buf + 8; assert(capture_read(buf, sizeof buf) == -1);
    read_result = 7; assert(capture_read(buf, sizeof buf) == -1);
    read_result = (int)ERR_EMPTY; assert(capture_read(buf, sizeof buf) == 0);
    read_result = (int)ERR_OVERRUN; assert(capture_read(buf, sizeof buf) == 0);
    assert(capture_overruns() == 1);
    read_result = -8; assert(capture_read(buf, sizeof buf) == -1);
    int calls = reads;
    assert(capture_read(NULL, sizeof buf) == -1);
    assert(capture_read(buf, 1) == -1 && reads == calls);
    cleanup_result = -9;
    capture_close(); capture_close();
    assert(stops == 1 && closes == 1 && terms == 1);
    assert(strstr(messages, "cleanup Stop ->"));
    assert(strstr(messages, "cleanup Close ->"));
    assert(strstr(messages, "cleanup Terminate ->"));
    assert(capture_read(buf, sizeof buf) == -1 && reads == calls);
    puts("Capture boundaries and cleanup checks passed (native calls mocked)");
    return 0;
}
