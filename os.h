#ifndef OS_UNIX
#define OS_UNIX

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>		//for ssize_t

int 	os_open(const char *filepath, int flags, int mode);
int	os_close(int fd);
ssize_t os_read(int fd, void *buffer, size_t size, int64_t offset);
ssize_t os_write(int fd, const void *buffer, size_t size, int64_t offset);
int64_t os_file_size(int fd);

#endif
