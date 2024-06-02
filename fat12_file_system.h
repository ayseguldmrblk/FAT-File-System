#ifndef FAT12_FILE_SYSTEM_H
#define FAT12_FILE_SYSTEM_H

#include <stdio.h>
#include <stdint.h>

typedef struct file_attributes
{
    uint8_t read_permissiom : 1;
    uint8_t write_permission : 1;
    uint8_t is_directory : 1;
    uint8_t is_protected : 1;
} file_attributes; // 4 bits


typedef struct directory_entry
{
    int64_t filename;
    char extension[3];
    file_attributes attributes;
    uint8_t reserved[10];

    uint16_t first_block_number;
    uint32_t file_size;
} Directory_Entry; // 32 bytes

typedef struct directory_entry_table
{
    uint32_t table_size;
    
    uint32_t table_capacity;
    Directory_Entry* table;
} Directory_Entry_Table; // 12 bytes





#endif // FAT12_FILE_SYSTEM_H