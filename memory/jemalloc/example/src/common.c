#include "common.h"

void *ex_alloc(int size)
{
    void *ptr;

    ptr = (void *)malloc(size);
    if (!(ptr)) {
        printf("Malloc fail\n");
        return NULL;
    }

    memset(ptr, 0, size);
    return ptr;
}
