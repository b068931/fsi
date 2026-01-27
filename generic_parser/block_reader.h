#ifndef BLOCK_READER_H
#define BLOCK_READER_H

#include <fstream>
#include <cstdint>

namespace generic_parser {
    //this type is used to represent position in a file
    using file_position_type = std::uintmax_t;

    //this class is used to reduce amount of calls to std::ifstream
    template<file_position_type block_size>
    class block_reader {
    private:
        file_position_type current_loaded_block;
        char* block;

        file_position_type file_size;
        std::ifstream* file_stream;

        void load_block_to_memory(file_position_type block_index) {
            assert(this->file_stream && "file stream was not set");

            file_position_type read_size = block_size;
            file_position_type expected_size = block_index * block_size + block_size;
            if (expected_size > this->file_size) {
                read_size = this->file_size - block_index * block_size;
            }

            //seek to the start of the block and read it
            this->file_stream->seekg(
                static_cast<std::streamoff>(block_index * block_size), 
                std::ios::beg
            );

            this->file_stream->read(
                this->block, 
                static_cast<std::streamsize>(read_size)
            );

            this->current_loaded_block = block_index;
        }

        void delete_file_stream() noexcept {
            delete this->file_stream;
            this->file_stream = nullptr;
        }

        void delete_block() noexcept {
            delete[] this->block;
            this->block = nullptr;
        }
    public:
        block_reader()
            :current_loaded_block{ 0 },
            block{ new char[block_size] },
            file_size{},
            file_stream{ nullptr }
        {}

        block_reader(nullptr_t)
            :block_reader{}
        {}

        block_reader(std::ifstream* stream, file_position_type size)
            :block_reader{}
        {
            this->set_file_stream(stream, size);
        }

        block_reader(const block_reader&) = delete;
        block_reader& operator=(const block_reader&) = delete;

        block_reader(block_reader&& other) noexcept
            :current_loaded_block{ other.current_loaded_block },
            block{ other.block },
            file_size{ other.file_size },
            file_stream{ other.file_stream }
        {
            other.block = nullptr;
            other.file_stream = nullptr;
            other.current_loaded_block = 0;
            other.file_size = 0;
        }

        block_reader& operator=(block_reader&& other) noexcept {
            if (this != &other) {
                this->delete_file_stream();
                this->delete_block();

                this->current_loaded_block = other.current_loaded_block;
                this->block = other.block;
                this->file_size = other.file_size;
                this->file_stream = other.file_stream;

                other.block = nullptr;
                other.file_stream = nullptr;
                other.current_loaded_block = 0;
                other.file_size = 0;
            }

            return *this;
        }

        bool check_file_stream() const { return this->file_stream; }

        void set_file_stream(nullptr_t) { this->file_stream = nullptr; }
        void set_file_stream(std::ifstream* stream, file_position_type size) {
            this->delete_file_stream();

            this->file_stream = stream;
            this->file_size = size;

            this->load_block_to_memory(0);
        }

        file_position_type get_symbols_count() const { return this->file_size; }
        char get_symbol(file_position_type index) {
            if (index < this->file_size) {
                file_position_type block_index = index / block_size;
                if (block_index != this->current_loaded_block) {
                    this->load_block_to_memory(block_index);
                }

                return this->block[index % block_size];
            }

            return '\0'; //if index is too big, then we simply return \0 symbol
        }

        ~block_reader() {
            this->delete_file_stream();
            this->delete_block();
        }
    };
}

#endif // !BLOCK_READER_H
