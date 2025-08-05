#ifndef PROGRAM_CONTEXT_H
#define PROGRAM_CONTEXT_H

#include "pch.h"

struct program_context {
private:
	std::size_t references_count{ 1 };
	std::mutex references_mutex{};

	program_context(
		std::uint64_t preferred_stack_size,
		void** code, std::uint32_t functions_count, 
		void** exposed_functions, std::uint32_t exposed_functions_count, 
		void* jump_table, std::uint64_t jump_table_size,
		void** program_strings, std::uint64_t program_strings_size
	)
		:preferred_stack_size{ preferred_stack_size },
		code{ code },
		functions_count{ functions_count },
		exposed_functions{ exposed_functions },
		exposed_functions_count{ exposed_functions_count },
		jump_table{ jump_table },
		jump_table_size{ jump_table_size },
		program_strings{ program_strings },
		program_strings_size{ program_strings_size }
	{}

public:
	std::uint64_t preferred_stack_size{};

	void** code{};
	std::uint32_t functions_count{};

	void** exposed_functions{};
	std::uint32_t exposed_functions_count{};

	void* jump_table{};
	std::uint64_t jump_table_size{};

	void** program_strings{};
	std::uint64_t program_strings_size{};

	static program_context* create(
		std::uint64_t preferred_stack_size,
		void** code, std::uint32_t functions_count, 
		void** exposed_functions, std::uint32_t exposed_functions_count, 
		void* jump_table, std::uint64_t jump_table_size,
		void** program_strings, std::uint64_t program_strings_size
	) {
		return new program_context{ 
			preferred_stack_size,
			code, functions_count, 
			exposed_functions, exposed_functions_count, 
			jump_table, jump_table_size,
			program_strings, program_strings_size
		};
	}

    program_context(program_context&&) = delete;
    program_context& operator=(program_context&&) = delete;

    program_context(const program_context&) = delete;
    program_context& operator=(const program_context&) = delete;

	static program_context* duplicate(program_context* object) {
		std::lock_guard lock{ object->references_mutex };
		assert(object->references_count != 0);

		++object->references_count;
		return object;
	}

	std::size_t decrease_references_count() {
		std::lock_guard lock{ this->references_mutex };
		return --this->references_count;
	}

	~program_context() noexcept;
};

#endif // !PROGRAM_CONTEXT_H