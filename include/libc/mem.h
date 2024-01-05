#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void memory_copy(uint8_t *source, uint8_t *dest, int nbytes);
void memory_set(uint8_t *dest, uint8_t val, uint32_t len);
void memory_set_32(uint32_t *dest, uint32_t val, uint32_t len);
void *malloc(uint32_t size);
void initialize_memory();
void* free(void* address);
void* get_top();

void *malloc_align(uint32_t size, uint32_t align);

//debug
void debug_traverse();

#endif