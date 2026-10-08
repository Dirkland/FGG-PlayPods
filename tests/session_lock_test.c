#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utime.h>

static int fail_lock, fail_close, closes;
static char messages[2048];
static int test_flock(int fd, int op)
{
    if (fail_lock) { errno = EIO; return -1; }
    return flock(fd, op);
}
static int test_close(int fd)
{
    closes++;
    int result = close(fd);
    if (fail_close) { errno = EINTR; return -1; }
    return result;
}
#define flock test_flock
#define close test_close
#include "../src/session_lock.c"
#undef flock
#undef close

void log_line(const char *fmt, ...)
{
    size_t len = strlen(messages);
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(messages + len, sizeof messages - len, fmt, ap);
    va_end(ap);
    assert(n >= 0 && (size_t)n + 1 < sizeof messages - len);
    strcat(messages, "\n");
}
static void wait_ok(pid_t pid)
{
    int status;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
int main(void)
{
    char dir[] = "/tmp/playpods-lock-test-XXXXXX", path[256];
    int owned = -1, other = -1;
    assert(mkdtemp(dir));
    snprintf(path, sizeof path, "%s/session.lock", dir);
    assert(session_lock_take(path, &owned) == 1);
    int saved = owned;
    assert(session_lock_take(path, &owned) == -1 && owned == saved);
    /* Even an ancient timestamp cannot authorize a second owner. */
    struct utimbuf old = {1, 1};
    assert(utime(path, &old) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (!child) {
        close(owned); /* Drop inherited reference; test a distinct open. */
        assert(session_lock_take(path, &other) == 0 && other == -1);
        _exit(0);
    }
    wait_ok(child);
    struct stat before, after;
    assert(stat(path, &before) == 0);
    session_lock_release(&owned);
    assert(owned == -1 && stat(path, &after) == 0 && before.st_ino == after.st_ino);
    child = fork();
    assert(child >= 0);
    if (!child) {
        assert(session_lock_take(path, &other) == 1);
        _exit(0); /* OS releases lock when owner exits, without unlinking. */
    }
    wait_ok(child);
    assert(session_lock_take(path, &owned) == 1);
    fail_close = 1;
    int prior = closes;
    session_lock_release(&owned); session_lock_release(&owned);
    assert(owned == -1 && closes == prior + 1);
    assert(strstr(messages, "release uncertain, not retried"));
    messages[0] = 0;
    fail_lock = 1;
    assert(session_lock_take(path, &owned) == -1 && owned == -1);
    const char *primary = strstr(messages, "acquisition failed");
    const char *secondary = strstr(messages, "close failed");
    assert(primary && secondary && primary < secondary);
    fail_lock = fail_close = 0;
    assert(unlink(path) == 0 && rmdir(dir) == 0);
    assert(session_lock_take(path, &owned) == -1 && owned == -1);
    puts("Session lock contention, lifetime and failure checks passed (Linux fixtures)");
    return 0;
}
