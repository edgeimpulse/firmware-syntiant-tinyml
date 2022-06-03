#ifndef EI_SAMPLE_STORAGE_H
#define EI_SAMPLE_STORAGE_H

/* Include ----------------------------------------------------------------- */
#include <stdint.h>

/* SD card size defines ---------------------------------------------------- */
#define SD_BLOCK_SIZE       512
#define SD_N_BLOCKS         1000
#define SD_FILE_SIZE        (SD_BLOCK_SIZE * SD_N_BLOCKS)

/* Prototypes -------------------------------------------------------------- */
void ei_create_bin(void);
void ei_write_bin(void);
int ei_write_data_to_bin(uint8_t *data, uint32_t address, uint32_t length);
int ei_write_last_data_to_bin(uint32_t address);
uint32_t ei_read_sample_buffer(uint8_t *sample_buffer, uint32_t address_offset, uint32_t n_read_bytes);

#endif