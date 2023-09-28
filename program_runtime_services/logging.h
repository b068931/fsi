#ifndef PRTS_LOGGING_H
#define PRTS_LOGGING_H

#include "declarations.h"

PROGRAMRUNTIMESERVICES_API return_value info(arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API return_value warning(arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API return_value error(arguments_string_type bundle);

#endif