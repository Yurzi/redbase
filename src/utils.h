#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>

namespace redbase {
int32_t open(const char *pathname, int32_t flags);
int32_t open(const char *pathname, int32_t flags, mode_t mode);
ssize_t read(int32_t fd, void *buf, size_t count);
ssize_t write(int32_t fd, const void *buf, size_t count);
off_t lseek(int32_t fd, off_t offset, int32_t whence);
void close(int32_t fd);
int32_t unlink(const char *pathname);
char *getcwd(char *buf, size_t size);
int32_t chdir(const char *path);
}  // namespace redbase

#endif  // ! __UTILS_H__
