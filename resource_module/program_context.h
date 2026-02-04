#ifndef PROGRAM_CONTEXT_H
#define PROGRAM_CONTEXT_H

#include "pch.h"

struct program_context {
private:
    std::size_t references_count{ 1 };
    std::mutex references_mutex{};

    program_context(
        void* application_image_base, void* application_runtime_function_entries,
        std::uint64_t program_preferred_stack_size,
        void** program_code, std::uint32_t program_functions_count, 
        void** program_exposed_functions, std::uint32_t program_exposed_functions_count, 
        void* program_jump_table, std::uint64_t program_jump_table_size,
        void** program_strings, std::uint64_t program_strings_size
    )
        :image_base{ application_image_base },
        runtime_function_entries{ application_runtime_function_entries },
        preferred_stack_size{ program_preferred_stack_size },
        code{ program_code },
        functions_count{ program_functions_count },
        exposed_functions{ program_exposed_functions },
        exposed_functions_count{ program_exposed_functions_count },
        jump_table{ program_jump_table },
        jump_table_size{ program_jump_table_size },
        strings{ program_strings },
        strings_size{ program_strings_size }
    {}

public:
    void* image_base{};
    void* runtime_function_entries{};

    std::uint64_t preferred_stack_size{};

    void** code{};
    std::uint32_t functions_count{};

    void** exposed_functions{};
    std::uint32_t exposed_functions_count{};

    void* jump_table{};
    std::uint64_t jump_table_size{};

    void** strings{};
    std::uint64_t strings_size{};

    static program_context* create(
        void* application_image_base, void* application_runtime_function_entries,
        std::uint64_t preferred_stack_size,
        void** code, std::uint32_t functions_count, 
        void** exposed_functions, std::uint32_t exposed_functions_count, 
        void* jump_table, std::uint64_t jump_table_size,
        void** program_strings, std::uint64_t program_strings_size
    ) {
        return new program_context{ 
            application_image_base, application_runtime_function_entries,
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
        std::scoped_lock lock{ object->references_mutex };
        assert(object->references_count != 0);

        ++object->references_count;
        return object;
    }

    std::size_t decrease_references_count() {
        std::scoped_lock lock{ this->references_mutex };
        return --this->references_count;
    }

    ~program_context() noexcept;
};

#endif // !PROGRAM_CONTEXT_H
