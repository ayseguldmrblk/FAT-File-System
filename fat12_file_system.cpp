#include "fat12_file_system.h"
#include <iostream>
#include <iomanip>
#include <ctime>

uint8_t fat12_table[FAT12_SIZE];
Directory_Entry entries[DIRECTORY_ENTRIES_PER_CLUSTER];

using namespace std;

void set_file_time(file_time *ft, int hours, int minutes, int seconds) {
    ft->hours = hours;
    ft->minutes = minutes;
    ft->seconds = seconds / 2;
}

void set_file_date(file_date *fd, int year, int month, int day) {
    fd->year = year - 1980;
    fd->month = month;
    fd->day = day;
}

void initialize_directory_entry(Directory_Entry* entry, const char* filename, const char* extension, uint32_t file_size, uint16_t first_block_number, const char* password) {
    strncpy((char*)&entry->filename, filename, 8);
    strncpy(entry->extension, extension, 3);
    
    entry->attributes.read_permission = 1;
    entry->attributes.write_permission = 1;
    entry->attributes.is_directory = 0;
    entry->attributes.is_protected = password != NULL && strlen(password) > 0;
    entry->attributes.filename_exceeds = strlen(filename) > 8;

    time_t t = time(NULL);
    struct tm *current_time = localtime(&t);

    set_file_time(&entry->creation_time, current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
    set_file_date(&entry->creation_date, current_time->tm_year + 1900, current_time->tm_mon + 1, current_time->tm_mday);
    set_file_time(&entry->last_modification_time, current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
    set_file_date(&entry->last_modification_date, current_time->tm_year + 1900, current_time->tm_mon + 1, current_time->tm_mday);

    entry->first_block_number = first_block_number;
    entry->file_size = file_size;

    if (password) {
        strncpy(entry->password, password, 16);
    }
}

void set_fat12_entry(uint16_t cluster, uint16_t value) {
    uint32_t index = cluster * 3 / 2;
    if (cluster % 2 == 0) {
        fat12_table[index] = value & 0xFF;
        fat12_table[index + 1] = (fat12_table[index + 1] & 0xF0) | ((value >> 8) & 0x0F);
    } else {
        fat12_table[index] = (fat12_table[index] & 0x0F) | ((value << 4) & 0xF0);
        fat12_table[index + 1] = (value >> 4) & 0xFF;
    }
}

uint16_t get_fat12_entry(uint16_t cluster) {
    uint32_t index = cluster * 3 / 2;
    if (cluster % 2 == 0) {
        return fat12_table[index] | ((fat12_table[index + 1] & 0x0F) << 8);
    } else {
        return ((fat12_table[index] & 0xF0) >> 4) | (fat12_table[index + 1] << 4);
    }
}

const char* get_cluster_status(uint16_t cluster) {
    uint16_t entry = get_fat12_entry(cluster);
    if (entry == 0x000) {
        return "Free";
    } else if (entry >= 0xFF0 && entry <= 0xFF6) {
        return "Reserved";
    } else if (entry == 0xFF7) {
        return "Bad";
    } else if (entry >= 0xFF8 && entry <= 0xFFF) {
        return "EOF";
    } else {
        return "Used";
    }
}

void initialize_free_blocks() {
    memset(fat12_table, 0xFF, FAT12_SIZE);
}

void write_super_block(FILE* fs, int block_size_kb) {
    fwrite("FAT12", 5, 1, fs); // File system type
    uint8_t reserved[3] = {0}; // Reserved bytes to align block size
    fwrite(reserved, sizeof(reserved), 1, fs);
    fwrite(&block_size_kb, sizeof(int), 1, fs); // Block size in KB
}

void write_FAT(FILE* fs) {
    fwrite(fat12_table, FAT12_SIZE, 1, fs);
}

void write_root_directory(FILE* fs) {
    fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
}

void print_file_time(file_time* ft) {
    printf("%02d:%02d:%02d", ft->hours, ft->minutes, ft->seconds * 2);
}

void print_file_date(file_date* fd) {
    printf("%04d-%02d-%02d", fd->year + 1980, fd->month, fd->day);
}

vector<string> split_path(const std::string& path) {
    vector<string> components;
    string component;
    for (char ch : path) {
        if (ch == '\\') {
            if (!component.empty()) {
                components.push_back(component);
                component.clear();
            }
        } else {
            component += ch;
        }
    }
    if (!component.empty()) {
        components.push_back(component);
    }
    return components;
}

void print_directory_entries(const std::string& path, Directory_Entry* dir_entries) {
    for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
        Directory_Entry entry = dir_entries[i];
        if (entry.filename != 0 && entry.filename != 0xFFFFFFFFFFFFFFFF) {
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            printf("%c%c           %10d  ", 
                   entry.attributes.read_permission ? 'r' : '-',
                   entry.attributes.write_permission ? 'w' : '-',
                   entry.file_size);
            print_file_date(&entry.creation_date);
            printf(" ");
            print_file_time(&entry.creation_time);
            printf("    ");
            print_file_date(&entry.last_modification_date);
            printf(" ");
            print_file_time(&entry.last_modification_time);
            printf("    %s       %s.%s\n", 
                   entry.attributes.is_protected ? "Yes" : "No",
                   entry_name.c_str(), entry.extension);
        }
    }
}

void dir(const char* path) {
    printf("Permissions  Size       Creation Date       Modification Date    Password  Name\n");
    printf("--------------------------------------------------------------------------------\n");

    std::string target_path = path;
    if (target_path == "\\") {
        print_directory_entries("", entries);
        return;
    }

    if (target_path[0] == '\\') {
        target_path = target_path.substr(1);
    }

    Directory_Entry* current_entries = entries;
    std::vector<std::string> components = split_path(target_path);

    for (const std::string& component : components) {
        bool found = false;
        for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
            Directory_Entry& entry = current_entries[i];
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == component) {
                if (entry.attributes.is_directory) {
                    current_entries = reinterpret_cast<Directory_Entry*>(reinterpret_cast<uintptr_t>(entries) + entry.first_block_number * 1024);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Error: Directory '%s' not found.\n", path);
            return;
        }
    }

    print_directory_entries(target_path, current_entries);
}

bool directory_exists(const std::string& dir_name) {
    for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
        if (entries[i].filename != 0 && entries[i].attributes.is_directory &&
            std::string(reinterpret_cast<char*>(&entries[i].filename), 8).substr(0, dir_name.size()) == dir_name) {
            return true;
        }
    }
    return false;
}

