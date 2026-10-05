#include "pager.h"
#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h> // Required for memcpy

#define MAX_PAGE_CACHE 2

struct PageCacheEntry {
    uint32_t page_num;
    PageBuffer buffer;
    bool in_use;
    bool dirty; // <-- Added to track pending disk updates
};

struct Pager {
    int fd;
    uint32_t page_count;
    struct PageCacheEntry cache[MAX_PAGE_CACHE];
};

Pager pager_open(const char *filename) {
    int fd = os_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return NULL;
    }
    Pager pager = malloc(sizeof(struct Pager));
    if (pager == NULL) {
        perror("Error allocating pager");
        os_close(fd);
        return NULL;
    } 
    pager->fd = fd;
    for (int i = 0; i < MAX_PAGE_CACHE; i++) {
        pager->cache[i].in_use = false;
        pager->cache[i].dirty = false; // <-- Initialized to false
    }

    int64_t file_size = os_file_size(fd);
    if (file_size < 0) {
        os_close(fd);
        free(pager);
        return NULL;
    }
    pager->page_count = (uint32_t)((file_size + PAGE_SIZE - 1) / PAGE_SIZE);
    return pager;
}

// pager_flush_all(pager): Flush all dirty cache slots to disk
void pager_flush_all(Pager pager) {
    if (pager == NULL) return;

    for (int i = 0; i < MAX_PAGE_CACHE; i++) {
        struct PageCacheEntry *slot = &pager->cache[i];
        
        // if slot.in_use and slot.dirty:
        if (slot->in_use && slot->dirty) {
            int64_t offset = (int64_t)slot->page_num * PAGE_SIZE;
            
            // os_write slot's data to disk at the right offset
            ssize_t bytes_written = os_write(pager->fd, slot->buffer.data, PAGE_SIZE, offset);
            if (bytes_written != PAGE_SIZE) {
                perror("Error flushing page to disk");
                // In production code, handle this error properly
            } else {
                // slot.dirty = false
                slot->dirty = false;
            }
        }
    }
}

// pager_close(pager): Flush pending modifications before resource cleanup
void pager_close(Pager pager) {
    if (pager == NULL) return;
    
    // pager_flush_all(pager) // call this before closing, so nothing's lost
    pager_flush_all(pager);
    
    // os_close, free, etc. (as before)
    os_close(pager->fd);
    free(pager);
}

// Internal helper matching your request to safely fetch an actual entry slot reference
// instead of just the isolated buffer.
static struct PageCacheEntry* _pager_get_cache_slot(Pager pager, uint32_t page_num) {
    if (pager == NULL) return NULL;

    // Check hit
    for (int i = 0; i < MAX_PAGE_CACHE; i++) {
        if (pager->cache[i].in_use && pager->cache[i].page_num == page_num) {
            return &pager->cache[i];
        }
    }

    // Cache miss — find empty slot
    int empty_slot_index = -1;
    for (int i = 0; i < MAX_PAGE_CACHE; i++) {
        if (!pager->cache[i].in_use) {
            empty_slot_index = i;
            break;
        }
    }

    if (empty_slot_index == -1) {
        fprintf(stderr, "cache full.\n");
        return NULL;
    }

    int64_t offset = (int64_t)page_num * PAGE_SIZE;
    struct PageCacheEntry *slot = &pager->cache[empty_slot_index];

    ssize_t bytes_read = os_read(pager->fd, slot->buffer.data, PAGE_SIZE, offset);
    if (bytes_read < 0) {
        perror("Unable to read file into cache");
        return NULL;
    }

    if (bytes_read < PAGE_SIZE) {
        for (ssize_t i = bytes_read; i < PAGE_SIZE; i++) {
            slot->buffer.data[i] = 0;
        }
    }

    slot->in_use = true;
    slot->dirty = false; // Freshly loaded from disk, not dirty yet
    slot->page_num = page_num;

    return slot;
}

// Public API unchanged, forwards payload pointer from our internal helper
PageBuffer* pager_get_page(Pager pager, uint32_t page_num) {
    struct PageCacheEntry *slot = _pager_get_cache_slot(pager, page_num);
    return slot ? &(slot->buffer) : NULL;
}

// pager_write_page(pager, page_num, data): Write directly to RAM cache
void pager_write_page(Pager pager, uint32_t page_num, const PageBuffer *in_buffer) {
    if (pager == NULL || in_buffer == NULL) return;

    // find or load page_num into a cache slot // reuse pager_get_page's logic here
    struct PageCacheEntry *slot = _pager_get_cache_slot(pager, page_num);
    if (slot == NULL) {
        fprintf(stderr, "Failed to allocate or locate cache slot for page %u\n", page_num);
        return;
    }

    // copy `data` into that slot's buffer
    memcpy(slot->buffer.data, in_buffer->data, PAGE_SIZE);

    // mark that slot dirty = true
    slot->dirty = true;

    // Expand internal page tracking bounds if tracking an appended slot
    if (page_num >= pager->page_count) {
        pager->page_count = page_num + 1;
    }
    
    // // no disk write here anymore — that happens later, on flush
}

uint32_t pager_page_count(Pager pager) {
    if (pager == NULL) return 0;
    return pager->page_count;
}

int main(void) {
    Pager p = pager_open("scratch.db");

    // Initialize an example data packet 
    PageBuffer data_packet;
    memset(data_packet.data, 0x41, PAGE_SIZE); // Fill with char 'A'

    // Write page to memory layer (Does not write to disk yet)
    pager_write_page(p, 0, &data_packet);
    
    // Explicit manual flush demo (also fully automated during pager_close)
    pager_flush_all(p);

    pager_close(p);
    return 0;
}

