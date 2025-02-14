#ifndef PRTS_POINTERS_H
#define PRTS_POINTERS_H

#include "module_interoperation.h"

PROGRAMRUNTIMESERVICES_API module_mediator::return_value allocate_pointer(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value deallocate_pointer(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value get_allocated_size(module_mediator::arguments_string_type bundle);

#endif