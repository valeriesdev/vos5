#include "filesystem/filesystem.h"
#include "libc/mem.h"
#include "drivers/ata.h"
#include "drivers/screen.h"


static void initialize_empty_fat_to_disk();
struct program_identifier* rescue_program_headers() ;
#define initial_node_name "INIT_NODE"

void init_fat_info() {
   num_registered_files = 1;
   first_free_sector = FIRST_DATA_LBA+1;
}

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
      num_registered_files = 1;
      first_free_sector = FIRST_DATA_LBA+1;
      kprintn("Successfully loaded FAT");
      struct file* files = get_files()+1;
      while(files->magic == 0xFFFFFFFF) {
         //kprintn(files->name);
         files++;
         num_registered_files++;
         if (files->lba > first_free_sector) first_free_sector = files->lba+1;
      }
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

   struct program_identifier* rescued_programs_lba = rescue_program_headers();
   int i = 0;
   for(i = 1; i < rescued_programs_lba[0].magic[0]; i++) {
      void* program = malloc(rescued_programs_lba[i].length*512);
      read_sectors_ATA_PIO((uint32_t)program, rescued_programs_lba[i].lba, rescued_programs_lba[i].length);
      struct file* node = fat_head+i;
      memory_copy((uint8_t*)&(((struct program_identifier*)program)->name), (uint8_t*)&(node->name), 32);
      node->lba = rescued_programs_lba[i].lba;
      node->length = rescued_programs_lba[i].length;
      node->magic = 0xFFFFFFFF;
      free(program);
      num_registered_files++;
      first_free_sector += rescued_programs_lba[i].length;
   }

   uint16_t* t_storage = malloc(512*3);
   memory_copy((uint8_t*)fat_head, (uint8_t*)t_storage,sizeof(fat_head));
   write_sectors_ATA_PIO(FAT_LBA, 2, (uint16_t*)fat_head);
}

struct program_identifier* rescue_program_headers() {
	struct program_identifier* identifiers = malloc(sizeof(struct program_identifier)*16);
	int program_found_counter = 1;
    int i = 0;
    for(i = 0; i < 256; i++) {
        void* program = malloc(512);
        read_sectors_ATA_PIO((uint32_t)program, i*8, 1);
        if(((struct program_identifier*) program)->magic[0] == 0xFFFFFFFF &&
           ((struct program_identifier*) program)->magic[1] == 0xFFFFFFFF &&
           ((struct program_identifier*) program)->magic[2] == 0xFFFFFFFF &&
           ((struct program_identifier*) program)->magic[3] == 0xFFFFFFFF) {
            //identifiers[program_found_counter] = malloc(sizeof(struct program_identifier));
            identifiers[program_found_counter].lba = i*8;
            identifiers[program_found_counter].length = ((struct program_identifier*) program)->length; 
            program_found_counter++;
        }
        free(program);
    }
    identifiers[0].magic[0] = program_found_counter;
    return identifiers;
}