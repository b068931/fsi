#ifndef DATA_READER
#define DATA_READER

#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <cassert>
#include <filesystem>

/// <summary>
/// -valid comment
/// -Created to read .data files. Space symbols are ignored, unless they are a part of "header" or "entry" expression. Last line must end with new line character
/// - this line creates "header" object
/// [header]
/// - this line creates "entry" object
/// name=value; -this is a comment
/// - headers can have 0 entries in them
/// [header2]
/// - error
/// =value;
/// - valid entry
/// name=;
/// - valid header, but it will be ignored
/// []
/// </summary>
class data_reader {
public:
	enum class errors : unsigned int {
		no_errors,
		unexpected_symbol_inside_header,
		unexpected_symbol_inside_value,
		unexpected_symbol,
		unexpected_symbol_at_start,
		file_error
	};

private:
	//this class is used to reduce amount of calls to std::ifstream
	template<size_t block_size>
	class block_reader {
	private:
		size_t current_loaded_block;
		char* block;

		std::uintmax_t file_size;
		std::ifstream* file_stream;
		void load_block_to_memory(size_t block_index) {
			size_t read_size = block_size;
			size_t expected_size = block_index * block_size + block_size;
			if (expected_size > this->file_size) {
				read_size = this->file_size - (block_index * block_size);
			}

			this->file_stream->seekg(block_index * block_size, std::ios::beg); //seek to the start of the block and read it
			this->file_stream->read(this->block, read_size);
			
			this->current_loaded_block = block_index;
		}
	public:
		block_reader()
			:current_loaded_block{0},
			block{new char[block_size]},
			file_stream{nullptr},
			file_size{}
		{}

		block_reader(nullptr_t)
			:block_reader{}
		{}


		block_reader(std::ifstream* file_stream, std::uintmax_t file_size)
			:block_reader{}
		{
			this->set_file_stream(file_stream);
		}

		void set_file_stream(nullptr_t) { this->file_stream = nullptr; }
		void set_file_stream(std::ifstream* file_stream, std::uintmax_t file_size) { 
			this->file_stream = file_stream;
			this->file_size = file_size;
			this->load_block_to_memory(0);
		}

		std::uintmax_t get_symbols_count() const { return this->file_size; }
		char get_symbol(size_t index) {
			if ((index / block_size) != this->current_loaded_block) {
				this->load_block_to_memory(index);
			}

			return this->block[index % block_size];
		}

		~block_reader() {
			delete this->block;
		}
	};

public:
	class header;
	class entry {
	private:
		char* name;
		size_t name_length;

		char* value;
		size_t value_length;

		void write_to_file(std::ofstream& file_stream) const {
			file_stream.write(this->name, this->name_length);
			file_stream << '=';
			
			file_stream.write(this->value, this->value_length);
			file_stream << '\n';
		}
	public:
		entry()
			:name{nullptr},
			value{nullptr},
			name_length{0},
			value_length{0}
		{}
		entry(char* name_to_set, size_t name_length, char* value_to_set, size_t value_length)
			:name{name_to_set},
			name_length{name_length},
			value{value_to_set},
			value_length{value_length}
		{}
		entry(const entry& original)
			:entry{}
		{
			this->copy_name(original.get_name(), original.get_name_length());
			this->copy_value(original.get_value(), original.get_value_length());
		}
		entry(entry&& old_object) noexcept
			:name{ old_object.name },
			name_length{ old_object.name_length },
			value{ old_object.value },
			value_length{ old_object.value_length }
		{
			old_object.name = nullptr;
			old_object.name_length = 0;
			old_object.value = nullptr;
			old_object.value_length = 0;
		}

		void operator= (const entry& entry) {
			assert(&entry != this && "self assignment");
			this->free_resources();

			this->copy_name(entry.get_name(), entry.get_name_length());
			this->copy_value(entry.get_value(), entry.get_value_length());
		}
		void operator= (entry&& old_entry) noexcept {
			assert(&old_entry != this && "self assignment");
			this->free_resources();

			this->name = old_entry.name;
			this->name_length = old_entry.name_length;

			this->value = old_entry.value;
			this->value_length = old_entry.value_length;

			old_entry.name = nullptr;
			old_entry.name_length = 0;

			old_entry.value = nullptr;
			old_entry.value_length = 0;
		}

		const char* get_name() const { return this->name; }
		size_t get_name_length() const { return this->name_length; }

		const char* get_value() const { return this->value; }
		size_t get_value_length() const { return this->value_length; }
		
		void copy_name(const char* new_name, size_t name_length) {
			this->delete_name();
			
			char* new_name_copy = new char[name_length] {};
			std::memcpy(new_name_copy, new_name, name_length);

			this->name = new_name_copy;
			this->name_length = name_length;
		}
		void copy_value(const char* new_value, size_t value_length) {
			this->delete_value();

			char* new_value_copy = new char[value_length] {};
			std::memcpy(new_value_copy, new_value, value_length);

			this->value = new_value_copy;
			this->value_length = value_length;
		}

