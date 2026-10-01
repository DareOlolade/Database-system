#include "pager.h"
#include "os.h"

#include <stdio.h>
#include <stdlib.h>

struct Pager{
	int fd;
	uint32_t page_count;
};

Pager pager_open(const char *filename){
	int fd = os_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if(fd<0){
		return NULL;
	}
	Pager pager = malloc(sizeof(struct Pager));
	if (pager == NULL){
		perror("Error allocating pager");
		os_close(fd);
		return NULL;
	} 
	pager->fd = fd;
	int64_t file_size = os_file_size(fd);

	if(file_size < 0){
		os_close(fd);
		free(pager);
		return NULL;
	}

	pager->page_count = (uint32_t)(file_size/PAGE_SIZE);
	return pager;
}

void pager_close(Pager pager){
	if(pager == NULL) return;
	os_close(pager->fd);
	free(pager);
}

void pager_read_page(Pager pager, uint32_t page_num, PageBuffer *out_buffer){
	if (pager == NULL || out_buffer == NULL) return;

	if(page_num >= pager->page_count){
		perror("Attempted to read out of bounds");
		return;
	}
	int64_t offset = (int64_t)page_num * PAGE_SIZE;

	ssize_t bytes_read = os_read(pager->fd, out_buffer->data, PAGE_SIZE, offset);
	if (bytes_read < 0){
		perror("Unable to read file");
		return;
	}
	if(bytes_read < PAGE_SIZE){
		for( ssize_t i = bytes_read; i< PAGE_SIZE; i++){
			out_buffer->data[i] = 0;
		}
	}
}


void pager_write_page(Pager pager, uint32_t page_num, const PageBuffer *in_buffer){
	if(pager == NULL || in_buffer == NULL) return;

	int64_t offset = (int64_t)page_num * PAGE_SIZE;

	ssize_t bytes_written = os_write(pager->fd, in_buffer->data, PAGE_SIZE, offset);
	if (bytes_written != PAGE_SIZE){
		if (bytes_written >= 0){
			perror("Error writting file");
		}
		return;
	}
	
	if (page_num >= pager->page_count) {
		pager->page_count = page_num + 1;
	}
	
}

uint32_t pager_page_count(Pager pager){
	if (pager == NULL) return 0;
	return pager->page_count;
}
