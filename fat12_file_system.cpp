#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <cstdio>
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
}

void writeFAT12(fstream &file, FAT12Entry *fat) {
    size_t fatSize = MAX_BLOCKS * sizeof(FAT12Entry); // Calculate size of the FAT12 table
    file.seekp(sizeof(SuperBlock) + MAX_BLOCKS / 8, ios::beg);
    file.write(reinterpret_cast<char*>(fat), fatSize);
}

uint32_t readFAT12Entry(const FAT12Entry *fat, uint32_t currentBlock) {
    return fat[currentBlock];
}

int chmod(const string &fileSystemFile, const string &path, bool readPermission, bool writePermission) {
    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);

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
        for (auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (!string(entry.extension).empty()) {
                entryName += "." + string(entry.extension, strnlen(entry.extension, sizeof(entry.extension)));
            }
            if (entryName == dirs[i]) {
                if (i == dirs.size() - 1) {
                    entry.attributes.read_permission = readPermission;
                    entry.attributes.write_permission = writePermission;

                    file.seekp(currentBlock * superBlock.blockSize, ios::beg);
                    file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));
                    file.flush();

                    file.close();
                    cout << "Permissions changed successfully." << endl;
                    return 0;
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
            cerr << "Directory or file not found: " << dirs[i] << endl;
            return -1;
        }
    }

    file.close();
    cerr << "Failed to change permissions." << endl;
    return -1;
}

int addpw(const string &fileSystemFile, const string &path, const string &password) {
    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);

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
        for (auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (!string(entry.extension).empty()) {
                entryName += "." + string(entry.extension, strnlen(entry.extension, sizeof(entry.extension)));
            }
            if (entryName == dirs[i]) {
                if (i == dirs.size() - 1) {
                    strncpy(entry.password, password.c_str(), sizeof(entry.password) - 1);
                    entry.password[sizeof(entry.password) - 1] = '\0'; // Ensure null termination

                    file.seekp(currentBlock * superBlock.blockSize, ios::beg);
                    file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));
                    file.flush();

                    file.close();
                    cout << "Password added/changed successfully." << endl;
                    return 0;
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
            cerr << "Directory or file not found: " << dirs[i] << endl;
            return -1;
        }
    }

    file.close();
    cerr << "Failed to add/change password." << endl;
    return -1;
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
}

