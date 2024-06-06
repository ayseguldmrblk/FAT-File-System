#ifndef FAT12_FILE_SYSTEM_H
#define FAT12_FILE_SYSTEM_H

#include <stdio.h>
#include <stdint.h>

#define TOTAL_CLUSTERS 4096
#define FAT12_SIZE (TOTAL_CLUSTERS * 3 / 2)
#define DIRECTORY_ENTRIES_PER_CLUSTER (1024 / sizeof(Directory_Entry))

uint8_t fat12_table[FAT12_SIZE];
Directory_Entry entries[DIRECTORY_ENTRIES_PER_CLUSTER]; // 16 entries per 1 KB cluster

typedef struct file_attributes 
{
    uint8_t read_permission : 1;
    uint8_t write_permission : 1;
    uint8_t is_directory : 1;
    uint8_t is_protected : 1;
    uint8_t filename_exceeds : 1; // Indicates if the filename exceeds 8 characters
    uint8_t reserved : 3;
} file_attributes; // 8 bits

typedef struct file_time 
{
    uint8_t hours : 5;   // 0-23
    uint8_t minutes : 6; // 0-59
    uint8_t seconds : 5; // 0-29 (2-second increments)
} file_time; // 16 bits

typedef struct file_date 
{
    uint8_t day : 5;   // 1-31
    uint8_t month : 4; // 1-12
    uint16_t year : 7; // 1980-2107 (year-1980)
} file_date; // 16 bits

typedef struct directory_entry 
{
    uint64_t filename; // 64 bits for the filename (8 characters with 8 bits each)
    char extension[3]; // File extension
    file_attributes attributes; // File attributes
    char reserved[20]; // Extended filename (if it exceeds 8 characters)
    file_time last_modification_time; // Time of last modification
    file_date last_modification_date; // Date of last modification
    uint16_t first_block_number; // The first block number of the file
    uint32_t file_size; // Size of the file in bytes
    char password[16]; // Password for the file
} Directory_Entry; // 64 bytes

// Function to set a value in the FAT12 table
void set_fat12_entry(uint16_t cluster, uint16_t value) 
{
    uint32_t index = cluster * 3 / 2;
    if (cluster % 2 == 0) 
    {
        fat12_table[index] = value & 0xFF;
        fat12_table[index + 1] = (fat12_table[index + 1] & 0xF0) | ((value >> 8) & 0x0F);
    } 
    else 
    {
        fat12_table[index] = (fat12_table[index] & 0x0F) | ((value << 4) & 0xF0);
        fat12_table[index + 1] = (value >> 4) & 0xFF;
    }
}

// Function to get a value from the FAT12 table
uint16_t get_fat12_entry(uint16_t cluster) 
{
    uint32_t index = cluster * 3 / 2;
    if (cluster % 2 == 0) 
    {
        return fat12_table[index] | ((fat12_table[index + 1] & 0x0F) << 8);
    } 
    else 
    {
        return ((fat12_table[index] & 0xF0) >> 4) | (fat12_table[index + 1] << 4);
    }
}

// Function to get the status of a cluster
const char* get_cluster_status(uint16_t cluster) 
{
    uint16_t entry = get_fat12_entry(cluster);
    if (entry == 0x000) 
    {
        return "Free";
    } 
    else if (entry >= 0xFF0 && entry <= 0xFF6) 
    {
        return "Reserved";
    } 
    else if (entry == 0xFF7) 
    {
        return "Bad";
    } 
    else if (entry >= 0xFF8 && entry <= 0xFFF) 
    {
        return "EOF";
    } 
    else 
    {
        return "Used";
    }
}

#endif // FAT12_FILE_SYSTEM_H