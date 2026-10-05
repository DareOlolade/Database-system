#include <stdio.h>

int main(void) {
    Pager p = pager_open("scratch.db");

    PageBuffer *first  = pager_get_page(p, 0);
    printf("first call ptr: %p\n", (void*)first);

    PageBuffer *second = pager_get_page(p, 0);
    printf("second call ptr: %p\n", (void*)second);
    // if these two addresses match, the cache hit path worked

    pager_close(p);
    return 0;
}