// Reading directory entries from a specific block
vector<DirectoryEntry> readDirectoryEntries(fstream &file, uint32_t block) {
    vector<DirectoryEntry> entries(16); // Assuming a block can hold 16 directory entries
    file.seekg(block * 512, ios::beg);
    file.read(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));

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

    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);

    uint8_t free_blocks[MAX_BLOCKS / 8];
    readFreeBlocks(file, free_blocks);

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);

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

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);

    uint32_t currentBlock = superBlock.rootDirectory; // Start at the correct root directory block

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
            for (const auto &entry : entries) {
                string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
                if (entryName == dir && entry.attributes.is_directory) {
                    currentBlock = entry.first_block_number;
                    found = true;
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

    uint8_t free_blocks[MAX_BLOCKS / 8];
    readFreeBlocks(file, free_blocks);
    cout << "Free blocks read successfully" << endl;

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);
    cout << "FAT12 table read successfully" << endl;

    uint32_t currentBlock = superBlock.rootDirectory;
    cout << "Reading directory entries from block: " << currentBlock << endl;
    
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
        for (auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (entryName == dirs[i]) {
                cout << "Found directory: " << entryName << " in block: " << currentBlock << endl;
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

                    cout << "Before clearing directory entry:" << endl;
                    for (const auto &e : entries) {
                        if (e.filename[0] != 0) {
                            cout << "Name: " << string(e.filename, strnlen(e.filename, sizeof(e.filename))) << endl;
                        }
                    }

                    // Clear the directory entry
                    for (auto &e : entries) {
                        string eName(e.filename, strnlen(e.filename, sizeof(e.filename)));
                        if (eName == dirs[i]) {
                            memset(&e, 0, sizeof(DirectoryEntry));
                            break;
                        }
                    }

                    file.seekp(currentBlock * superBlock.blockSize, ios::beg);
                    file.write(reinterpret_cast<char*>(entries.data()), entries.size() * sizeof(DirectoryEntry));
                    file.flush(); // Ensure data is written to the file
                    cout << "Cleared directory entry for: " << dirs[i] << " in block: " << currentBlock << endl;

                    // Mark the directory block as free
                    int dirBlock = entry.first_block_number;
                    fat[dirBlock] = FAT_FREE;
                    free_blocks[dirBlock / 8] |= (1 << (dirBlock % 8));
                    writeFAT12(file, fat);
                    writeFreeBlocks(file, free_blocks);
                    cout << "Marked block " << dirBlock << " as free" << endl;

                    // Initialize the cleared directory block to empty entries
                    DirectoryEntry emptyEntries[16] = {};
                    file.seekp(dirBlock * superBlock.blockSize, ios::beg);
                    file.write(reinterpret_cast<char*>(emptyEntries), sizeof(emptyEntries));
                    cout << "Cleared directory block: " << dirBlock << endl;

                    cout << "After clearing directory entry:" << endl;
                    entries = readDirectoryEntries(file, currentBlock);
                    bool cleared = true;
                    for (const auto &e : entries) {
                        if (e.filename[0] != 0) {
                            cout << "Name: " << string(e.filename, strnlen(e.filename, sizeof(e.filename))) << endl;
                            if (string(e.filename, strnlen(e.filename, sizeof(e.filename))) == dirs[i]) {
                                cleared = false;
                            }
                        }
                    }

                    file.close();

                    if (!cleared) {
                        cerr << "Error: Directory entry not cleared for: " << dirs[i] << endl;
                        return -1;
                    }

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

int dumpe2fs(const string &fileSystemFile) {
    fstream file(fileSystemFile, ios::binary | ios::in);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);
    cout << "Superblock information:" << endl;
    cout << "Total blocks: " << superBlock.totalBlocks << endl;
    cout << "Free blocks: " << superBlock.freeBlocks << endl;
    cout << "Block size: " << superBlock.blockSize << " bytes" << endl;
    cout << "Root directory block: " << superBlock.rootDirectory << endl;
    cout << "First data block: " << superBlock.firstDataBlock << endl;

    uint8_t free_blocks[MAX_BLOCKS / 8];
    readFreeBlocks(file, free_blocks);
    cout << "Free blocks bitmap (hex):" << endl;
    for (int i = 0; i < MAX_BLOCKS / 8; i++) {
        if (i % 16 == 0) {
            cout << endl;
        }
        cout << hex << setw(2) << setfill('0') << (int)free_blocks[i];
    }
    cout << endl;

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);
    cout << "FAT12 table (non-empty blocks):" << endl;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (fat[i] != FAT_FREE) {
            cout << "Block " << i << ": " << hex << fat[i] << endl;
        }
    }

    cout << "Root directory entries:" << endl;
    vector<DirectoryEntry> rootEntries = readDirectoryEntries(file, superBlock.rootDirectory);
    for (const auto &entry : rootEntries) {
        if (entry.filename[0] != 0) {
            cout << "Name: " << string(entry.filename, strnlen(entry.filename, sizeof(entry.filename))) << endl;
            cout << "First block: " << entry.first_block_number << endl;
            cout << "Is directory: " << (entry.attributes.is_directory ? "Yes" : "No") << endl;
        }
    }

    file.close();
    return 0;
}