void mkdir(const char* dir_name) {
    std::vector<std::string> components = split_path(dir_name);
    Directory_Entry* current_entries = entries;
    Directory_Entry* parent_entries = nullptr;

    for (size_t i = 0; i < components.size() - 1; ++i) {
        bool found = false;
        for (size_t j = 0; j < DIRECTORY_ENTRIES_PER_CLUSTER; ++j) {
            Directory_Entry& entry = current_entries[j];
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == components[i] && entry.attributes.is_directory) {
                parent_entries = current_entries;
                current_entries = reinterpret_cast<Directory_Entry*>(reinterpret_cast<uintptr_t>(entries) + entry.first_block_number * 1024);
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Error: Parent directory '%s' does not exist.\n", components[i].c_str());
            return;
        }
    }

    std::string new_dir_name = components.back();
    for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
        if (current_entries[i].filename == 0 || current_entries[i].filename == 0xFFFFFFFFFFFFFFFF) {
            initialize_directory_entry(&current_entries[i], new_dir_name.c_str(), "", 0, 0, NULL);
            current_entries[i].attributes.is_directory = 1;
            static uint16_t next_block_number = 3;
            current_entries[i].first_block_number = next_block_number++;
            printf("Directory '%s' created successfully.\n", dir_name);
            return;
        }
    }
    printf("Error: No empty directory entries available.\n");
}

