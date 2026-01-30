#include "pch.h"
#include "program_functions.h"

namespace {
    std::uint32_t nullify_function_pointer_variables(std::vector<char>& destination, const memory_layouts_builder::memory_addresses& locals) {
        std::uint32_t instructions_size = 0;
        for (const auto& variable : locals | std::views::values) {
            if (variable.second == 4) { //if local is a pointer
                destination.push_back('\x48');
                destination.push_back('\xc7');
                destination.push_back('\x85');
                write_bytes(variable.first, destination);
                write_bytes<std::uint32_t>(0, destination);

                instructions_size += 11;
            }
        }

        return instructions_size;
    }
}

void generate_program_termination_code(std::vector<char>& destination, program_loader::termination_codes error_code) {
    destination.push_back('\x48'); //mov rcx, error_code
    destination.push_back('\xc7');
    destination.push_back('\xc1');
    ::write_bytes<std::int32_t>(static_cast<std::int32_t>(error_code), destination);
    
    destination.push_back('\x41'); //call [r10 + 24]
    destination.push_back('\xff');
    destination.push_back('\x52');
    destination.push_back('\x18');
}

void generate_stack_allocation_code(std::vector<char>& destination, std::uint32_t size) {
    if (size == 0) {
        return;
    }

    destination.push_back('\x48'); //add rbp, size
    destination.push_back('\x81');
    destination.push_back('\xc5');

    write_bytes(size, destination);

    destination.push_back('\x4c'); //cmp rbp, r9
    destination.push_back('\x39');
    destination.push_back('\xcd');

    destination.push_back('\x72'); //jb end
    destination.push_back(char{ program_termination_code_size });

    generate_program_termination_code(destination, program_loader::termination_codes::stack_overflow);

    //:end
}

void generate_stack_deallocation_code(std::vector<char>& destination, std::uint32_t size) {
    if (size == 0) {
        return;
    }

    destination.push_back('\x48'); //sub rbp, size
    destination.push_back('\x81');
    destination.push_back('\xed');

    write_bytes(size, destination);
}

std::uint32_t generate_function_prologue(std::vector<char>& destination, std::uint32_t allocation_size, const memory_layouts_builder::memory_addresses& locals) {
    destination.push_back('\x48'); //mov rax, [rsp]
    destination.push_back('\x8b');
    destination.push_back('\x04');
    destination.push_back('\x24');

    destination.push_back('\x48'); //mov [rbp], rax
    destination.push_back('\x89');
    destination.push_back('\x45');
    destination.push_back('\x00');

    destination.push_back('\x48'); //add rsp, 8
    destination.push_back('\x83');
    destination.push_back('\xc4');
    destination.push_back('\x08');

    generate_stack_allocation_code(destination, allocation_size + 8); //8 = return address
    return nullify_function_pointer_variables(destination, locals) + stack_allocation_code_size + 12;
}

void generate_function_epilogue(std::vector<char>& destination, std::uint32_t deallocation_size, std::uint32_t arguments_deallocation_size) {
    generate_stack_deallocation_code(destination, deallocation_size + 8); //8 = return address

    destination.push_back('\x48'); //mov rax, rbp
    destination.push_back('\x89');
    destination.push_back('\xe8');

    generate_stack_deallocation_code(destination, arguments_deallocation_size);

    destination.push_back('\xff'); //jmp [rax]
    destination.push_back('\x20');
}

void* create_executable_function(const std::vector<char>& source) {
    char* destination = static_cast<char*>(VirtualAlloc(
        nullptr, 
        source.size(), 
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE));

    if (destination == nullptr) {
        return nullptr;
    }

    std::ranges::copy(source, destination);

    DWORD previous_protection = 0;
    PDWORD previous_protection_pointer = &previous_protection;
    VirtualProtect(destination, source.size(), PAGE_EXECUTE, previous_protection_pointer); //W^E

    FlushInstructionCache(GetCurrentProcess(), destination, source.size());
    return destination;
}
