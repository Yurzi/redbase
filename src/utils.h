#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

namespace redbase {
int32_t Open(const char *pathname, int32_t flags);
int32_t Open(const char *pathname, int32_t flags, mode_t mode);
ssize_t Read(int32_t fd, void *buf, size_t count);
ssize_t Write(int32_t fd, const void *buf, size_t count);
off_t Lseek(int32_t fd, off_t offset, int32_t whence);
void Close(int32_t fd);
int32_t Unlink(const char *pathname);
char *Getcwd(char *buf, size_t size);
int32_t Chdir(const char *path);
}  // namespace redbase

#endif  // ! __UTILS_H__
