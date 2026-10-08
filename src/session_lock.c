#include "session_lock.h"
#include "log.h"

#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void session_lock_release(int *owned_fd)
{
    int fd = *owned_fd;
    *owned_fd = -1;
    if (fd >= 0 && close(fd) < 0)
        log_line("lock: close failed: %s; release uncertain, not retried",
                 strerror(errno));
}

int session_lock_take(const char *path, int *owned_fd)
{
    int fd, saved;
    if (*owned_fd >= 0) {
        log_line("lock: this session already owns a descriptor");
        return -1;
    }
    fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        log_line("lock: open failed: %s", strerror(errno));
        return -1;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        *owned_fd = fd;
        return 1;
    }
    saved = errno;
    if (saved == EWOULDBLOCK || saved == EAGAIN)
        log_line("lock: another copy holds the session lock");
    else
        log_line("lock: acquisition failed: %s", strerror(saved));
    session_lock_release(&fd);
    return (saved == EWOULDBLOCK || saved == EAGAIN) ? 0 : -1;
}