int readFile(const string &fileSystemFile, const string &path) {
    cout << "Reading file: " << path << endl;
    cout << "From file system: " << fileSystemFile << endl;

    fstream file(fileSystemFile, ios::binary | ios::in);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);

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

    DirectoryEntry fileEntry;
    bool fileFound = false;

    for (size_t i = 0; i < dirs.size(); ++i) {
        bool found = false;
        vector<DirectoryEntry> entries = readDirectoryEntries(file, currentBlock);
        for (const auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (entry.extension[0] != ' ') {
                entryName += "." + string(entry.extension, strnlen(entry.extension, sizeof(entry.extension)));
            }
            cout << "Entry: " << entryName << endl;
            if (entryName == dirs[i]) {
                if (i == dirs.size() - 1) {
                    fileEntry = entry;
                    fileFound = true;
                    cout << "File found: " << entryName << endl;
                    found = true;
                    break;
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
            cerr << "Directory or file not found: " << dirs[i] << endl;
            return -1;
        }
    }

    if (!fileFound) {
        cerr << "File not found: " << dirs.back() << endl;
        return -1;
    }

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);

    uint32_t currentDataBlock = fileEntry.first_block_number;
    size_t fileSize = fileEntry.file_size;
    vector<uint8_t> data(fileSize);

    size_t offset = 0;
    while (offset < fileSize) {
        cout << "Reading block: " << currentDataBlock << " at offset: " << offset << endl;
        file.seekg(currentDataBlock * superBlock.blockSize, ios::beg);
        size_t chunkSize = min(fileSize - offset, (size_t)superBlock.blockSize);
        file.read(reinterpret_cast<char*>(data.data() + offset), chunkSize);
        cout << "Read " << chunkSize << " bytes from block " << currentDataBlock << endl;
        cout << "Bytes read: ";
        for (size_t i = 0; i < chunkSize; ++i) {
            cout << hex << static_cast<int>(data[offset + i]) << " ";
        }
        cout << endl;
        offset += chunkSize;
        currentDataBlock = readFAT12Entry(fat, currentDataBlock);
        if (currentDataBlock == FAT_END) {
            break;
        }
    }

    file.close();
    cout << "File content:" << endl;
    cout.write(reinterpret_cast<const char*>(data.data()), data.size());
    cout << endl;
    return 0;
}

