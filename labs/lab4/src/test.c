#include "test.h"
#include <assert.h>

#define QUERY_SIZE 100
#define ALIGNMENT sizeof(struct block_header)

void* heap;
struct block_header* block;

static bool validate_allocated_memory(void* ptr) {
    uintptr_t start_address = (uintptr_t)heap;
    uintptr_t end_address = start_address + QUERY_SIZE;
    uintptr_t ptr_address = (uintptr_t)ptr;

    if (ptr_address >= start_address && ptr_address < end_address && (ptr_address - start_address) % ALIGNMENT == 0) {
        return true;
    } else {
        return false;
    }
}

static bool successful_memory_allocation() {
    printf("Test for successful memory allocation started...\n");
    heap = heap_init(QUERY_SIZE);
    if (!heap) {
        printf("Error: memory allocation failed.\n");
        return false;
    }

    if (validate_allocated_memory(heap)) {
        printf("Memory allocated successfully. \nTest test for successful memory allocation passed.\n");
        return true;
    } else {
        printf("Error: incorrect memory allocation.\n");
        return false;
    }
}

static bool freeing_one_block_from_several() {
    printf("Test for freeing one block from several allocated started...\n");

    void* heap = _malloc(QUERY_SIZE);
    debug_heap(stdout, block);
    assert(heap != NULL);

    printf("Freeing the first block...\n");
    _free(heap);
    printf("First block freed successfully.\n");
    debug_heap(stdout, block);

    if (validate_allocated_memory(heap)) {
        printf("Error: memory was not freed correctly.\n");
        return false;
    }

    printf("Test for freeing one block from several allocated passed.\n");
    return true;
}

static bool freeing_two_blocks_from_several() {
    printf("Test for freeing two blocks from several allocated started...\n");

    void* heap1 = _malloc(QUERY_SIZE);
    void* heap2 = _malloc(QUERY_SIZE);
    void* heap3 = _malloc(QUERY_SIZE);
    debug_heap(stdout, block);
    assert(heap1 != NULL && heap2 != NULL && heap3 != NULL);  

    printf("Freeing the first block...\n");
    _free(heap1); 
    printf("First block freed successfully.\n");
    _free(heap2);
    printf("Second block freed successfully.\n");
    debug_heap(stdout, block);

    if (validate_allocated_memory(heap1) && validate_allocated_memory(heap2)) {
        printf("Error: memory was not freed correctly.\n");
        return false;
    }

    _free(heap3);
    debug_heap(stdout, block);

    printf("Test for freeing two blocks from several allocated passed.\n");
    return true;
}

static bool new_memory_region_expands_old() {
    printf("Test for the expansion of the old region with the new one started...\n");
    size_t query_size = 10000;

    void* ptr1 = _malloc(query_size);
    debug_heap(stdout, block);
    assert(ptr1 != NULL); 

    size_t size1 = *((size_t*)ptr1 - 1);

    printf("Freeing the first block...\n");
    _free(ptr1);
    printf("First block freed successfully.\n");

    size_t new_query_size = 20000;
    void* ptr2 = _malloc(new_query_size);
    debug_heap(stdout, block);
    assert(ptr2 != NULL);

    size_t size2 = *((size_t*)ptr2 - 1);

    if (size2 >= size1) {
        printf("New memory region expanded the old one.\n");
    } else {
        printf("Error: new memory region did not expand the old one.\n");
        return false;
    }

    _free(ptr2);
    printf("Test for the expansion of the old region with the new one passed.\n");
    return true;
}

static bool new_region_stands_out_in_a_different_place() {
    printf("Test for allocating a new region elsewhere started...\n");
    size_t query_size = 10000;

    void* ptr1 = _malloc(query_size);
    debug_heap(stdout, block);
    assert(ptr1 != NULL);

    size_t new_query_size = 30000;
    void* ptr2 = _malloc(new_query_size);
    debug_heap(stdout, block);
    assert(ptr2 != NULL); 

    _free(ptr1);
    _free(ptr2);

    if ((uintptr_t)ptr2 < (uintptr_t)ptr1 || (uintptr_t)ptr2 > (uintptr_t)ptr1 + query_size) {
        printf("New memory region is allocated in a different place.\n");
    } else {
        printf("Error: new memory region extends the old one.\n");
        return false;
    }

    printf("Test for allocating a new region elsewhere passed.\n");
    return true;
}

bool run_tests() {
    if (successful_memory_allocation()) {
        block = (struct block_header*) heap;
        return freeing_one_block_from_several() && freeing_two_blocks_from_several() && new_memory_region_expands_old() && new_region_stands_out_in_a_different_place();
    }
    return false;
}
