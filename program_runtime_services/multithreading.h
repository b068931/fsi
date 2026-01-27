#ifndef PRTS_MULTITHREADING_H
#define PRTS_MULTITHREADING_H

#include "module_interoperation.h"

PROGRAMRUNTIMESERVICES_API module_mediator::return_value yield(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value self_terminate(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value self_priority(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value thread_id(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value thread_group_id(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle);

PROGRAMRUNTIMESERVICES_API module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value create_thread_group(module_mediator::arguments_string_type bundle);

#endif
