#ifndef PRTS_POINTERS_H
#define PRTS_POINTERS_H

#include "declarations.h"

PROGRAMRUNTIMESERVICES_API return_value allocate_pointer(arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API return_value deallocate_pointer(arguments_string_type bundle);
PROGRAMRUNTIMESERVICES_API return_value get_allocated_size(arguments_string_type bundle);

#endif