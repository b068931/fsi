#ifndef BLOCK_READER_H
#define BLOCK_READER_H

#include <fstream>
#include <cstdint>

//this type is used to represent position in a file
using filepos = std::uintmax_t;

//this class is used to reduce amount of calls to std::ifstream
template<filepos block_size>
class block_reader {
private:
	filepos current_loaded_block;
	char* block;

	filepos file_size;
	std::ifstream* file_stream;

	void load_block_to_memory(filepos block_index) {
		assert(this->file_stream && "file stream was not set");

		filepos read_size = block_size;
		filepos expected_size = block_index * block_size + block_size;
		if (expected_size > this->file_size) {
			read_size = this->file_size - (block_index * block_size);
		}

		this->file_stream->seekg(block_index * block_size, std::ios::beg); //seek to the start of the block and read it
		this->file_stream->read(this->block, read_size);

		this->current_loaded_block = block_index;
	}
	void delete_file_stream() { delete this->file_stream; }
	void delete_block() { delete this->block; }
public:
	block_reader()
		:current_loaded_block{ 0 },
		block{ new char[block_size] },
		file_stream{ nullptr },
		file_size{}
	{}

	block_reader(nullptr_t)
		:block_reader{}
	{}

	block_reader(std::ifstream* file_stream, filepos file_size)
		:block_reader{}
	{
		this->set_file_stream(file_stream, file_size);
	}

	bool check_file_stream() const { return this->file_stream; }

	void set_file_stream(nullptr_t) { this->file_stream = nullptr; }
	void set_file_stream(std::ifstream* file_stream, filepos file_size) {
		this->delete_file_stream();

		this->file_stream = file_stream;
		this->file_size = file_size;

		this->load_block_to_memory(0);
	}

	filepos get_symbols_count() const { return this->file_size; }
	char get_symbol(filepos index) {
		if (index < this->file_size) {
			filepos block_index = (index / block_size);
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

#endif // !BLOCK_READER_H