void rmdir(const char* dir_name) {
    std::vector<std::string> components = split_path(dir_name);
    std::string path_so_far;
    Directory_Entry* current_entries = entries;
    Directory_Entry* parent_entries = nullptr;

    for (size_t i = 0; i < components.size() - 1; ++i) {
        if (!path_so_far.empty()) {
            path_so_far += "\\";
        }
        path_so_far += components[i];
        bool found = false;
        for (size_t j = 0; j < DIRECTORY_ENTRIES_PER_CLUSTER; ++j) {
            Directory_Entry& entry = current_entries[j];
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == components[i] && entry.attributes.is_directory) {
                parent_entries = current_entries;
                current_entries = reinterpret_cast<Directory_Entry*>(reinterpret_cast<uintptr_t>(entries) + entry.first_block_number * 1024); // Simulate directory block
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Error: Directory '%s' not found.\n", path_so_far.c_str());
            return;
        }
    }

    std::string target_dir_name = components.back();
    for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
        if (current_entries[i].filename != 0 && current_entries[i].attributes.is_directory) {
            std::string entry_name(reinterpret_cast<char*>(&current_entries[i].filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == target_dir_name) {
                // Check if the directory is empty
                bool is_empty = true;
                for (size_t j = 0; j < DIRECTORY_ENTRIES_PER_CLUSTER; ++j) {
                    if (current_entries[j].filename != 0 && current_entries[j].filename != 0xFFFFFFFFFFFFFFFF) {
                        is_empty = false;
                        break;
                    }
                }

                if (is_empty) {
                    memset(&current_entries[i], 0, sizeof(Directory_Entry));
                    printf("Directory '%s' removed successfully.\n", dir_name);
                    return;
                } else {
                    printf("Error: Directory '%s' is not empty.\n", dir_name);
                    return;
                }
            }
        }
    }
    printf("Error: Directory '%s' not found.\n", dir_name);
}

void initialize_directory_entries() {
    memset(entries, 0, sizeof(entries));
}

void create_file_system(const char* file_name, int block_size_kb) {
    FILE* fs = fopen(file_name, "wb");
    if (!fs) {
        perror("Error creating file system file");
        exit(EXIT_FAILURE);
    }

    // Initialize free blocks and directory entries
    initialize_free_blocks();
    initialize_directory_entries();

    // Write the superblock with the correct block size
    write_super_block(fs, block_size_kb);
    write_FAT(fs);
    write_root_directory(fs);

    // Set the file size to the maximum
    int total_blocks = 4096 * block_size_kb;
    fseek(fs, total_blocks * 1024 - 1, SEEK_SET);
    fputc('\0', fs);

    fclose(fs);
}

Directory_Entry* find_entry(const char* path) {
    std::vector<std::string> components = split_path(path);
    Directory_Entry* current_entries = entries;

    for (const std::string& component : components) {
        bool found = false;
        for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
            Directory_Entry& entry = current_entries[i];
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == component) {
                if (entry.attributes.is_directory) {
                    current_entries = reinterpret_cast<Directory_Entry*>(reinterpret_cast<uintptr_t>(entries) + entry.first_block_number * 1024); // Simulate directory block
                }
                found = true;
                break;
            }
        }
        if (!found) {
            return nullptr;
        }
    }

    return current_entries;
}

