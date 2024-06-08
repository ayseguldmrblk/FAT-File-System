#ifndef FAT12_FILE_SYSTEM_H
#define FAT12_FILE_SYSTEM_H

#include <cstdint>
#include <string>
#include <fstream>

using namespace std;

// Constants
#define MAX_BLOCKS 4096
#define BLOCK_SIZE_512 512
#define BLOCK_SIZE_1024 1024

// SuperBlock structure
typedef struct SuperBlock {
    uint32_t totalBlocks;
    uint32_t freeBlocks;
    uint32_t blockSize;
    uint32_t rootDirectory;
    uint32_t firstDataBlock;
} SuperBlock;

typedef uint16_t FAT12Entry;
#define FAT_FREE 0x0000
#define FAT_END  0xFFFF

// File attributes
typedef struct file_attributes {
    uint8_t read_permission : 1;
    uint8_t write_permission : 1;
    uint8_t is_directory : 1;
    uint8_t is_protected : 1;
    uint8_t filename_exceeds : 1;
    uint8_t reserved : 3;
} file_attributes;

// File time structure
typedef struct file_time {
    uint8_t hours : 5;
    uint8_t minutes : 6;
    uint8_t seconds : 5;
} file_time;

// File date structure
typedef struct file_date {
    uint8_t day : 5;
    uint8_t month : 4;
    uint16_t year : 7;
} file_date;

// Directory entry structure
typedef struct DirectoryEntry {
    char filename[8]; // 64 bits for the filename (8 characters with 8 bits each)
    char extension[3]; // File extension
    file_attributes attributes; // File attributes
    char reserved[18]; // Extended filename (if it exceeds 8 characters)
    file_time last_modificaton_time; // Time of last modification
    file_date last_modification_date; // Date of last modification
    file_time creation_time; // Time of creation
    file_date creation_date; // Date of creation
    uint16_t first_block_number; // The first block number of the file
    uint32_t file_size; // Size of the file in bytes
    char password[16]; // Password for the file
} DirectoryEntry;

// Function prototypes
void initializeSuperBlock(SuperBlock &superBlock, uint32_t blockSize);
void writeSuperBlock(std::ofstream &file, SuperBlock &superBlock);
void writeSuperBlock(std::fstream &file, SuperBlock &superBlock);
void readSuperBlock(std::ifstream &file, SuperBlock &superBlock);
void readSuperBlock(std::fstream &file, SuperBlock &superBlock);
void initializeFreeBlocks(uint8_t *free_blocks);
void writeFreeBlocks(std::ofstream &file, uint8_t *free_blocks);
void writeFreeBlocks(std::fstream &file, uint8_t *free_blocks);
void readFreeBlocks(std::ifstream &file, uint8_t *free_blocks);
void readFreeBlocks(std::fstream &file, uint8_t *free_blocks);
void initializeFAT12(FAT12Entry *fat);
void writeFAT12(std::ofstream &file, FAT12Entry *fat);
void writeFAT12(std::fstream &file, FAT12Entry *fat);
void readFAT12(std::ifstream &file, FAT12Entry *fat);
void readFAT12(std::fstream &file, FAT12Entry *fat);
uint32_t readFAT12Entry(const FAT12Entry *fat, uint32_t currentBlock);
vector<DirectoryEntry> readDirectoryEntries(fstream &file, uint32_t block);
int findFreeBlock(uint8_t *free_blocks);

#endif // FAT12_FILE_SYSTEM_H
