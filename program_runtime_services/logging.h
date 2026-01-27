#ifndef PRTS_LOGGING_H
#define PRTS_LOGGING_H

#include "module_interoperation.h"

PROGRAMRUNTIMESERVICES_API module_mediator::return_value info(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value warning(module_mediator::arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API module_mediator::return_value error(module_mediator::arguments_string_type bundle);

#endif
