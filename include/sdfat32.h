/********************************************************************
 * sdfat32.h
 *
 *  Header file for SPI SD card reader that implements a minimal
 *  read-only driver for FAT32 file system, and an interface to
 *  read CAS and ROM files into emulator memory.
 *
 *  This is a minimal implementation, FAT32, read only, v1.0 SD card
 *  compliant driver. The goal is functionality not performance.
 *
 *  May 9, 2021
 *
 *******************************************************************/

#ifndef __SDFAT32_H__
#define __SDFAT32_H__

#include    <stdint.h>

#define     FAT32_LONG_FILE_NAME    256
#define     FAT32_DOS_FILE_NAME     13

typedef struct
    {
        int         is_directory;
        char        lfn[FAT32_LONG_FILE_NAME];
        char        sfn[FAT32_DOS_FILE_NAME];
        uint32_t    cluster_chain_head;
        int         file_size;
    } dir_entry_t;

typedef enum
    {
        FAT_OK,
        FAT_SD_FAIL,
        FAT_BAD_SECTOR_SIG,
        FAT_BAD_PARTITION_TYPE,
        FAT_BAD_SECTOR_PER_CLUS,
    } fat_error_t;

/********************************************************************
 *  SD FAT32 reader API
 */
fat_error_t fat32_init(void);
int         fat32_parse_dir(uint32_t start_cluster, dir_entry_t *directory_list, int dir_list_length);

int         fat32_fopen(dir_entry_t *directory_entry);
void        fat32_fclose(void);
int         fat32_fseek(int byte_position);
int         fat32_fread(uint8_t *buffer, int buffer_length);
int         fat32_fstat(void);
int         fat32_ftell(void);

#endif  /* __SDFAT32_H__ */
