#ifndef PAGER_H
#define PAGER_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096 

typedef struct {
	char data[PAGE_SIZE];
} pageBuffer;

struct Pager;
typedef struct Pager* Pager;

Pager pager_open(const char *filename);
void pager_close(Pager pager);
void pager_read_page(Pager pager, uint32_t page_num, PageBuffer *out_buffer);
void pager_write_page(Pager pager, uint32_t page_num, const PageBuffer *in_buffer);
uint32_t pager_page_count(Pager pager);

#endif