		void set_value(char* new_value, size_t new_value_length) { 
			this->delete_value();

			this->value = new_value;
			this->value_length = new_value_length;
		}
		void set_name(char* new_name, size_t new_name_length) {
			this->delete_name();

			this->name = new_name;
			this->name_length = new_name_length;
		}

		void free_resources() {
			this->delete_name();
			this->delete_value();
		}
		void delete_name() {
			delete this->name;

			this->name = nullptr;
			this->name_length = 0;
		}
		void delete_value() {
			delete this->value;

			this->value = nullptr;
			this->value_length = 0;
		}

		friend class header;

		~entry() {
			this->free_resources();
		}
	};
	class header {
	private:
		char* name;
		size_t name_length;

		std::vector<entry> entries;
		void copy_name(const header& header) {
			this->name = new char[header.name_length];
			std::memcpy(this->name, header.name, header.name_length);

			this->name_length = header.name_length;
		}
		void move_name(header&& old_header) noexcept {
			this->name = old_header.name;
			this->name_length = old_header.name_length;

			old_header.name = nullptr;
			old_header.name_length = 0;
		}

		void write_to_file(std::ofstream& file_stream) const {
			file_stream << '[';
			file_stream.write(this->name, this->name_length);
			file_stream << "]\n";

			for (const entry& entry : this->entries) {
				entry.write_to_file(file_stream);
			}
		}
	public:
		header()
			:name{nullptr},
			name_length{ 0 },
			entries{}
		{}

		header(char* name, size_t name_length)
			:header{}
		{
			this->set_name(name, name_length);
		}

		header(const header& header)
			:entries{ header.entries }
		{
			this->copy_name(header);
		}

		header(header&& old_header) noexcept
			:entries{ std::move(old_header.entries) }
		{
			this->move_name(std::move(old_header));
		}

		void operator= (const header& header) {
			assert(&header != this && "self assignment");
			this->delete_name();
			
			this->entries = header.entries;
			this->copy_name(header);
		}
		void operator= (header&& old_header) noexcept {
			assert(&old_header != this && "self assignment");
			this->delete_name();

			this->entries = std::move(old_header.entries);
			this->move_name(std::move(old_header));
		}

		const char* get_name() const { return this->name; }
		size_t get_name_length() const { return this->name_length; }

		void set_name(char* new_name, size_t new_name_length) {
			this->delete_name();

			this->name = new_name;
			this->name_length = new_name_length;
		}
		void copy_name(const char* new_name, size_t name_length) {
			this->delete_name();

			char* new_name_copy = new char[name_length] {};
			std::memcpy(new_name_copy, new_name, name_length);

			this->name = new_name_copy;
			this->name_length = name_length;
		}

		void emplace_entry(char* name_to_set, size_t name_length, char* value_to_set, size_t value_length) { this->entries.emplace_back(name_to_set, name_length, value_to_set, value_length); }
		entry& back() { return this->entries.back(); }
		const entry& back() const { return this->entries.back(); }

		void add_entry(const entry& entry_to_add) { this->entries.push_back(entry_to_add); }
		void add_entry(entry&& entry_to_add) { this->entries.push_back(std::move(entry_to_add)); }

		entry& get_entry(size_t index) { return this->entries[index]; }
		const entry& get_entry(size_t index) const { return this->entries[index]; }
		void delte_entry(size_t index) { this->entries.erase(this->entries.begin() + index); }

		auto begin() { return this->entries.begin(); }
		auto end() { return this->entries.end(); }
		auto cbegin() const { return this->entries.cbegin(); }
		auto cend() const { return this->entries.cend(); }

		void delete_entries() {
			this->entries.resize(0);
		}
		void delete_name() {
			delete this->name;

			this->name = nullptr;
			this->name_length = 0;
		}
		void free_resources() {
			this->delete_entries();
			this->delete_name();
		}

		friend class data_reader;

		~header() {
			this->delete_name();
		}
	};

private:
	mutable errors error_state;

	std::vector<header> headers;
	block_reader<1024> reader;
	size_t last_point;
	errors (data_reader::* current_state)(size_t);

	size_t get_last_string_size(size_t index) { return index - this->last_point; }
	std::pair<char*, size_t> get_last_string(size_t index) { 
		++this->last_point;

		size_t last_string_size = this->get_last_string_size(index);
		char* last_string = new char[last_string_size] {};

		for (size_t index_in_last_string = 0; this->last_point < index; ++index_in_last_string, ++this->last_point) {
			assert(index_in_last_string < last_string_size);
			last_string[index_in_last_string] = this->reader.get_symbol(this->last_point);
		}

		return { last_string, last_string_size };
	}

