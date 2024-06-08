#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "fat12_file_system.h"

using namespace std;

// Function definitions

void initializeSuperBlock(SuperBlock &superBlock, uint32_t blockSize) {
    superBlock.totalBlocks = MAX_BLOCKS;
    superBlock.freeBlocks = MAX_BLOCKS - 20; // 20 blocks used for metadata
    superBlock.blockSize = blockSize;
    superBlock.rootDirectory = 19; // Setting root directory block correctly
    superBlock.firstDataBlock = 20;
}

void initializeRootDirectory(ofstream &file, SuperBlock &superBlock) {
    DirectoryEntry emptyEntries[16] = {};
    file.seekp(superBlock.rootDirectory * superBlock.blockSize, ios::beg);
    file.write(reinterpret_cast<char*>(emptyEntries), sizeof(emptyEntries));
    cerr << "Root directory initialized. Size: " << sizeof(emptyEntries) << " bytes" << endl;
}

void writeSuperBlock(ofstream &file, SuperBlock &superBlock) {
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    cerr << "Superblock written. Size: " << sizeof(SuperBlock) << " bytes" << endl;
}

void writeSuperBlock(fstream &file, SuperBlock &superBlock) {
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    cerr << "Superblock written. Size: " << sizeof(SuperBlock) << " bytes" << endl;
}

void readSuperBlock(ifstream &file, SuperBlock &superBlock) {
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
}

void readSuperBlock(fstream &file, SuperBlock &superBlock) {
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
}

void initializeFreeBlocks(uint8_t *free_blocks) {
    memset(free_blocks, 0xFF, MAX_BLOCKS / 8); // Mark all blocks as free
    for (int i = 0; i < 20; i++) {
        free_blocks[i / 8] &= ~(1 << (i % 8)); // Mark first 20 blocks as used
    }
}

void writeFreeBlocks(ofstream &file, uint8_t *free_blocks) {
    file.seekp(sizeof(SuperBlock), ios::beg);
    file.write(reinterpret_cast<char*>(free_blocks), MAX_BLOCKS / 8);
    cerr << "Free blocks written. Size: " << MAX_BLOCKS / 8 << " bytes" << endl;
}

void writeFreeBlocks(fstream &file, uint8_t *free_blocks) {
    file.seekp(sizeof(SuperBlock), ios::beg);
    file.write(reinterpret_cast<char*>(free_blocks), MAX_BLOCKS / 8);
    cerr << "Free blocks written. Size: " << MAX_BLOCKS / 8 << " bytes" << endl;
}

void readFreeBlocks(ifstream &file, uint8_t *free_blocks) {
    file.seekg(sizeof(SuperBlock), ios::beg);
    file.read(reinterpret_cast<char*>(free_blocks), MAX_BLOCKS / 8);
}

void readFreeBlocks(fstream &file, uint8_t *free_blocks) {
    file.seekg(sizeof(SuperBlock), ios::beg);
    file.read(reinterpret_cast<char*>(free_blocks), MAX_BLOCKS / 8);
}

void initializeFAT12(FAT12Entry *fat) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        fat[i] = FAT_FREE;
    }
}

void writeFAT12(ofstream &file, FAT12Entry *fat) {
    size_t fatSize = MAX_BLOCKS * sizeof(FAT12Entry); // Calculate size of the FAT12 table
    file.seekp(sizeof(SuperBlock) + MAX_BLOCKS / 8, ios::beg);
    file.write(reinterpret_cast<char*>(fat), fatSize);
    cerr << "FAT12 table written. Size: " << fatSize << " bytes" << endl;
}

void writeFAT12(fstream &file, FAT12Entry *fat) {
    size_t fatSize = MAX_BLOCKS * sizeof(FAT12Entry); // Calculate size of the FAT12 table
    file.seekp(sizeof(SuperBlock) + MAX_BLOCKS / 8, ios::beg);
    file.write(reinterpret_cast<char*>(fat), fatSize);
    cerr << "FAT12 table written. Size: " << fatSize << " bytes" << endl;
}

