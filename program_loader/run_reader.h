#ifndef RUN_READER_H
#define RUN_READER_H

#include <stdint.h>
#include <map>
#include <iostream>
#include "../dll_mediator/block_reader.h"

template<typename container>
class run_reader {
public:
	class run { //used to access bytes ONLY from specific run
	private:
		filepos run_start;
		filepos run_position;
		filepos run_size;

		std::shared_ptr<block_reader<1024>> reader; //we will use only one block_reader for all runs
	public:
		run(filepos start, filepos size, std::shared_ptr<block_reader<1024>> reader)
			:run_start{ start },
			run_size{ size },
			reader{ reader },
			run_position{ 0 }
		{}

		run(const run&) = delete;
		void operator=(const run&) = delete;

		run(run&&) = default;
		run& operator=(run&&) = default;

		char get_symbol() {
			if (this->run_position < this->run_size) {
				return this->reader->get_symbol(this->run_start + (this->run_position++));
			}

			return '\0';
		}

		filepos get_run_size() const { return this->run_size; }
		filepos get_run_position() const { return this->run_position; }

		template<typename type>
		type get_object(type value = type{}) {
			char* bytes = reinterpret_cast<char*>(&value);
			for (filepos count = 0; (count < sizeof(type)) && (this->run_position < this->run_size);
				++count, ++this->run_position) {
				bytes[count] = this->reader->get_symbol(this->run_start + this->run_position);
			}

			return value;
		}
	};
	using container_run_initialize_function = void (container::*)(run);

	run_reader(const std::string& file_name, container* cont, std::map<char, container_run_initialize_function> run_initializers) {
		std::shared_ptr<block_reader<1024>> reader;

		reader = std::shared_ptr<block_reader<1024>>{
			new block_reader<1024>{
				new std::ifstream{file_name, std::ios::binary},
				std::filesystem::file_size(file_name)
			}
		};
		
		for (filepos index = 0, length = reader->get_symbols_count(); (index < length);) {
			char run_type = reader->get_symbol(index++);

			uint64_t run_size = 0;
			char* run_size_bytes = reinterpret_cast<char*>(&run_size);
			for (size_t count = 0; count < sizeof(uint64_t); ++count) {
				run_size_bytes[count] = reader->get_symbol(index++);
			}

			auto found_run = run_initializers.find(run_type); //check if container recognizes this type of run
			if (found_run != run_initializers.end()) { //if not then we simply ignore this run
				(cont->*(found_run->second))(run{ index, run_size, reader });
			}

			index += run_size;
		}
	}
};

#endif // !RUN_READER_H=