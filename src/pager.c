#include "pager.h"
#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_PAGE_CACHE 2

struct PageCacheEntry{
	uint32_t page_num;
	PageBuffer buffer;
	bool in_use;
};


struct Pager{
	int fd;
	uint32_t page_count;
	struct PageCacheEntry cache[MAX_PAGE_CACHE];
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
	for (int i = 0; i< MAX_PAGE_CACHE; i++){
		pager->cache[i].in_use = false;
	}

	int64_t file_size = os_file_size(fd);
	if(file_size < 0){
		os_close(fd);
		free(pager);
		return NULL;
	}
	pager->page_count = (uint32_t)((file_size + PAGE_SIZE - 1) / PAGE_SIZE);
	return pager;
}

void pager_close(Pager pager){
	if(pager == NULL) return;
	os_close(pager->fd);
	free(pager);
}

void pager_write_page(Pager pager, uint32_t page_num, const PageBuffer *in_buffer){
	if(pager == NULL || in_buffer == NULL) return;

	int64_t offset = (int64_t)page_num * PAGE_SIZE;

	ssize_t bytes_written = os_write(pager->fd, in_buffer->data, PAGE_SIZE, offset);
	if (bytes_written != PAGE_SIZE){
		perror("Error writting file");
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

	
PageBuffer* pager_get_page(Pager pager, uint32_t page_num){
	if (pager == NULL) return NULL;

	for (int i = 0; i < MAX_PAGE_CACHE; i++){
		if (pager->cache[i].in_use && pager->cache[i].page_num == page_num){
			return &(pager->cache[i].buffer);
		}
	}


	//cache miss
	int empty_slot_index = -1;
	for (int i = 0; i < MAX_PAGE_CACHE; i++){
		if (pager->cache[i].in_use == false){
			empty_slot_index = i;
			break;
		}
	}


	if (empty_slot_index == -1){
		fprintf(stderr, "cache full. \n");
		return NULL;
	}

	int64_t offset = (int64_t)page_num * PAGE_SIZE;
	struct PageCacheEntry *slot = &pager->cache[empty_slot_index];

    	ssize_t bytes_read = os_read(pager->fd, slot->buffer.data, PAGE_SIZE, offset);
    	if (bytes_read < 0) {
        	perror("Unable to read file into cache");
		return NULL;
    	}

    	// Handle partial page reads by zeroing out the remainder
    	if (bytes_read < PAGE_SIZE) {
        	for (ssize_t i = bytes_read; i < PAGE_SIZE; i++) {
            		slot->buffer.data[i] = 0;
        	}
    	}

    	// 5. Update slot metadata and return
    	slot->in_use = true;
    	slot->page_num = page_num;

    	return &(slot->buffer);
	
}
int main(void) {
    Pager p = pager_open("scratch.db");

    PageBuffer *first  = pager_get_page(p, 0);
    printf("first call ptr: %p\n", (void*)first);

    PageBuffer *second = pager_get_page(p, 0);
    printf("second call ptr: %p\n", (void*)second);

    pager_close(p);
    return 0;
}
