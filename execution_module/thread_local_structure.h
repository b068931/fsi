#ifndef THREAD_LOCAL_STRUCTURE_H
#define THREAD_LOCAL_STRUCTURE_H

#include "pch.h"
#include "scheduler.h"
#include "control_code_templates.h"

struct thread_local_structure {
    // Contains data that will be passed to the main function in thread.
    module_mediator::arguments_string_type initializer{};

    // These will be called after the putback. That is, after executor gives up its program thread.
    std::vector<module_mediator::callback_bundle*> deferred_callbacks{};

    // Will be used to store an address of the FSI function between calls to the "create_thread" and "on_thread_creation".
    void* program_function_address{};

    // Store program thread priority information between calls.
    module_mediator::return_value priority{
        std::numeric_limits<decltype(priority)>::min()
    };

    // State of the execution thread before switching to the program state.
    saved_executor_thread_state execution_thread_state{};

    // Information about the currently running program thread, as acquired from the scheduler.
    scheduler::schedule_information currently_running_thread_information{};
};

#endif // !THREAD_LOCAL_STRUCTURE_H
