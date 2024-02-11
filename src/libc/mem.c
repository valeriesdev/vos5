/**
 * @defgroup   MEM mem
 * @ingroup    LIBC
 *
 * @brief      This file implements a memory management system.
 * 
 * @par
 * The only functions which are safe to use elsewhere are
 * 		- memory_copy
 * 		- memory_set
 * 		- ta_alloc
 * 		- ta_free
 * 		- initialize_memory
 * 		- get_top
 * @note       initialize_memory needs to be called in the kernel initialization process 
 * 
 * The memory manager consists of a linked list of blocks. Each block has
 * - size: The size of the block
 * - next: The next block
 * - used: If the block is currently used
 * - valid: A magic number (0x0FBC)
 * - data: The data in the block
 * 
 * When allocating a new block, the following procedure is carried out:
 * 
 * If an unused block (one where used == 0) which has a size greater than the new size does not exist:  Append the new block to the end of the list. <br>
 * Otherwise, if the unused block has room for the new size and a char:  Split the unused block in two and allocate the new block to the first, and mark the block used (used = 1). <br>
 * Otherwise, Allocate the new block to the unused block, and mark the block used.<br>
 * 
 * If, when allocating, there are more than ta_free_BLOCK_THRESHOLD blocks marked unused, the refactor_ta_free function is called to attempt and clear up ta_free blocks.
 * 		
 * When ta_freeing a block, the block is simply marked unused, unless it is the last block. 
 * If it is the last block so, the block is deleted. Then, if the new last block is marked unused, recursively ta_free the last block.
 * 
 * When returning the ta_alloc'd block, or when ta_freeing a block, the address returned/used is the address of data, so that pointers can be assigned directly to the return value of ta_alloc. <br>
 * To ta_free a block, the address used is stepped back until the magic number is valid (0x0FBC). Then, the block is properly aligned and can be ta_freed.
 * 
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include <stddef.h>
#include "libc/mem.h"
#include "libc/string.h"
#include "libc/function.h"
#include "drivers/screen.h"
#include "cpu/task_manager.h"

typedef struct Block Block;
#define false 0
#define true 1

struct Block {
    void *addr;
    Block *next;
    size_t size;
};

typedef struct {
    Block *free;   // first ta_free block
    Block *used;   // first used block
    Block *fresh;  // first available blank block
    size_t top;    // top ta_free addr
} Heap;

static Heap *heap = NULL;
static const void *heap_limit = NULL;
static size_t heap_split_thresh;
static size_t heap_alignment;
static size_t heap_max_blocks;

/**
 * @brief      Copys memory from source to dest
 * @ingroup    MEM
 * 
 * @param      source  The source
 * @param      dest    The destination
 * @param[in]  nbytes  The number of bytes
 */
void memory_copy(uint8_t *source, uint8_t *dest, int nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);
    }
}

/**
 * @brief      Sets memory to a vlue
 * @ingroup    MEM
 * 
 * @param      dest  The destination to be set
 * @param[in]  val   The value to set the bytes to
 * @param[in]  len   The amount of bytes to set
 */