void readFAT12(ifstream &file, FAT12Entry *fat) {
    size_t fatSize = MAX_BLOCKS * sizeof(FAT12Entry);
    file.seekg(sizeof(SuperBlock) + MAX_BLOCKS / 8, ios::beg);
    file.read(reinterpret_cast<char*>(fat), fatSize);
}

void readFAT12(fstream &file, FAT12Entry *fat) {
    size_t fatSize = MAX_BLOCKS * sizeof(FAT12Entry);
    file.seekg(sizeof(SuperBlock) + MAX_BLOCKS / 8, ios::beg);
    file.read(reinterpret_cast<char*>(fat), fatSize);
}

int findFreeBlock(FAT12Entry *fat) {
    for (int i = 20; i < MAX_BLOCKS; i++) {
        if (fat[i] == FAT_FREE) {
            return i;
        }
    }
    return -1;
}

// Adding a directory entry
void addDirectoryEntry(fstream &file, DirectoryEntry &entry, uint32_t block) {
    vector<DirectoryEntry> entries = readDirectoryEntries(file, block);
    bool entryAdded = false;

    // Find the first empty slot to add the new entry
    for (auto &dirEntry : entries) {
        if (dirEntry.filename[0] == 0) { // Empty slot found
            dirEntry = entry;
            entryAdded = true;
            break;
        }
    }

    if (!entryAdded) {
        cerr << "No empty slot found in block: " << block << endl;
        return;
    }

    // Write the updated entries back to the block
    file.seekp(block * 512, ios::beg);
    file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));
    file.flush(); // Ensure the data is written to the file
    cerr << "Added directory entry for: " << entry.filename << " in block: " << block << endl;

    // Debug: Verify the entries were written correctly
    vector<DirectoryEntry> debugEntries = readDirectoryEntries(file, block);
    cerr << "Debug: Directory entries after writing:\n";
    for (const auto &debugEntry : debugEntries) {
        if (debugEntry.filename[0] != 0) {
            string debugEntryName(debugEntry.filename, strnlen(debugEntry.filename, sizeof(debugEntry.filename)));
            cerr << "Name: " << debugEntryName << endl;
        } else {
            cerr << "Empty directory entry" << endl;
        }
    }
}

// Reading directory entries from a specific block
vector<DirectoryEntry> readDirectoryEntries(fstream &file, uint32_t block) {
    vector<DirectoryEntry> entries(16); // Assuming a block can hold 16 directory entries
    file.seekg(block * 512, ios::beg);
    file.read(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));

    // Debug: Verify the entries were read correctly
    cerr << "Debug: Directory entries read from block " << block << ":\n";
    for (const auto &entry : entries) {
        if (entry.filename[0] != 0) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            cerr << "Name: " << entryName << endl;
        } else {
            cerr << "Empty directory entry" << endl;
        }
    }

    return entries;
}

bool validateFileSystem(const string &fileSystemFile, const string &newDirName) {
    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return false;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);

    uint32_t rootBlock = superBlock.rootDirectory;
    vector<DirectoryEntry> rootEntries = readDirectoryEntries(file, rootBlock);

    bool dirFound = false;
    cout << "Root directory entries:\n";
    cout << "------------------------\n";
    for (const auto &entry : rootEntries) {
        if (entry.filename[0] != 0) { // Only print non-empty entries
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            string entryType = entry.attributes.is_directory ? "Directory" : "File";
            cout << "Name: " << entryName << ", Type: " << entryType << ", First Block: " << entry.first_block_number << endl;

            if (entryName == newDirName && entry.attributes.is_directory) {
                dirFound = true;
                uint32_t dirBlock = entry.first_block_number;

                // Check that the directory block is initialized
                vector<DirectoryEntry> dirEntries = readDirectoryEntries(file, dirBlock);
                for (const auto &dirEntry : dirEntries) {
                    if (dirEntry.filename[0] != 0) {
                        cerr << "Error: Directory block is not empty" << endl;
                        return false;
                    }
                }

                cout << "Directory '" << newDirName << "' is correctly created in block " << dirBlock << endl;
                break;
            }
        } else {
            cout << "Empty directory entry" << endl;
        }
    }

    if (!dirFound) {
        cerr << "Error: Directory '" << newDirName << "' not found in root directory" << endl;
        return false;
    }

    file.close();
    return true;
}

