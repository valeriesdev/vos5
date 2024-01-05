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
 * 		- malloc
 * 		- free
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
 * If, when allocating, there are more than FREE_BLOCK_THRESHOLD blocks marked unused, the refactor_free function is called to attempt and clear up free blocks.
 * 		
 * When freeing a block, the block is simply marked unused, unless it is the last block. 
 * If it is the last block so, the block is deleted. Then, if the new last block is marked unused, recursively free the last block.
 * 
 * When returning the malloc'd block, or when freeing a block, the address returned/used is the address of data, so that pointers can be assigned directly to the return value of malloc. <br>
 * To free a block, the address used is stepped back until the magic number is valid (0x0FBC). Then, the block is properly aligned and can be freed.
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

#define TRUE 1
#define FALSE 0
#define FREE_BLOCK_THRESHOLD 10
#define ALIGN(n) (sizeof(struct block) + n)
#define ALIGN_P(n) (sizeof(size_t) + sizeof(struct block*) + sizeof(uint32_t))

/**
 * @brief      A block of memory.
 * @ingroup    MEM
 */
struct block {
	size_t size;
	struct block *next;
	uint8_t used;
	uint32_t valid;
	uint8_t data;
};

// Private function definitions
static void *find_free(size_t n);
static void *alloc(size_t size);
static void print_node(struct block *current);
static void traverse();

struct block *head = (struct block*)0x100000;
struct block *top  = (struct block*)0x100000;
uint32_t num_free_blocks;

/**
 * Memory Utility Functions
 */

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
 * Memory Segmentation Functions
 */

/**
 * @brief      Initializes the memory.
 * @ingroup    MEM
 */
void initialize_memory() {
	head = (struct block*)0x100000;
	top  = (struct block*)0x100000;
	(*top).size = ALIGN(32);
	(*top).next = NULL;
	(*top).valid = 0x0FBC;
	(*top).used = TRUE;

	num_free_blocks = 0;

	if((int)head == 0x100000) kprintn("Memory initialized properly at 0x100000");
	else kprintn("MEMORY FAILED TO INITIALIZE!");

	return;
}

/**
 * @brief      Allocates a block of memory
 * @ingroup    MEM
 *
 * @param[in]  size  The size of the block
 *
 * @return     The pointer to the start of the memory within that block
 * 
 * If an unused block which has a size greater than the new size does not exist:
 * 		Allocate the new block to the end of the list
 * Otherwise, if the unused block has room for the new size and a char:
 * 		Split the unused block in two and allocate the new block to the first
 * Otherwise, 
 * 		Allocate the new block to the unused block
 */
static void *alloc(size_t size) {
	struct block *newBlock = (struct block *)find_free(size);
	if((*newBlock).next == NULL) { // Procedure for allocating a block at the end of the db
		char* tptr = (char*)newBlock;
		newBlock = (struct block*)(tptr + (*newBlock).size);
		(*newBlock).size = ALIGN(size);
		(*newBlock).next = NULL;
		(*newBlock).valid = 0x0FBC;
		(*newBlock).used = TRUE;

		(*top).next = newBlock;
		top = (*top).next;
	
		return &(*newBlock).data;
	} else { // Procedure for allocating a block at the middle of the db
		if((*newBlock).size - ALIGN(size) > ALIGN(sizeof(char))) { // Split block
			struct block *insertBlock = (struct block*)((uint8_t*)newBlock + ALIGN(size));
			(*insertBlock).size = (*newBlock).size - ALIGN(size);
			(*insertBlock).next = (*newBlock).next;
			(*insertBlock).valid = 0x0FBC;
			(*insertBlock).used = FALSE;
			
			(*newBlock).size = ALIGN(size);
			(*newBlock).next = insertBlock;
			(*newBlock).used = TRUE;
			return &(*newBlock).data;
		} else { // Fit into oversize block
			(*newBlock).used = TRUE;
			return &(*newBlock).data;
		}
	}

	return NULL;
}

