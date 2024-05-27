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

        //Setters
        void set_file_name(string file_name) { this->file_name = file_name; }
        void set_file_size(uint32_t file_size) { this->file_size = file_size; }
        void set_read_permission(bool read_permission) { this->read_permission = read_permission; }
        void set_write_permission(bool write_permission) { this->write_permission = write_permission; }
        void set_is_protected(bool is_protected) { this->is_protected = is_protected; }
        void set_creation_time(time_t creation_time) { this->creation_time = creation_time; }
        void set_modification_time(time_t modification_time) { this->modification_time = modification_time; }

    private:
        string file_name;
        uint32_t file_size;
        bool read_permission;
        bool write_permission;
        bool is_protected;
        time_t creation_time;
        time_t modification_time;
};