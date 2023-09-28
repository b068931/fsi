#ifndef PRGORAM_CONTEXT_H
#define PROGRAM_CONTEXT_H

#include "pch.h"


struct program_context {
private:
	size_t references_count{ 1 };
	std::mutex references_mutex{};

	program_context(
		void** code, uint32_t functions_count, 
		void** exposed_functions, uint32_t exposed_functions_count, 
		void* jump_table, uint64_t jump_table_size,
		void** program_strings, uint64_t program_strings_size
	)
		:code{ code },
		functions_count{ functions_count },
		exposed_functions{ exposed_functions },
		exposed_functions_count{ exposed_functions_count },
		jump_table{ jump_table },
		jump_table_size{ jump_table_size },
		program_strings{ program_strings },
		program_strings_size{ program_strings_size }
	{}

public:
	void** code{};
	uint32_t functions_count{};

	void** exposed_functions{};
	uint32_t exposed_functions_count{};

	void* jump_table{};
	uint64_t jump_table_size{};

	void** program_strings{};
	uint64_t program_strings_size{};

	static program_context* create(
		void** code, uint32_t functions_count, 
		void** exposed_functions, uint32_t exposed_functions_count, 
		void* jump_table, uint64_t jump_table_size,
		void** program_strings, uint64_t program_strings_size
	) {
		return new program_context{ 
			code, functions_count, 
			exposed_functions, exposed_functions_count, 
			jump_table, jump_table_size,
			program_strings, program_strings_size
		};
	}
	static program_context* duplicate(program_context* object) {
		std::lock_guard lock{ object->references_mutex };
		assert(object->references_count != 0);

		++object->references_count;
		return object;
	}
	size_t decrease_references_count() {
		std::lock_guard lock{ this->references_mutex };
		return --this->references_count;
	}

	~program_context() noexcept;
};

#endif // !PRGORAM_CONTEXT_H