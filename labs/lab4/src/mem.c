#include <stdarg.h>
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

void debug_block(struct block_header* b, const char* fmt, ... );
void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

static bool            block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }
static size_t          pages_count   ( size_t mem )                      { return mem / getpagesize() + ((mem % getpagesize()) > 0); }
static size_t          round_pages   ( size_t mem )                      { return getpagesize() * pages_count( mem ) ; }

static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) {
  *((struct block_header*)addr) = (struct block_header) {
    .next = next,
    .capacity = capacity_from_size(block_sz),
    .is_free = true
  };
}

static size_t region_actual_size( size_t query ) { return size_max( round_pages( query ), REGION_MIN_SIZE ); }

extern inline bool region_is_invalid( const struct region* r );



static void* map_pages(void const* addr, size_t length, int additional_flags) {
  return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , 0, 0 );
}

/*  аллоцировать регион памяти и инициализировать его блоком */
static struct region alloc_region  ( void const * addr, size_t query ) {
    size_t region_size = region_actual_size(query);
    void* region_addr = map_pages(addr, region_size, 0);

    if (region_addr == MAP_FAILED) {
        return REGION_INVALID;
    }

    block_init(region_addr, (block_size){region_size}, NULL);

    struct region result = {region_addr, region_size, false};
    return result;
}

static void* block_after( struct block_header const* block )         ;

void* heap_init( size_t initial ) {
  const struct region region = alloc_region( HEAP_START, initial );
  if ( region_is_invalid(&region) ) return NULL;

  return region.addr;
}

#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */

static bool block_splittable( struct block_header* restrict block, size_t query) {
  return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

static bool split_if_too_big( struct block_header* block, size_t query ) {
    if (block_splittable(block, query)) {
        size_t remaining_capacity = block->capacity.bytes - query - offsetof(struct block_header, contents);

        if (remaining_capacity >= BLOCK_MIN_CAPACITY) {
            struct block_header* new_block = (struct block_header*)((char*)block + query + offsetof(struct block_header, contents));
            new_block->next = block->next;
            new_block->capacity.bytes = remaining_capacity;
            new_block->is_free = true;
            block->next = new_block;
            block->capacity.bytes = query;
            return true;
        }
    }
    return false;
}


/*  --- Слияние соседних свободных блоков --- */

static void* block_after( struct block_header const* block )              {
  return  (void*) (block->contents + block->capacity.bytes);
}
static bool blocks_continuous (
                               struct block_header const* fst,
                               struct block_header const* snd ) {
  return (void*)snd == block_after(fst);
}

static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) {
  return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

static bool try_merge_with_next( struct block_header* block ) {
    if (block == NULL || block->next == NULL) {
        return false;
    }

    struct block_header* next_block = block->next;

    if (next_block->is_free) {
        block->capacity.bytes += next_block->capacity.bytes + offsetof(struct block_header, contents);
        block->next = next_block->next;
        return true;
    }

    return false;
}


/*  --- ... ecли размера кучи хватает --- */

struct block_search_result {
  enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
  struct block_header* block;
};


static struct block_search_result find_good_or_last  ( struct block_header* restrict block, size_t sz )    {
    struct block_header* current = block;
    struct block_header* last = NULL;

    while (current != NULL) {
        if (block_is_big_enough(sz, current) && current->is_free) {
            return (struct block_search_result){BSR_FOUND_GOOD_BLOCK, current};
        }

        last = current;
        current = current->next;
    }

    if (last != NULL) {
        return (struct block_search_result){BSR_REACHED_END_NOT_FOUND, last};
    }

    return (struct block_search_result){BSR_CORRUPTED, NULL};
}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
static struct block_search_result try_memalloc_existing ( size_t query, struct block_header* block ) {
    struct block_header* current = block;
    struct block_header* last = NULL;

    while (current != NULL) {
        if (block_is_big_enough(query, current) && current->is_free) {
            if (split_if_too_big(current, query)) {
                return (struct block_search_result){BSR_FOUND_GOOD_BLOCK, current};
            }
            return (struct block_search_result){BSR_FOUND_GOOD_BLOCK, current};
        }

        last = current;
        current = current->next;
    }

    if (last != NULL) {
        return (struct block_search_result){BSR_REACHED_END_NOT_FOUND, last};
    }

    return (struct block_search_result){BSR_CORRUPTED, NULL};
}



static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) {
    const size_t region_size = region_actual_size(query);
    void* region_addr = map_pages(block_after(last), region_size, 0);

    if (region_addr == MAP_FAILED) {
        return NULL;
    }

    block_init(region_addr, (block_size){region_size}, NULL);

    last->next = region_addr;

    if (try_merge_with_next(last)) {
        return last;
    } else {
        return region_addr;
    }
}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
static struct block_header* memalloc(size_t query, struct block_header* heap_start) {
    query = size_max(query, BLOCK_MIN_CAPACITY);
    struct block_search_result block_found = try_memalloc_existing(query, heap_start);

    if (block_found.type == BSR_CORRUPTED || block_found.type == BSR_REACHED_END_NOT_FOUND) {
        struct block_header* new_block = NULL;

        if (block_found.type == BSR_CORRUPTED) {
            new_block = heap_init(query);
        } else if (block_found.type == BSR_REACHED_END_NOT_FOUND) {
            new_block = grow_heap(block_found.block, query);
        }

        if (new_block != NULL) {
            split_if_too_big(new_block, query);
            new_block->is_free = false;
            return new_block;
        } else {
            return NULL;
        }
    } else if (block_found.type == BSR_FOUND_GOOD_BLOCK) {
        if (split_if_too_big(block_found.block, query)) {
            block_found.block->is_free = false;
            return block_found.block;
        } else {
            block_found.block->is_free = false;
            return block_found.block;
        }
    }
    return NULL;
}


void* _malloc( size_t query ) {
  struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
  if (addr) return addr->contents;
  else return NULL;
}

static struct block_header* block_get_header(void* contents) {
  return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void _free( void* mem ) {
  if (!mem) return ;
  struct block_header* header = block_get_header( mem );
  header->is_free = true;
  while(try_merge_with_next(header));
}


