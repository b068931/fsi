#ifndef RESOURCE_CONTAINER_H
#define RESOURCE_CONTAINER_H

#include "pch.h"
#include "id_generator.h"
#include "module_interoperation.h"

#include "../logger_module/logging.h"

struct resource_container {
private:
    struct memory_comparator {
        bool operator() (const void* left, const void* right) const noexcept {
            // C++ standard deems pointer comparison to be UB if pointers do not belong to the same array, etc.
            return reinterpret_cast<std::uintptr_t>(left) < reinterpret_cast<std::uintptr_t>(right);
        }
    };

public:
    std::vector<module_mediator::callback_bundle*> destroy_callbacks{};
    std::set<void*, memory_comparator> allocated_memory{ memory_comparator{} };

    std::recursive_mutex* lock{ new std::recursive_mutex{} };

    resource_container() = default;
    void move_resource_container_to_this(resource_container&& object) {  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
        this->allocated_memory = std::move(object.allocated_memory);
    }

    resource_container(const resource_container& object) = delete;
    resource_container& operator= (const resource_container& object) = delete;

    resource_container(resource_container&& object) noexcept
        :allocated_memory{ std::move(object.allocated_memory) }
    {}
    resource_container& operator= (resource_container&& object) noexcept {
        this->move_resource_container_to_this(std::move(object));
        return *this;
    }

    virtual ~resource_container() noexcept {
        // Calls to destroy callbacks must precede the deallocation of memory
        for (auto& destroy_callback : this->destroy_callbacks) {
            std::size_t module_index = interoperation::get_module_part()->find_module_index(destroy_callback->module_name);
            if (module_index == module_mediator::module_part::module_not_found) {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Module {} not found.", 
                        destroy_callback->module_name
                    )
                );

                continue;
            }

            std::size_t function_index = interoperation::get_module_part()->find_function_index(
                module_index,
                destroy_callback->function_name
            );
            if (function_index == module_mediator::module_part::function_not_found) {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Function {} not found in module {}.", 
                        destroy_callback->function_name, 
                        destroy_callback->module_name
                    )
                );

                continue;
            }

            module_mediator::return_value result = interoperation::get_module_part()->call_module(
                module_index,
                function_index,
                destroy_callback->arguments_string
            );

            if (result != module_mediator::module_success) {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Deferred callback for module {} and function {} failed with error code {}." \
                        " Failure in callback execution may lead to memory leaks.",
                        destroy_callback->module_name,
                        destroy_callback->function_name,
                        result
                    )
                );
            }
        }

        if (!this->allocated_memory.empty()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(), 
                std::format("Destroyed resource container had {} dangling memory block(s).", this->allocated_memory.size())
            );
        }

        for (void* memory : this->allocated_memory) {
            // It is guaranteed that the memory is allocated as a char array
            delete[] static_cast<char*>(memory);
        }

        delete this->lock;
    }
};

#endif