int writeFile(const string &fileSystemFile, const string &path, const vector<uint8_t> &data) {
    fstream file(fileSystemFile, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Failed to open file system file: " << fileSystemFile << endl;
        return -1;
    }

    SuperBlock superBlock;
    readSuperBlock(file, superBlock);

    uint8_t free_blocks[MAX_BLOCKS / 8];
    readFreeBlocks(file, free_blocks);

    FAT12Entry fat[MAX_BLOCKS];
    readFAT12(file, fat);

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
        for (auto &entry : entries) {
            string entryName(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
            if (!string(entry.extension).empty()) {
                entryName += "." + string(entry.extension, strnlen(entry.extension, sizeof(entry.extension)));
            }
            if (entryName == dirs[i]) {
                if (i == dirs.size() - 1) {
                    cerr << "File or directory already exists: " << dirs[i] << endl;
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

            DirectoryEntry newFile;
            memset(&newFile, 0, sizeof(DirectoryEntry));

            string filename = dirs[i];
            string name, extension;
            size_t dot_pos = filename.find('.');
            if (dot_pos != string::npos) {
                name = filename.substr(0, dot_pos);
                extension = filename.substr(dot_pos + 1);
            } else {
                name = filename;
                extension = "";
            }

            // Pad filename and extension
            name.resize(8, ' ');
            extension.resize(3, ' ');

            strncpy(newFile.filename, name.c_str(), sizeof(newFile.filename));
            strncpy(newFile.extension, extension.c_str(), sizeof(newFile.extension));

            newFile.attributes.is_directory = 0;
            newFile.attributes.read_permission = 1;
            newFile.attributes.write_permission = 1;
            newFile.creation_date = {1, 1, 40}; // Date: 01/01/1980
            newFile.last_modification_date = newFile.creation_date;
            newFile.first_block_number = freeBlock;
            newFile.file_size = data.size();

            addDirectoryEntry(file, newFile, currentBlock);

            // Write data to blocks
            uint32_t currentDataBlock = freeBlock;
            size_t offset = 0;
            while (offset < data.size()) {
                int nextFreeBlock = findFreeBlock(fat);
                if (nextFreeBlock == -1) {
                    cerr << "No free blocks available for data" << endl;
                    return -1;
                }

                fat[currentDataBlock] = nextFreeBlock;
                currentDataBlock = nextFreeBlock;

                free_blocks[currentDataBlock / 8] &= ~(1 << (currentDataBlock % 8));

                size_t chunkSize = min(data.size() - offset, (size_t)superBlock.blockSize);
                file.seekp(currentDataBlock * superBlock.blockSize, ios::beg);
                file.write(reinterpret_cast<const char*>(data.data() + offset), chunkSize);
                offset += chunkSize;
            }

            fat[currentDataBlock] = FAT_END;
            writeFAT12(file, fat);
            writeFreeBlocks(file, free_blocks);

            file.close();
            cout << "File written successfully." << endl;
            return 0;
        }
    }

    file.close();
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <operation> <file_system_file> [block_size/path]" << endl;
        return 1;
    }

    string operation = argv[1];
    string fileSystemFile = argv[2];

    if (operation == "makeFileSystem") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " makeFileSystem <block_size> <file_system_file>" << endl;
            return 1;
        }

        uint32_t blockSize;
        string blockSizeStr = argv[2];
        fileSystemFile = argv[3];

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

        SuperBlock superBlock;
        initializeSuperBlock(superBlock, blockSize);
        writeSuperBlock(file, superBlock);

        uint8_t free_blocks[MAX_BLOCKS / 8];
        initializeFreeBlocks(free_blocks);
        writeFreeBlocks(file, free_blocks);

        FAT12Entry fat[MAX_BLOCKS];
        initializeFAT12(fat);
        writeFAT12(file, fat);

        initializeRootDirectory(file, superBlock);

        file.seekp(4 * 1024 * 1024 - 1, ios::beg);
        file.write("", 1);

        file.close();
        cerr << "File system created successfully." << endl;
    } else if (operation == "dir") {
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
        if (mkdir(fileSystemFile, path) != 0) {
            cerr << "Failed to create directory." << endl;
        }
    } else if (operation == "rmdir") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " rmdir <file_system_file> <path>" << endl;
            return 1;
        }
        string path = argv[3];
        if (rmdir(fileSystemFile, path) == 0) {
            cerr << "Directory removed successfully." << endl;
        } else {
            cerr << "Failed to remove directory." << endl;
        }
    } else if (operation == "dumpe2fs") {
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " dumpe2fs <file_system_file>" << endl;
            return 1;
        }
        if (dumpe2fs(fileSystemFile) != 0) {
            cerr << "Failed to dump file system information." << endl;
        } 
    } else if (operation == "read") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " read <file_system_file> <path>" << endl;
            return 1;
        }
        string path = argv[3];
        if (readFile(fileSystemFile, path) != 0) {
            cerr << "Failed to read file." << endl;
        }
    } else if (operation == "write") {
        if (argc != 5) {
            cerr << "Usage: " << argv[0] << " write <file_system_file> <path> <data>" << endl;
            return 1;
        }
        string path = argv[3];
        string data_str = argv[4];
        vector<uint8_t> data(data_str.begin(), data_str.end());
        if (writeFile(fileSystemFile, path, data) != 0) {
            cerr << "Failed to write file." << endl;
        }
        cout << "File written successfully." << endl;
    }  else if (operation == "chmod") {
        if (argc != 6) {
            cerr << "Usage: " << argv[0] << " chmod <file_system_file> <path> <read_permission> <write_permission>" << endl;
            return 1;
        }
        string path = argv[3];
        bool readPermission = (argv[4][0] == '1');
        bool writePermission = (argv[5][0] == '1');
        if (chmod(fileSystemFile, path, readPermission, writePermission) != 0) {
            cerr << "Failed to change permissions." << endl;
            return 1;
        }
        cout << "Permissions changed successfully." << endl;
    } else if (operation == "addpw") {
        if (argc != 5) {
            cerr << "Usage: " << argv[0] << " addpw <file_system_file> <path> <password>" << endl;
            return 1;
        }
        string path = argv[3];
        string password = argv[4];
        if (addpw(fileSystemFile, path, password) != 0) {
            cerr << "Failed to add/change password." << endl;
            return 1;
        }
        cout << "Password added/changed successfully." << endl;
    } else {
        cerr << "Invalid operation: " << operation << endl;
        return 1;
    }

    return 0;
}