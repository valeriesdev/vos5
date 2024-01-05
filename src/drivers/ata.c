/**
 * @defgroup   DRIVERS drivers
 */
/**
 * @defgroup   ATA ata
 * @ingroup    DRIVERS
 *
 * @brief      This file implements a driver to read and write from the hard disk using ATA.
 * 
 * @par
 * All code in the ATA driver should be effectively private. All communication to and from the disk should be handled by the filesystem driver rather than the ATA driver.
 * 
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include "cpu/ports.h"
#include "drivers/ata.h"

/*
BSY: a 1 means that the controller is busy executing a command. No register should be accessed (except the digital output register) while this bit is set.
RDY: a 1 means that the controller is ready to accept a command, and the drive is spinning at correct speed..
WFT: a 1 means that the controller detected a write fault.
SKC: a 1 means that the read/write head is in position (seek completed).
DRQ: a 1 means that the controller is expecting data (for a write) or is sending data (for a read). Don't access the data register while this bit is 0.
COR: a 1 indicates that the controller had to correct data, by using the ECC bytes (error correction code: extra bytes at the end of the sector that allows to verify its integrity and, sometimes, to correct errors).
IDX: a 1 indicates the the controller retected the index mark (which is not a hole on hard-drives).
ERR: a 1 indicates that an error occured. An error code has been placed in the error register.
*/

#define STATUS_BSY 0x80 // A 1 on this port indicates that the controller is busy executing a command. No register should be accessed (except the digital output register) while this bit is set.
#define STATUS_RDY 0x40 // A 1 on this port indicates that the controller is ready to accept a command, and the drive is spinning at correct speed..
#define STATUS_DRQ 0x08 // A 1 on this port indicates that the controller is expecting data (for a write) or is sending data (for a read). Don't access the data register while this bit is 0.
#define STATUS_DF 0x20
#define STATUS_ERR 0x01 // A 1 on this port indicates that an error occured. An error code has been placed in the error register.

#define ATA_DATA           0x1F0 // ATA Data Register
#define ATA_ERROR          0x1F1 // ATA Error Register
#define ATA_SECTOR_COUNT   0x1F2 // ATA Sector Count Register
#define ATA_LBA_LOW        0x1F3 // ATA LBA Register Low Byte
#define ATA_LBA_MID        0x1F4 // ATA LBA Register Mid Byte
#define ATA_LBA_HIGH       0x1F5 // ATA LBA Register High Byte
#define ATA_SELECT_DRIVE   0x1F6 // ATA Select Drive Register
#define ATA_STATUS_COMMAND 0x1F7 // ATA Status Command Register

#define ATA_READ_SECTORS   0x20 // ATA Read Sectors Command
#define ATA_WRITE_SECTORS  0x30 // ATA Write Sectors Command

// Private function definitions
static void ATA_wait_BSY();
static void ATA_wait_RDY();

/**
 * @brief      Reads sectors from the hard disk through ATA PIO method
 * @ingroup    ATA
 *
 * @param[in]  target_address  The address to read the data into
 * @param[in]  LBA             The logical block address to read from
 * @param[in]  sector_count    How many sectors to read from
 * 
 * @code
 * uint8_t  size_of_data = 256;
 * uint8_t* data_to_be_read = malloc(sizeof(uint8_t) * 512 * (uint8_t)(size_of_data/512)+1);
 * read_sectors_ATA_PIO((uint32_t)data_to_be_read, 17, (uint8_t)(size_of_data/512)+1);
 * @endcode
 */
void read_sectors_ATA_PIO(uint32_t target_address, uint32_t LBA, uint8_t sector_count) {
    ATA_wait_BSY();
    port_byte_out(ATA_SELECT_DRIVE,   0xE0 | ((LBA >>24) & 0xF));
    port_byte_out(ATA_SECTOR_COUNT,   sector_count);
    port_byte_out(ATA_LBA_LOW,        (uint8_t) LBA);
    port_byte_out(ATA_LBA_MID,        (uint8_t)(LBA >> 8));
    port_byte_out(ATA_LBA_HIGH,       (uint8_t)(LBA >> 16)); 
    port_byte_out(ATA_STATUS_COMMAND, ATA_READ_SECTORS);

    uint16_t *target = (uint16_t*) target_address;

    int j = 0;
    for (j = 0;j<sector_count;j++) {
        ATA_wait_BSY();
        ATA_wait_RDY();
        int i = 0;
        for(i = 0;i < 256; i++) {
            target[i] = port_word_in(ATA_DATA);
        }
        target += 256;
    }
}

/**
 * @brief      Writes sectors to the hard disk through ATA PIO method
 * @ingroup    ATA
 *
 * @param[in]  LBA           The logical block address to write to
 * @param[in]  sector_count  How many sectors to write
 * @param      bytes         Array of the bytes to be written
 * 
 */
void write_sectors_ATA_PIO(uint32_t LBA, uint8_t sector_count, uint16_t* bytes) {
    ATA_wait_BSY();
    port_byte_out(ATA_SELECT_DRIVE,   (0xE0 | ((LBA >>24) & 0xF)));
    port_byte_out(ATA_SECTOR_COUNT,   sector_count);
    port_byte_out(ATA_LBA_LOW,        (uint8_t) LBA);
    port_byte_out(ATA_LBA_MID,        (uint8_t)(LBA >> 8));
    port_byte_out(ATA_LBA_HIGH,       (uint8_t)(LBA >> 16)); 
    port_byte_out(ATA_STATUS_COMMAND, ATA_WRITE_SECTORS);

    int j = 0;
    for (j = 0; j < sector_count; j++) {
        ATA_wait_BSY();
        ATA_wait_RDY();
        int i = 0;
        for(i = 0; i < 256; i++) {
            port_word_out(ATA_DATA, bytes[i]); 
        }
    }
}
/**
 * @brief Loops until ATA Busy port is not true
 * @ingroup    ATA
 **/
static void ATA_wait_BSY() {
    while(port_byte_in(ATA_STATUS_COMMAND)&STATUS_BSY);
}

/**
 * @brief Loops until ATA Ready port is true
 * @ingroup    ATA
 **/
static void ATA_wait_RDY() {
    while(!(port_byte_in(ATA_STATUS_COMMAND)&STATUS_RDY));
}