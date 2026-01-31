#include "pch.h"
#include "program_loader.h"
#include "exposed_functions_management.h"

#include "../logger_module/logging.h"

extern std::unordered_map<std::uintptr_t, exposed_function_data>* exposed_functions;

namespace {
    std::shared_mutex exposed_functions_mutex{};
}

void merge_exposed_functions(std::unordered_map<std::uintptr_t, exposed_function_data>& new_exposed_functions) {
    std::scoped_lock lock{ exposed_functions_mutex };
    exposed_functions->merge(new_exposed_functions);
}

void remove_exposed_functions(std::span<void*> function_addresses) {
    std::scoped_lock lock{ exposed_functions_mutex };
    for (void* function_address : function_addresses) {
        if (function_address != nullptr) {
            auto result = exposed_functions->erase(
            std::bit_cast<std::uintptr_t>(function_address)
            );

            if (result != 1) {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format("Failed to remove exposed function with address {} "
                                "from exposed functions list.", std::bit_cast<std::uintptr_t>(function_address))
                );
            }
        }
    }
}

module_mediator::return_value check_function_arguments(module_mediator::arguments_string_type bundle) {
    auto [signature_string, function_address] =
        module_mediator::arguments_string_builder::unpack<void*, unsigned long long>(bundle);

    // std::unordered_map does not invalidate references and pointers to its objects unless they were erased.
    module_mediator::arguments_string_type found_signature_string{};
    {
        std::shared_lock lock{ exposed_functions_mutex };
        auto found_exposed_function_information = exposed_functions->find(function_address);
        if (found_exposed_function_information != exposed_functions->end()) {
            found_signature_string = found_exposed_function_information->second.function_signature.get();
        }
    }

    if (found_signature_string == module_mediator::arguments_string_type{}) {
        return module_mediator::module_failure; // "function was not found"
    }

    if (module_mediator::arguments_string_builder::check_if_arguments_strings_match(
        static_cast<module_mediator::arguments_string_type>(signature_string), 
        found_signature_string)
        ) {
        return module_mediator::module_success; // "signatures match"
    }

    return module_mediator::module_failure; // "signatures do not match"
}

module_mediator::return_value get_function_name(module_mediator::arguments_string_type bundle) {
    auto [function_address] = 
        module_mediator::arguments_string_builder::unpack<std::uintptr_t>(bundle);

    char* found_name = nullptr;
    {
        std::shared_lock lock{ exposed_functions_mutex };
        auto found_exposed_function_information = exposed_functions->find(function_address);
        if (found_exposed_function_information != exposed_functions->end()) {
            found_name = found_exposed_function_information->second.function_name.data();
        }
    }

    return reinterpret_cast<std::uintptr_t>(found_name);
}

