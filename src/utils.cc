#include "utils.h"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

namespace redbase {
void unix_error(const char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(-1);
}

int32_t open(const char *pathname, int32_t flags) {
  int32_t rc = ::open(pathname, flags);
  if (rc < 0)
    unix_error("Open error");

  return rc;
}

int32_t open(const char *pathname, int32_t flags, mode_t mode) {
  int32_t rc = ::open(pathname, flags, mode);
  if (rc < 0)
    unix_error("Open error");

  return rc;
}

ssize_t read(int32_t fd, void *buf, size_t count) {
  ssize_t rc = ::read(fd, buf, count);
  if (rc < 0)
    unix_error("Read error");

  return rc;
}

ssize_t write(int32_t fd, const void *buf, size_t count) {
  ssize_t rc = ::write(fd, buf, count);
  if (rc < 0)
    unix_error("Write error");

  return rc;
}

off_t lseek(int32_t fd, off_t offset, int32_t whence) {
  off_t rc = ::lseek(fd, offset, whence);
  if (rc < 0)
    unix_error("Lssk error");

  return rc;
}

void close(int32_t fd) { int32_t rc = ::close(fd); }

int32_t unlink(const char *pathname) {
  int32_t rc = ::unlink(pathname);
  if (rc < 0)
    unix_error("Unlink error");

  return 0;
}

char *getcwd(char *buf, size_t size) {
  char *src = ::getcwd(buf, size);
  if (src == nullptr)
    unix_error("Getcwd error");
  return src;
}

int32_t chdir(const char *path) {
  int32_t rc = ::chdir(path);
  if (rc < 0)
    unix_error("Chdir error");
  return 0;
}
}  // namespace redbase
