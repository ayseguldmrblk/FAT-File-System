#ifndef FAT12_FILE_SYSTEM_H
#define FAT12_FILE_SYSTEM_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>

#define TOTAL_CLUSTERS 4096
#define FAT12_SIZE (TOTAL_CLUSTERS * 3 / 2)
#define DIRECTORY_ENTRIES_PER_CLUSTER (1024 / sizeof(Directory_Entry))

using namespace std;

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
    char reserved[18]; // Extended filename (if it exceeds 8 characters)
    file_time last_modification_time; // Time of last modification
    file_date last_modification_date; // Date of last modification
    file_time creation_time; // Time of creation
    file_date creation_date; // Date of creation
    uint16_t first_block_number; // The first block number of the file
    uint32_t file_size; // Size of the file in bytes
    char password[16]; // Password for the file
} Directory_Entry; // 64 bytes

extern uint8_t fat12_table[FAT12_SIZE];
extern Directory_Entry entries[DIRECTORY_ENTRIES_PER_CLUSTER]; // 16 entries per 1 KB cluster

// File system functions
void set_file_time(file_time *ft, int hours, int minutes, int seconds);
void set_file_date(file_date *fd, int year, int month, int day);
void initialize_directory_entry(Directory_Entry* entry, const char* filename, const char* extension, uint32_t file_size, uint16_t first_block_number, const char* password);
void set_fat12_entry(uint16_t cluster, uint16_t value);
uint16_t get_fat12_entry(uint16_t cluster);
const char* get_cluster_status(uint16_t cluster);
void create_file_system(const char* file_name, int block_size_kb);

// File directory functions
vector<string> split_path(const std::string& path);
void print_directory_entries(const std::string& path, Directory_Entry* dir_entries);
void dir(const char* dir_name);
void mkdir(const char* dir_name);
void rmdir(const char* dir_name);
void write_file(const char* file_path, const char* linux_file);
void read_file(const char* file_path, const char* linux_file);
void delete_file(const char* file_path);
void dumpe2fs();
void chmod_file(const char* file_path, const char* permissions);
void add_password(const char* file_path, const char* password);

#endif // FAT12_FILE_SYSTEM_H
