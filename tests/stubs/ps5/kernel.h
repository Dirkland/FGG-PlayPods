#ifndef TEST_PS5_KERNEL_H
#define TEST_PS5_KERNEL_H
#include <stdint.h>
#include <sys/types.h>
intptr_t kernel_dynlib_resolve(pid_t pid, uint32_t handle, const char *nid);
#endif