static void *find_free(size_t n) {
	struct block *current = head;
	for(; (*current).next != NULL; current = (*current).next) {
		if((*current).used == FALSE && ALIGN(n) < (*current).size) return current;
	}
	//if((*current).used == FALSE && ALIGN(n) < (*current).size) return current;
	return current;
}

/**
 * @brief      Frees a block of memory
 * @ingroup    MEM
 *
 * @param      address  The address of the value to be freed
 * 
 * @return     NULL on success.
 */
void* free(void *address) {
	num_free_blocks++;
	struct block *changeBlock = (struct block *)address;
	char *tBlock = (char*)address;

	for(; (int) tBlock >= (int) head; --tBlock) {
		if((*(struct block *)tBlock).valid == 0x0FBC) {
			changeBlock = (struct block *)tBlock;
			break;
		}
	}

	(*changeBlock).used = FALSE;
	memory_set(&(*changeBlock).data, 0, (*changeBlock).size - ALIGN(0));
	if(changeBlock == top) {
		num_free_blocks--;
		memory_set((uint8_t*)changeBlock, 0, &(*changeBlock).data - (uint8_t*)changeBlock);
		struct block *current = head;
		while(current->next != changeBlock) {
			current = current->next;
		}
		current->next = NULL;
		top = current;
	}

	if(top->used == 0) free(top);

	return NULL;
}

/**
 * @brief      Attempts to reduce the number of unused blocks
 * @ingroup    MEM
 * @todo       Verify functionality
 */
void refactor_free() {
	struct block *current = head;
	while(current != NULL) {
		if(!(current->used == TRUE || current->next->used == TRUE)) {
			struct block *adjacent_block = current->next;
			current->next = adjacent_block->next;

			current->size += adjacent_block->size;
			memory_set(&(*current).data, 0, (*current).size - ALIGN(0));
		} else {
			current = current->next;
		}
	}
}

/**
 * @brief      Allocate a block of memory
 * @ingroup    MEM
 *
 * @param[in]  size  The size
 *
 * @return     The address of the block
 */
void *malloc(uint32_t size) {
	if(num_free_blocks > FREE_BLOCK_THRESHOLD) refactor_free();

	void* t = alloc(size);
	return t;
}

/**
 * @bried     Allocate ablock of memory, with provision to allign that to a specific address.
 * @ingroup   MEM
 * 
 * @todo       FINISH!
 * 
 * @param[in] size  The size
 * @param[in] align The address to align to
 * 
 * @return    The address of the block
 */
void *malloc_align(uint32_t size, uint32_t align) {
	struct block * new_block = (void*)(((long)(top + (*top).size + align) & 0xFFFFF000) - 0x10);// - ALIGN_P(size);
	(*new_block).size = ALIGN(size);
	(*new_block).next = NULL;
	(*new_block).valid = 0x0FBC;
	(*new_block).used = TRUE;

	(*top).next = new_block;
	top = (*top).next;
	
	return &(*new_block).data;
}

// Debug functions

void* get_top() {
	return top;
}

static void print_node(struct block *current) {
	//UNUSED(current);
	char* string = hex_to_ascii((int) current); 
	kprint("ADDR: ");
	kprint(string);
	free(string);

	kprint(", SIZE:");
	string = hex_to_ascii((int)(*current).size); 
	kprint(string);
	free(string);

	kprint(", USED:");
	string = int_to_ascii((int)(*current).used);
	kprint(string);
	free(string);

	kprint(", NEXT:");
	string = hex_to_ascii((int)(*current).next);
	kprint(string);
	free(string);
	kprint("\n");
}

static void traverse() {
	struct block *current = head;
	for(; (*current).next != NULL; current = (*current).next) {
		print_node(current);
	}
	print_node(current);
	kprint("\n");
}

void debug_traverse() {
	traverse();
}