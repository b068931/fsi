#ifndef STANDARD_INPUT_OUTPUT_H
#define STANDARD_INPUT_OUTPUT_H

#include "module_interoperation.h"

PROGRAMRUNTIMESERVICES_API module_mediator::return_value attach_to_stdio(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value detach_from_stdio(module_mediator::arguments_string_type bundle);

PROGRAMRUNTIMESERVICES_API module_mediator::return_value callback_register_output(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value callback_register_input(module_mediator::arguments_string_type bundle);

PROGRAMRUNTIMESERVICES_API module_mediator::return_value out(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value in(module_mediator::arguments_string_type bundle);

#endif