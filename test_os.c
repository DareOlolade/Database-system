#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "os.h"

#define PAGE_SIZE 4096


extern int 	os_open(const char *filepath, int flags, int mode);
extern int	os_close(int fd);
extern ssize_t os_read(int fd, void *buffer, size_t size, int64_t offset);
extern ssize_t os_write(int fd, const void *buffer, size_t size, int64_t offset);
extern int64_t os_file_size(int fd);

int main(void){
	const char *test_filename = "test_os.bin";

  	const char *pattern0 = "PAGE_ZERO_PATTERN";
    	const char *pattern1 = "PAGE_ONE_PATTERN_ALT";
    	const char *pattern2 = "PAGE_TWO_PATTERN_HDR";

	size_t len0 = strlen(pattern0);
	size_t len1 = strlen(pattern1);
	size_t len2 = strlen(pattern2);
	
	char read_buf0[32] = {0};
	char read_buf1[32] = {0};
	char read_buf2[32] = {0};

 	unlink(test_filename);


	int fd = os_open(test_filename, O_CREAT | O_RDWR, 0644);
	assert(fd != -1 && "FAIL: os_open failed to create scratch file");

	// Write to Page 0 (Offset 0)
    	ssize_t bytes_written = os_write(fd, pattern0, len0, 0);
    	assert(bytes_written == (ssize_t)len0 && "FAIL: Short write or error on Page 0");

	// Write to Page 1 (Offset PAGE_SIZE)
    	bytes_written = os_write(fd, pattern1, len1, PAGE_SIZE);
    	assert(bytes_written == (ssize_t)len1 && "FAIL: Short write or error on Page 1");

	// Write to Page 2 (Offset 2 * PAGE_SIZE)
    	bytes_written = os_write(fd, pattern2, len2, 2 * PAGE_SIZE);
    	assert(bytes_written == (ssize_t)len2 && "FAIL: Short write or error on Page 2");

	// Close the file descriptor
    	int close_status = os_close(fd);
    	assert(close_status == 0 && "FAIL: os_close failed on write descriptor");


    	fd = os_open(test_filename, O_RDONLY, 0);
    	assert(fd != -1 && "FAIL: os_open failed to reopen file for reading");

	// Read and verify Page 0
    	ssize_t bytes_read = os_read(fd, read_buf0, len0, 0);
    	assert(bytes_read == (ssize_t)len0 && "FAIL: Short read or error on Page 0");
    	assert(memcmp(read_buf0, pattern0, len0) == 0 && "FAIL: Data mismatch on Page 0");

	// Read and verify Page 1
    	bytes_read = os_read(fd, read_buf1, len1, PAGE_SIZE);
    	assert(bytes_read == (ssize_t)len1 && "FAIL: Short read or error on Page 1");
    	assert(memcmp(read_buf1, pattern1, len1) == 0 && "FAIL: Data mismatch on Page 1");

	// Read and verify Page 2
    	bytes_read = os_read(fd, read_buf2, len2, 2 * PAGE_SIZE);
    	assert(bytes_read == (ssize_t)len2 && "FAIL: Short read or error on Page 2");
    	assert(memcmp(read_buf2, pattern2, len2) == 0 && "FAIL: Data mismatch on Page 2");

	// Close the read file descriptor
    	close_status = os_close(fd);
    	assert(close_status == 0 && "FAIL: os_close failed on read descriptor");
	int unlink_status = unlink(test_filename);
	assert(unlink_status == 0 && "FAIL: Could not delete scratch file after test run");

	printf("iteration 0: PASS\n");
	return 0;
}