void write_file(const char* file_path, const char* linux_file) {
    FILE* src = fopen(linux_file, "rb");
    if (!src) {
        perror("Error opening source file");
        return;
    }

    fseek(src, 0, SEEK_END);
    uint32_t file_size = ftell(src);
    fseek(src, 0, SEEK_SET);

    std::vector<std::string> components = split_path(file_path);
    Directory_Entry* current_entries = entries;

    for (size_t i = 0; i < components.size() - 1; ++i) {
        bool found = false;
        for (size_t j = 0; j < DIRECTORY_ENTRIES_PER_CLUSTER; ++j) {
            Directory_Entry& entry = current_entries[j];
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == components[i] && entry.attributes.is_directory) {
                current_entries = reinterpret_cast<Directory_Entry*>(reinterpret_cast<uintptr_t>(entries) + entry.first_block_number * 1024); // Simulate directory block
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Error: Parent directory '%s' does not exist.\n", components[i].c_str());
            return;
        }
    }

    std::string new_file_name = components.back();
    for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
        if (current_entries[i].filename == 0 || current_entries[i].filename == 0xFFFFFFFFFFFFFFFF) {
            initialize_directory_entry(&current_entries[i], new_file_name.c_str(), "", file_size, 0, NULL);

            uint16_t cluster = 2;
            while (file_size > 0) {
                uint8_t buffer[1024];
                size_t read_size = fread(buffer, 1, sizeof(buffer), src);
                if (read_size > 0) {
                    file_size -= read_size;
                }
                cluster++;
            }
            printf("File '%s' written successfully.\n", file_path);
            fclose(src);
            return;
        }
    }
    printf("Error: No empty directory entries available.\n");
}

void read_file(const char* file_path, const char* linux_file) {
    Directory_Entry* entry = find_entry(file_path);
    if (!entry) {
        printf("Error: File '%s' not found.\n", file_path);
        return;
    }

    FILE* dest = fopen(linux_file, "wb");
    if (!dest) {
        perror("Error opening destination file");
        return;
    }

    uint32_t file_size = entry->file_size;
    uint16_t cluster = entry->first_block_number;

    while (file_size > 0) {
        uint8_t buffer[1024];
        size_t write_size = file_size > sizeof(buffer) ? sizeof(buffer) : file_size;
        fwrite(buffer, 1, write_size, dest);
        file_size -= write_size;
        cluster++;
    }
    fclose(dest);
    printf("File '%s' read successfully.\n", file_path);
}