// Updated mkdir function
int mkdir(const string &fileSystemFile, const string &path) {
    cout << "Creating directory: " << path << endl;
    cout << "File system file: " << fileSystemFile << endl;

    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);
    cout << "SuperBlock read successfully" << endl;
    cout << "Root directory block: " << superBlock.rootDirectory << endl;

    uint8_t free_blocks[MAX_BLOCKS / 8];
    readFreeBlocks(file, free_blocks);
    cout << "Free blocks read successfully" << endl;

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);
    cout << "FAT12 table read successfully" << endl;

    uint32_t currentBlock = superBlock.rootDirectory;
    cout << "Starting at root directory block: " << currentBlock << endl;

    vector<string> dirs;
    size_t start = 0, end;

    if (path[0] == '\\') {
        start = 1;
    }
    while ((end = path.find('\\', start)) != string::npos) {
        string dir = path.substr(start, end - start);
        if (!dir.empty()) {
            dirs.push_back(dir);
        }
        start = end + 1;
    }
    if (start < path.size()) {
        dirs.push_back(path.substr(start));
    }

    for (size_t i = 0; i < dirs.size(); ++i) {
        bool found = false;
        vector<DirectoryEntry> entries = readDirectoryEntries(file, currentBlock);
        cout << "Reading directory entries from block: " << currentBlock << endl;
        for (auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (entryName == dirs[i]) {
                if (i == dirs.size() - 1) {
                    cerr << "Directory already exists: " << dirs[i] << endl;
                    return -1;
                }
                if (entry.attributes.is_directory) {
                    currentBlock = entry.first_block_number;
                    found = true;
                    break;
                } else {
                    cerr << "Not a directory: " << dirs[i] << endl;
                    return -1;
                }
            }
        }
        if (!found) {
            if (i != dirs.size() - 1) {
                cerr << "Parent directory not found: " << dirs[i] << endl;
                return -1;
            }

            int freeBlock = findFreeBlock(fat);
            if (freeBlock == -1) {
                cerr << "No free blocks available" << endl;
                return -1;
            }

            fat[freeBlock] = FAT_END;
            free_blocks[freeBlock / 8] &= ~(1 << (freeBlock % 8));
            writeFAT12(file, fat);
            writeFreeBlocks(file, free_blocks);

            DirectoryEntry newDir;
            memset(&newDir, 0, sizeof(DirectoryEntry));
            strncpy(newDir.filename, dirs[i].c_str(), sizeof(newDir.filename) - 1);
            newDir.filename[sizeof(newDir.filename) - 1] = '\0'; // Ensure null termination
            newDir.attributes.is_directory = 1;
            newDir.attributes.read_permission = 1;
            newDir.attributes.write_permission = 1;
            newDir.creation_date = {1, 1, 40}; // Date: 01/01/1980
            newDir.last_modification_date = newDir.creation_date;
            newDir.first_block_number = freeBlock;
            newDir.file_size = 0;

            // Write the new directory entry
            addDirectoryEntry(file, newDir, currentBlock);
            cout << "Added directory entry for: " << dirs[i] << " in block: " << currentBlock << endl;

            // Initialize new directory block with empty entries
            DirectoryEntry emptyEntries[16] = {};
            file.seekp(freeBlock * superBlock.blockSize, ios::beg);
            file.write(reinterpret_cast<char*>(emptyEntries), sizeof(emptyEntries));
            cout << "Initialized new directory block: " << freeBlock << endl;

            currentBlock = freeBlock;
        }
    }

    file.close();
    cout << "Directory created successfully." << endl;

    return 0;
}