void memory_set(uint8_t *dest, uint8_t val, uint32_t len) {
    uint8_t *temp = (uint8_t *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

/**
 * If compaction is enabled, inserts block
 * into ta_free list, sorted by addr.
 * If disabled, add block has new head of
 * the ta_free list.
 */
static void insert_block(Block *block) {
#ifndef TA_DISABLE_COMPACT
    Block *ptr  = heap->free;
    Block *prev = NULL;
    while (ptr != NULL) {
        if ((size_t)block->addr <= (size_t)ptr->addr) {
            break;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    if (prev != NULL) {
        prev->next = block;
    } else {
        heap->free = block;
    }
    block->next = ptr;
#else
    block->next = heap->free;
    heap->free  = block;
#endif
}

#ifndef TA_DISABLE_COMPACT
static void release_blocks(Block *scan, Block *to) {
    Block *scan_next;
    while (scan != to) {
        scan_next   = scan->next;
        scan->next  = heap->fresh;
        heap->fresh = scan;
        scan->addr  = 0;
        scan->size  = 0;
        scan        = scan_next;
    }
}

static void compact() {
    Block *ptr = heap->free;
    Block *prev;
    Block *scan;
    while (ptr != NULL) {
        prev = ptr;
        scan = ptr->next;
        while (scan != NULL &&
               (size_t)prev->addr + prev->size == (size_t)scan->addr) {
            prev = scan;
            scan = scan->next;
        }
        if (prev != ptr) {
            size_t new_size =
                (size_t)prev->addr - (size_t)ptr->addr + prev->size;
            ptr->size   = new_size;
            Block *next = prev->next;
            // make merged blocks available
            release_blocks(ptr->next, prev->next);
            // relink
            ptr->next = next;
        }
        ptr = ptr->next;
    }
}
#endif

bool ta_init(const void *base, const void *limit, const size_t heap_blocks, const size_t split_thresh, const size_t alignment) {
    heap = (Heap *)base;
    heap_limit = limit;
    heap_split_thresh = split_thresh;
    heap_alignment = alignment;
    heap_max_blocks = heap_blocks;

    heap->free   = NULL;
    heap->used   = NULL;
    heap->fresh  = (Block *)(heap + 1);
    heap->top    = (size_t)(heap->fresh + heap_blocks);

    Block *block = heap->fresh;
    size_t i     = heap_max_blocks - 1;
    while (i--) {
        block->next = block + 1;
        block++;
    }
    block->next = NULL;
    return true;
}

bool ta_free(void *free) {
    Block *block = heap->used;
    Block *prev  = NULL;
    while (block != NULL) {
        if (free == block->addr) {
            if (prev) {
                prev->next = block->next;
            } else {
                heap->used = block->next;
            }
            insert_block(block);
#ifndef TA_DISABLE_COMPACT
            compact();
#endif
            return true;
        }
        prev  = block;
        block = block->next;
    }
    return false;
}

static Block *alloc_block(size_t num) {
    Block *ptr  = heap->free;
    Block *prev = NULL;
    size_t top  = heap->top;
    num         = (num + heap_alignment - 1) & -heap_alignment;
    while (ptr != NULL) {
        const int is_top = ((size_t)ptr->addr + ptr->size >= top) && ((size_t)ptr->addr + num <= (size_t)heap_limit);
        if (is_top || ptr->size >= num) {
            if (prev != NULL) {
                prev->next = ptr->next;
            } else {
                heap->free = ptr->next;
            }
            ptr->next  = heap->used;
            heap->used = ptr;
            if (is_top) {
                ptr->size = num;
                heap->top = (size_t)ptr->addr + num;
#ifndef TA_DISABLE_SPLIT
            } else if (heap->fresh != NULL) {
                size_t excess = ptr->size - num;
                if (excess >= heap_split_thresh) {
                    ptr->size    = num;
                    Block *split = heap->fresh;
                    heap->fresh  = split->next;
                    split->addr  = (void *)((size_t)ptr->addr + num);
                    split->size = excess;
                    insert_block(split);
#ifndef TA_DISABLE_COMPACT
                    compact();
#endif
                }
#endif
            }
            return ptr;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    // no matching ta_free blocks
    // see if any other blocks available
    size_t new_top = top + num;
    if (heap->fresh != NULL && new_top <= (size_t)heap_limit) {
        ptr         = heap->fresh;
        heap->fresh = ptr->next;
        ptr->addr   = (void *)top;
        ptr->next   = heap->used;
        ptr->size   = num;
        heap->used  = ptr;
        heap->top   = new_top;
        return ptr;
    }
    return NULL;
}

void *ta_alloc(size_t num) {
    Block *block = alloc_block(num);
    if (block != NULL) {
        return block->addr;
    }
    return NULL;
}

void *ta_alloc_align(size_t num, size_t alignment) {
    Block *block = alloc_block(num*2);
    if (block != NULL) {
        void * addr = (void*)(((size_t)block->addr + alignment) & -alignment);
        return addr;
    }

    return NULL;
}

static void memclear(void *ptr, size_t num) {
    size_t *ptrw = (size_t *)ptr;
    size_t numw  = (num & -sizeof(size_t)) / sizeof(size_t);
    while (numw--) {
        *ptrw++ = 0;
    }
    num &= (sizeof(size_t) - 1);
    uint8_t *ptrb = (uint8_t *)ptrw;
    while (num--) {
        *ptrb++ = 0;
    }
}

void *ta_calloc(size_t num, size_t size) {
    num *= size;
    Block *block = alloc_block(num);
    if (block != NULL) {
        memclear(block->addr, num);
        return block->addr;
    }
    return NULL;
}

static size_t count_blocks(Block *ptr) {
    size_t num = 0;
    while (ptr != NULL) {
        num++;
        ptr = ptr->next;
    }
    return num;
}

size_t ta_num_free() {
    return count_blocks(heap->free);
}

size_t ta_num_used() {
    return count_blocks(heap->used);
}

size_t ta_num_fresh() {
    return count_blocks(heap->fresh);
}

bool ta_check() {
    return heap_max_blocks == ta_num_free() + ta_num_used() + ta_num_fresh();
}