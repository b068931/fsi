#ifndef PRTS_BACKEND_FUNCTIONS_H
#define PRTS_BACKEND_FUNCTIONS_H

#include "module_interoperation.h"

namespace backend {
    module_mediator::memory allocate_program_memory(
        module_mediator::return_value thread_id,
        module_mediator::return_value thread_group_id, 
        module_mediator::eight_bytes size
    );

    void deallocate_program_memory(
        module_mediator::return_value thread_id,
        module_mediator::return_value thread_group_id, 
        module_mediator::memory address
    );

    std::pair<module_mediator::memory, module_mediator::eight_bytes> decay_pointer(module_mediator::memory);
}

#endif