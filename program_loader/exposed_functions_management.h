#ifndef EXPOSED_FUNCTIONS_MANAGEMENT_H
#define EXPOSED_FUNCTIONS_MANAGEMENT_H

#include "../module_mediator/module_part.h"

struct exposed_function_data {
    std::string function_name;
    std::unique_ptr<module_mediator::arguments_string_element[]> function_signature;
};

extern void merge_exposed_functions(
    std::unordered_map<std::uintptr_t, exposed_function_data>& new_exposed_functions);

extern void remove_exposed_functions(std::span<void*> function_addresses);

#endif
