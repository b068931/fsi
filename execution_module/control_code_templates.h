#ifndef CONTROL_CODE_TEMPLATES_H
#define CONTROL_CODE_TEMPLATES_H

#include <cstdint>

/// <summary>
/// CONTROL_CODE_TEMPLATE_LOAD_PROGRAM saves the value of both the stack top register and
/// the stack frame pointer register into this structure. Saving frame pointer allows for a
/// complete continuity of stack frames across context switches. Low-level functions use 
/// this data to change context on a per-instruction basis.
/// </summary>
struct saved_executor_thread_state {
    std::uint64_t stack_top{};
    std::uint64_t stack_frame_pointer{};
};

extern "C" {
    extern const std::uint64_t CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_SIZE;
    extern const std::uint64_t CONTROL_CODE_TEMPLATE_LOAD_PROGRAM_SIZE;
    extern const std::uint64_t CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE_SIZE;
    extern const std::uint64_t CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_SIZE;
    extern const std::uint64_t CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE_SIZE;

                 void CONTROL_CODE_TEMPLATE_LOAD_PROGRAM(void*, void*, std::size_t is_startup);
    [[noreturn]] void CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD(void*);
    [[noreturn]] void CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION(void*);

    // These trampoline functions are not to be used from the C++ code directly.
    // They are only declared here to allow referencing their addresses.
    // They should be called by generated machine code only.

    extern char CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE[1];
    extern char CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE[1];
    extern char CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT[1];
    extern char CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT[1];
}

#endif