void delete_file(const char* file_path) {
    std::vector<std::string> components = split_path(file_path);
    Directory_Entry* current_entries = entries;
    Directory_Entry* parent_entries = nullptr;

    for (size_t i = 0; i < components.size() - 1; ++i) {
        bool found = false;
        for (size_t j = 0; j < DIRECTORY_ENTRIES_PER_CLUSTER; ++j) {
            Directory_Entry& entry = current_entries[j];
            std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
            entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
            if (entry_name == components[i] && entry.attributes.is_directory) {
                parent_entries = current_entries;
                current_entries = reinterpret_cast<Directory_Entry*>(reinterpret_cast<uintptr_t>(entries) + entry.first_block_number * 1024); // Simulate directory block
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Error: Parent directory '%s' does not exist.\n", components[i].c_str());
            return;
        }
    }

    std::string target_file_name = components.back();
    for (size_t i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; ++i) {
        Directory_Entry& entry = current_entries[i];
        std::string entry_name(reinterpret_cast<char*>(&entry.filename), 8);
        entry_name = entry_name.substr(0, entry_name.find('\0')); // Remove trailing nulls
        if (entry_name == target_file_name && !entry.attributes.is_directory) {
            // Clear FAT entries
            uint16_t cluster = entry.first_block_number;
            while (cluster < 0xFF8) { // Continue until end of cluster chain
                uint16_t next_cluster = get_fat12_entry(cluster);
                set_fat12_entry(cluster, 0x000); // Mark cluster as free
                cluster = next_cluster;
            }

            // Clear directory entry
            memset(&entry, 0, sizeof(Directory_Entry));
            printf("File '%s' deleted successfully.\n", file_path);
            return;
        }
    }
    printf("Error: File '%s' not found.\n", file_path);
}

void dumpe2fs() {
    printf("Filesystem information:\n");
    printf("Total clusters: %d\n", TOTAL_CLUSTERS);
    printf("FAT12 size: %d bytes\n", FAT12_SIZE);
    printf("Directory entries per cluster: %lu\n", DIRECTORY_ENTRIES_PER_CLUSTER);
}

void chmod_file(const char* file_path, const char* permissions) {
    Directory_Entry* entry = find_entry(file_path);
    if (!entry) {
        printf("Error: File '%s' not found.\n", file_path);
        return;
    }

    if (strcmp(permissions, "-rw") == 0) {
        entry->attributes.read_permission = 1;
        entry->attributes.write_permission = 0;
    } else if (strcmp(permissions, "+rw") == 0) {
        entry->attributes.read_permission = 1;
        entry->attributes.write_permission = 1;
    } else {
        printf("Error: Invalid permissions '%s'.\n", permissions);
        return;
    }

    printf("Permissions for '%s' set to '%s'.\n", file_path, permissions);
}

void add_password(const char* file_path, const char* password) {
    Directory_Entry* entry = find_entry(file_path);
    if (!entry) {
        printf("Error: File '%s' not found.\n", file_path);
        return;
    }

    strncpy(entry->password, password, 16);
    entry->attributes.is_protected = 1;
    printf("Password for '%s' set successfully.\n", file_path);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <block_size_kb> <file_system_name> [operation] [parameters...]\n", argv[0]);
        return 1;
    }

    int block_size_kb = atoi(argv[1]);
    if (block_size_kb != 0.5 && block_size_kb != 1) {
        fprintf(stderr, "Block size must be 0.5 or 1 KB.\n");
        return 1;
    }

    const char* file_system_name = argv[2];

    if (argc == 3) {
        create_file_system(file_system_name, block_size_kb);
        printf("File system created successfully.\n");
    } else if (argc >= 4) {
        const char* operation = argv[3];
        if (strcmp(operation, "dir") == 0 && argc == 5) {
            FILE* fs = fopen(file_system_name, "rb");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
            dir(argv[4]);
        } else if (strcmp(operation, "mkdir") == 0 && argc == 5) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            mkdir(argv[4]);
            fseek(fs, FAT12_SIZE, SEEK_SET);
            fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
        } else if (strcmp(operation, "rmdir") == 0 && argc == 5) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            rmdir(argv[4]);
            fseek(fs, FAT12_SIZE, SEEK_SET);
            fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
        } else if (strcmp(operation, "write") == 0 && argc == 6) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            write_file(argv[4], argv[5]);
            fseek(fs, FAT12_SIZE, SEEK_SET);
            fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
        } else if (strcmp(operation, "read") == 0 && argc == 6) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            read_file(argv[4], argv[5]);
            fclose(fs);
        } else if (strcmp(operation, "del") == 0 && argc == 5) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            delete_file(argv[4]);
            fseek(fs, FAT12_SIZE, SEEK_SET);
            fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
        } else if (strcmp(operation, "dumpe2fs") == 0 && argc == 4) {
            FILE* fs = fopen(file_system_name, "rb");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            dumpe2fs();
            fclose(fs);
        } else if (strcmp(operation, "chmod") == 0 && argc == 6) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            chmod_file(argv[4], argv[5]);
            fseek(fs, FAT12_SIZE, SEEK_SET);
            fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
        } else if (strcmp(operation, "addpw") == 0 && argc == 6) {
            FILE* fs = fopen(file_system_name, "rb+");
            if (!fs) {
                perror("Error opening file system file");
                return 1;
            }
            fread(fat12_table, FAT12_SIZE, 1, fs);
            fread(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            add_password(argv[4], argv[5]);
            fseek(fs, FAT12_SIZE, SEEK_SET);
            fwrite(entries, sizeof(Directory_Entry), DIRECTORY_ENTRIES_PER_CLUSTER, fs);
            fclose(fs);
        } else {
            fprintf(stderr, "Unknown or incorrect usage of operation: %s\n", operation);
        }
    }

    return 0;
}
