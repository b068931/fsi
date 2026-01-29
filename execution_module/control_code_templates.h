#ifndef CONTROL_CODE_TEMPLATES_H
#define CONTROL_CODE_TEMPLATES_H

#include "../module_mediator/module_part.h"

extern "C" {
    extern const inline std::uint64_t CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_SIZE;
    extern const inline std::uint64_t CONTROL_CODE_TEMPLATE_LOAD_PROGRAM_SIZE;
    extern const inline std::uint64_t CONTROL_CODE_TEMPLATE_SPECIAL_CALL_MODULE;
    extern const inline std::uint64_t CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_SIZE;

    [[noreturn]] void             CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD(void*);
                 void             CONTROL_CODE_TEMPLATE_LOAD_PROGRAM(void*, void*, std::size_t is_startup);
    [[noreturn]] void             CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION(void*);
    module_mediator::return_value CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE();
}

#endif