// Updated dir function
void dir(const string &fileSystemFile, const string &path) {
    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);
    cout << "SuperBlock read successfully" << endl;
    cout << "Root directory block: " << superBlock.rootDirectory << endl;

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);
    cout << "FAT12 table read successfully" << endl;

    uint32_t currentBlock = superBlock.rootDirectory; // Start at the correct root directory block
    cout << "Starting at root directory block: " << currentBlock << endl;

    if (path != "\\") {
        vector<string> dirs;
        size_t start = 0, end;
        if (path[0] == '\\') {
            start = 1;
        }
        while ((end = path.find('\\', start)) != string::npos) {
            string dir = path.substr(start, end - start);
            if (!dir.empty()) {
                dirs.push_back(dir);
            }
            start = end + 1;
        }
        if (start < path.size()) {
            dirs.push_back(path.substr(start));
        }

        for (const auto &dir : dirs) {
            bool found = false;
            vector<DirectoryEntry> entries = readDirectoryEntries(file, currentBlock);
            cout << "Looking for directory: " << dir << " in block: " << currentBlock << endl;
            for (const auto &entry : entries) {
                string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
                if (entryName == dir && entry.attributes.is_directory) {
                    currentBlock = entry.first_block_number;
                    found = true;
                    cout << "Found directory: " << dir << " at block: " << currentBlock << endl;
                    break;
                }
            }
            if (!found) {
                cerr << "Directory not found: " << dir << endl;
                return;
            }
        }
    }

    vector<DirectoryEntry> entries = readDirectoryEntries(file, currentBlock);
    cout << "Reading directory entries from block: " << currentBlock << endl;

    cout << "Permissions  Size       Creation Date       Modification Date    Password  Name\n";
    cout << "--------------------------------------------------------------------------------\n";

    for (const auto &entry : entries) {
        if (entry.filename[0] != 0) { // Only print non-empty entries
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            string entryExtension(entry.extension, strnlen(entry.extension, sizeof(entry.extension)));
            string fullName = entryName + (entryExtension.empty() ? "" : "." + entryExtension);

            string permissions = "";
            permissions += (entry.attributes.read_permission ? "r" : "-");
            permissions += (entry.attributes.write_permission ? "w" : "-");
            permissions += (entry.attributes.is_directory ? "d" : "-");

            cout << permissions << "      "
                 << entry.file_size << "       "
                 << entry.creation_date.year + 1980 << "-"
                 << (int)entry.creation_date.month << "-"
                 << (int)entry.creation_date.day << "       "
                 << entry.last_modification_date.year + 1980 << "-"
                 << (int)entry.last_modification_date.month << "-"
                 << (int)entry.last_modification_date.day << "       "
                 << (entry.attributes.is_protected ? "Yes" : "No") << "        "
                 << fullName << "\n";
        }
    }

    file.close();
    cout << "File system file closed: " << fileSystemFile << endl;
}

