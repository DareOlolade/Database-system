#include "os.h"

#include <stdio.h>
#include <stdlib.h>		//for size_t
#include <fcntl.h>		//for 
#include <sys/stat.h>		//for stat
#include <unistd.h>		//for pread, pwrite  


int os_open(const char *filepath, int flags, int mode){
	int fd = open(filepath, flags, mode);
	if(fd < 0){
		perror("Error opening db");
		return -1;
	}
	return fd;
}

int os_close(int fd){
	int check = close(fd);
	if(check != 0){
		perror("Error closing db");
		return -1;
	}
	return check;
}

ssize_t os_read(int fd, void *buffer, size_t size, int64_t offset){
	ssize_t bytes_read = pread(fd, buffer, size, offset);
	if (bytes_read == -1){
		perror("Error reading db");
		return -1;
	}

	return bytes_read;
}

ssize_t os_write(int fd, const void *buffer, size_t size, int64_t offset){
	ssize_t bytes_written = pwrite(fd, buffer, size, offset);

	if (bytes_written == -1){
		perror("Error writting to db");
		return -1;
	}

	return bytes_written;
}



int64_t os_file_size(int fd){
	struct stat st;

	if(fstat(fd, &st) == 0){
		return st.st_size;
	}
	perror("Cannot get file size");
	return -1;
}
