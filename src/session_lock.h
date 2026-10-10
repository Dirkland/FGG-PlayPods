#ifndef FGG_SESSION_LOCK_H
#define FGG_SESSION_LOCK_H

/* Initialize *owned_fd to -1. Returns 1 when acquired, 0 when busy, -1
 * on error. Keep the descriptor for the whole session. Never unlink the
 * lock file: a second inode would let another process acquire a new lock. */
int session_lock_take(const char *path, int *owned_fd);
/* Consumes ownership, attempts close once, and reports any uncertainty. */
void session_lock_release(int *owned_fd);

#endif
