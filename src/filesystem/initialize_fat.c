#include "filesystem/filesystem.h"
#include "libc/mem.h"
#include "drivers/ata.h"
#include "drivers/screen.h"


static void initialize_empty_fat_to_disk();
uint16_t* rescue_program_headers() ;
#define initial_node_name "INIT_NODE"
/**
 * @brief      Loads a FAT table from disk.
 * @ingroup    FILESYSTEM
 * @note       If no FAT table is present, it will attempt to initialize one.
 */
void load_fat_from_disk() {
   fat_head = malloc(sizeof(uint8_t)*6*512); // Free handled
   read_sectors_ATA_PIO((uint32_t)fat_head, FAT_LBA, 6);

   if(fat_head->magic != 0xFFFFFFFF) {
      kprintn("Loading FAT from disk failed, invalid allocation table. Creating new FAT");
      initialize_empty_fat_to_disk();
      fat_head = free(fat_head);
      load_fat_from_disk();
   } else {
      kprintn("Successfully loaded FAT");
      num_registered_files = 1;
      first_free_sector = fat_head->lba+1;
   }
}

/**
 * @brief      Initializes the empty fat table to disk.
 * @ingroup    FILESYSTEM
 * @todo       Fix fat_head and t_storage free behavior
 * @todo       Make static
 * @todo       Removing that for loop causes the hard disk driver to hang
 */
static void initialize_empty_fat_to_disk() {
   fat_head = (struct file*)malloc(512*3);
   memory_copy((uint8_t*)&initial_node_name, (uint8_t*)&(fat_head->name), 9);
   fat_head->lba = FIRST_DATA_LBA;
   fat_head->length = 1;
   fat_head->magic = 0xFFFFFFFF;

   uint16_t* rescued_programs_lba = rescue_program_headers();
   int i = 0;
   for(i = 1; i < rescued_programs_lba[0]; i++) {
      void* program = malloc(512);
      read_sectors_ATA_PIO((uint32_t)program, rescued_programs_lba[i], 1);
      struct file* node = fat_head+i;
      memory_copy((uint8_t*)&(((struct program_identifier*)program)->name), (uint8_t*)&(node->name), 32);
      node->lba = rescued_programs_lba[i];
      node->length = 1;
      node->magic = 0xFFFFFFFF;
      free(program);
   }

   uint16_t* t_storage = malloc(512*3);
   memory_copy((uint8_t*)fat_head, (uint8_t*)t_storage,sizeof(fat_head));
   write_sectors_ATA_PIO(FAT_LBA, 2, (uint16_t*)fat_head);
}

uint16_t* rescue_program_headers() {
	uint16_t* program_lbas = malloc(sizeof(uint8_t)*256);
	int program_found_counter = 1;
    int i = 0;
    for(i = 0; i < 256; i++) {
        void* program = malloc(512);
        read_sectors_ATA_PIO((uint32_t)program, i*8, 1);
        if(((struct program_identifier*) program)->magic[0] == 0xFFFFFFFF &&
           ((struct program_identifier*) program)->magic[1] == 0xFFFFFFFF &&
           ((struct program_identifier*) program)->magic[2] == 0xFFFFFFFF &&
           ((struct program_identifier*) program)->magic[3] == 0xFFFFFFFF) {
            program_lbas[program_found_counter] = i*8;
            program_found_counter++;
        }
        free(program);
    }
    program_lbas[0] = program_found_counter;
    return program_lbas;
}