// Remove directory function
int rmdir(const string &fileSystemFile, const string &path) {
    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);
    cout << "SuperBlock read successfully" << endl;
    cout << "Root directory block: " << superBlock.rootDirectory << endl;

    uint8_t free_blocks[MAX_BLOCKS / 8];
    readFreeBlocks(file, free_blocks);
    cout << "Free blocks read successfully" << endl;

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);
    cout << "FAT12 table read successfully" << endl;

    uint32_t currentBlock = superBlock.rootDirectory;
    vector<string> dirs;
    size_t start = 0, end;

    if (path[0] == '\\') {
        start = 1;
    }
    while ((end = path.find('\\', start)) != string::npos) {
        string dir = path.substr(start, end - start);
        if (!dir.empty()) {
            dirs.push_back(dir);
        }
        start = end + 1;
    }
    if (start < path.size()) {
        dirs.push_back(path.substr(start));
    }

    for (size_t i = 0; i < dirs.size(); ++i) {
        bool found = false;
        vector<DirectoryEntry> entries = readDirectoryEntries(file, currentBlock);
        cout << "Reading directory entries from block: " << currentBlock << endl;
        for (auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (entryName == dirs[i]) {
                if (i == dirs.size() - 1) {
                    if (!entry.attributes.is_directory) {
                        cerr << "Not a directory: " << dirs[i] << endl;
                        return -1;
                    }

                    // Check if directory is empty
                    vector<DirectoryEntry> subEntries = readDirectoryEntries(file, entry.first_block_number);
                    bool isEmpty = true;
                    for (const auto &subEntry : subEntries) {
                        if (subEntry.filename[0] != 0) {
                            isEmpty = false;
                            break;
                        }
                    }
                    if (!isEmpty) {
                        cerr << "Directory not empty: " << dirs[i] << endl;
                        return -1;
                    }

                    // Clear the directory entry
                    int dirBlock = entry.first_block_number;
                    memset(&entry, 0, sizeof(DirectoryEntry));
                    file.seekp(currentBlock * superBlock.blockSize, ios::beg);
                    file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));
                    file.flush();
                    cout << "Cleared directory entry for: " << dirs[i] << " in block: " << currentBlock << endl;

                    // Mark the block as free
                    fat[dirBlock] = FAT_FREE;
                    free_blocks[dirBlock / 8] |= (1 << (dirBlock % 8));
                    writeFAT12(file, fat);
                    writeFreeBlocks(file, free_blocks);
                    cout << "Marked block " << dirBlock << " as free" << endl;

                    file.close();
                    cout << "Directory removed successfully." << endl;
                    return 0;
                } else {
                    currentBlock = entry.first_block_number;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            cerr << "Directory not found: " << dirs[i] << endl;
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <operation> <block_size> <file_system_file>" << endl;
        return 1;
    }

    string operation = argv[1];
    string blockSizeStr = argv[2];
    string fileSystemFile = argv[3];

    if (operation == "makeFileSystem") {
        uint32_t blockSize;
        if (blockSizeStr == "1") {
            blockSize = 1024;
        } else if (blockSizeStr == "0.5") {
            blockSize = 512;
        } else {
            cerr << "Error: Block size must be either 1 or 0.5 KB." << endl;
            return 1;
        }

        ofstream file(fileSystemFile, ios::binary);
        if (!file.is_open()) {
            cerr << "Failed to create file system file: " << fileSystemFile << endl;
            return 1;
        }

        cerr << "Initializing superblock..." << endl;
        SuperBlock superBlock;
        initializeSuperBlock(superBlock, blockSize);
        cerr << "Writing superblock to file..." << endl;
        writeSuperBlock(file, superBlock);

        cerr << "Initializing free blocks..." << endl;
        uint8_t free_blocks[MAX_BLOCKS / 8];
        initializeFreeBlocks(free_blocks);
        cerr << "Writing free blocks to file..." << endl;
        writeFreeBlocks(file, free_blocks);

        cerr << "Initializing FAT12 table..." << endl;
        FAT12Entry fat[MAX_BLOCKS];
        initializeFAT12(fat);
        cerr << "Writing FAT12 table to file..." << endl;
        writeFAT12(file, fat);

        cerr << "Initializing root directory..." << endl;
        initializeRootDirectory(file, superBlock);

        // Fill the rest of the file with zeros to ensure the file is exactly 4 MB
        cerr << "Filling the rest of the file to ensure it is 4 MB..." << endl;
        file.seekp(4 * 1024 * 1024 - 1, ios::beg);
        file.write("", 1);

        file.close();
        cerr << "File system created successfully." << endl;
    } else if (operation == "dir") {
        fileSystemFile = argv[2];
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " dir <file_system_file> <path>" << endl;
            return 1;
        }
        string path = argv[3];
        dir(fileSystemFile, path);
    } else if (operation == "mkdir") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " mkdir <file_system_file> <path>" << endl;
            return 1;
        }
        string path = argv[3];
        string fileSystemFile = argv[2];
        cout << "Creating directory: " << path << endl;
        if (mkdir(fileSystemFile, path) == 0) {
            cerr << "Directory created successfully." << endl;
        } else {
            cerr << "Failed to create directory." << endl;
        }
    } else if (operation == "rmdir") {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " rmdir <file_system_file> <path>" << endl;
        return 1;
    }
    string path = argv[3];
    string fileSystemFile = argv[2];
    cout << "Removing directory: " << path << endl;
    if (rmdir(fileSystemFile, path) == 0) {
        cerr << "Directory removed successfully." << endl;
    } else {
        cerr << "Failed to remove directory." << endl;
    }
    } else {
        cerr << "Unknown operation: " << operation << endl;
        return 1;
    }

    return 0;
}

