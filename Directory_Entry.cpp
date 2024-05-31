#include <string>
#include <ctime>

using namespace std;

class Directory_Entry 
{
    public:
        //Default Constructor
        Directory_Entry() : file_name(""), 
                            file_size(0), 
                            read_permission(false), 
                            write_permission(false), 
                            is_protected(false), 
                            creation_time(0), 
                            modification_time(0) {}

        //Parameterized Constructor
        Directory_Entry(string file_name, 
                        uint32_t file_size, 
                        bool read_permission, 
                        bool write_permission, 
                        bool is_protected, 
                        time_t creation_time, 
                        time_t modification_time) : file_name(file_name), 
                                                    file_size(file_size), 
                                                    read_permission(read_permission), 
                                                    write_permission(write_permission), 
                                                    is_protected(is_protected), 
                                                    creation_time(creation_time), 
                                                    modification_time(modification_time) {}

        //Getters
        string get_file_name() { return file_name; }
        uint32_t get_file_size() { return file_size; }
        bool get_read_permission() { return read_permission; }
        bool get_write_permission() { return write_permission; }
        bool get_is_protected() { return is_protected; }
        time_t get_creation_time() { return creation_time; }
        time_t get_modification_time() { return modification_time; }
        string get_password() { return password; }
        

        //Setters
        void set_file_name(string file_name) { this->file_name = file_name; }
        void set_file_size(uint32_t file_size) { this->file_size = file_size; }
        void set_read_permission(bool read_permission) { this->read_permission = read_permission; }
        void set_write_permission(bool write_permission) { this->write_permission = write_permission; }
        void set_is_protected(bool is_protected) { this->is_protected = is_protected; }
        void set_creation_time(time_t creation_time) { this->creation_time = creation_time; }
        void set_modification_time(time_t modification_time) { this->modification_time = modification_time; }
        void set_password(string password) { this->password = password; }

    private:
        string file_name;
        string password;
        uint32_t file_size;
        bool read_permission;
        bool write_permission;
        bool is_protected;
        time_t creation_time;
        time_t modification_time;
        uint16_t start_block;
};

class Directory_Entry_Table
{
    public:
        //Default Constructor
        Directory_Entry_Table() : table_size(0), table_capacity(0), table(nullptr) {}

        //Parameterized Constructor
        Directory_Entry_Table(uint32_t table_capacity) : table_size(0), table_capacity(table_capacity), table(new Directory_Entry[table_capacity]) {}

        //Destructor
        ~Directory_Entry_Table() { delete[] table; }

        //Getters
        uint32_t get_table_size() { return table_size; }
        uint32_t get_table_capacity() { return table_capacity; }
        Directory_Entry* get_table() { return table; }

        //Setters
        void set_table_size(uint32_t table_size) { this->table_size = table_size; }
        void set_table_capacity(uint32_t table_capacity) { this->table_capacity = table_capacity; }
        void set_table(Directory_Entry* table) { this->table = table; }

        //Add Entry
        void add_entry(Directory_Entry entry)
        {
            if (table_size < table_capacity)
            {
                table[table_size] = entry;
                table_size++;
            }
        }

        //Remove Entry
        void remove_entry(uint32_t index)
        {
            if (index < table_size)
            {
                for (uint32_t i = index; i < table_size - 1; i++)
                {
                    table[i] = table[i + 1];
                }
                table_size--;
            }
        }

        //Get Entry
        Directory_Entry get_entry(uint32_t index)
        {
            if (index < table_size)
            {
                return table[index];
            }
            return Directory_Entry();
        }

    private:
        uint32_t table_size;
        uint32_t table_capacity;
        Directory_Entry* table;
}