	//alphabet, numbers, _
	bool is_string_character(char symbol) { return std::isalnum(static_cast<unsigned char>(symbol)) || (symbol == '_'); }

	errors inside_comment(size_t index) {
		char next_symbol = this->reader.get_symbol(index);
		if (next_symbol == '\n') { //comment lasts until a new string symbol is encountered
			this->current_state = &data_reader::base_state;
			this->last_point = index; //comments are simply ignored
		}

		return errors::no_errors;
	}
	errors inside_header(size_t index) {
		char next_symbol = this->reader.get_symbol(index);
		if ((next_symbol != ' ') && (next_symbol != '\n') && !this->is_string_character(next_symbol) && (next_symbol != '\r')) { //space symbols, new lines and string characters are ignored
			if (next_symbol == ']') {
				if ((index - this->last_point) > 1) { //header must not be empty
					auto last_string = this->get_last_string(index);
					this->headers.emplace_back(last_string.first, last_string.second);
				}

				this->current_state = &data_reader::base_state;
				return errors::no_errors;
			}

			return errors::unexpected_symbol_inside_header;
		}

		return errors::no_errors;
	}
	errors inside_value(size_t index) {
		char next_symbol = this->reader.get_symbol(index);
		if ((next_symbol != ' ') && !this->is_string_character(next_symbol) && (next_symbol != '\r')) {
			if (next_symbol == '\n') {
				if ((index - this->last_point) > 1) { //values with empty values are valid entries
					auto last_string = this->get_last_string(index);
					this->headers.back().back().set_value(last_string.first, last_string.second);
				}

				this->current_state = &data_reader::base_state;
				return errors::no_errors;
			}

			return errors::unexpected_symbol_inside_value;
		}

		return errors::no_errors;
	}
	errors base_state(size_t index) {
		char next_symbol = this->reader.get_symbol(index);
		if ((next_symbol != ' ') && (next_symbol != '\n') && !this->is_string_character(next_symbol) && (next_symbol != '\r')) {
			if (next_symbol == '=') {
				if ((index - this->last_point) > 1) { //values with empty names are considered as errors
					entry new_entry{};
					auto last_string = this->get_last_string(index);

					new_entry.set_name(last_string.first, last_string.second);

					this->headers.back().add_entry(std::move(new_entry)); //move semantics are used here
					this->current_state = &data_reader::inside_value;
					return errors::no_errors;
				}
			}
			else if (next_symbol == '[') {
				this->last_point = index;
				this->current_state = &data_reader::inside_header;
				return errors::no_errors;
			}
			else if (next_symbol == '-') {
				this->current_state = &data_reader::inside_comment;
				return errors::no_errors;
			}

			return errors::unexpected_symbol;
		}

		return errors::no_errors;
	}
	errors start_state(size_t index) { //initially program expects only headers
		char next_symbol = this->reader.get_symbol(index);
		if ((next_symbol != ' ') && (next_symbol != '\n') && !this->is_string_character(next_symbol) && (next_symbol != '\r')) {
			if (next_symbol == '[') {
				this->current_state = &data_reader::inside_header;
				this->last_point = index;
				return errors::no_errors;
			}

			return errors::unexpected_symbol_at_start; //if reached to this point then file structure is insufficient
		}

		return errors::no_errors;
	}
	
	void clean_object() {
		this->headers.resize(0);
		this->current_state = &data_reader::start_state;
		this->reader.set_file_stream(nullptr);
		this->last_point = 0;
	}
public:
	data_reader()
		:headers{},
		current_state{&data_reader::start_state},
		reader{},
		last_point{0},
		error_state{errors::no_errors}
	{}

	//Loads new file. All previous headers are discarded
	void load_new_file(const std::string& file_name) {
		std::ifstream file_stream{ file_name, std::ios::binary | std::ios::in };
		this->clean_object();

		if (!file_stream.good()) { this->error_state = errors::file_error; return; }

		this->reader.set_file_stream(&file_stream, std::filesystem::file_size(file_name));
		for (size_t index = 0, size = this->reader.get_symbols_count(); index < size; ++index) {
			errors error = (this->*this->current_state)(index); //calls a function address to which is stored inside a pointer (nothing interesting but syntax is ugly)
			if (error != errors::no_errors) {
				this->error_state = error;
				this->clean_object();
				return;
			}
		}
	}
	void write_to_file(const std::string& file_name) const {
		std::ofstream file_stream{ file_name };
		if (!file_stream.good()) { this->error_state = errors::file_error; return; }

		for (const header& header : this->headers) {
			header.write_to_file(file_stream);
		}
	}

	errors get_error() const { return this->error_state; }
	std::vector<header>& get_headers() { return this->headers; }
};

#endif