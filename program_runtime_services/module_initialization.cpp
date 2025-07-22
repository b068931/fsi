#include "pch.h"
#include "module_interoperation.h"

namespace {
	module_mediator::module_part* part = nullptr;
}

namespace interoperation {
    module_mediator::module_part* get_module_part() {
        return ::part;
    }

    module_mediator::return_value get_current_thread_group_id() {
        return module_mediator::fast_call(
            get_module_part(),
            index_getter::execution_module(),
            index_getter::execution_module_get_current_thread_group_id()
        );
    }

    module_mediator::return_value allocate(std::uint64_t size) {
        return module_mediator::fast_call<module_mediator::return_value, std::uint64_t>(
            get_module_part(),
            index_getter::resource_module(),
            index_getter::resource_module_allocate_program_memory(),
            get_current_thread_group_id(),
            size
        );
    }

    void deallocate(module_mediator::memory* pointer) {
        module_mediator::fast_call<module_mediator::return_value, void*>(
            get_module_part(),
            index_getter::resource_module(),
            index_getter::resource_module_deallocate_program_memory(),
            get_current_thread_group_id(),
            pointer
        );
    }
}

PROGRAMRUNTIMESERVICES_API void initialize_m(module_mediator::module_part* module_part) {
	::part = module_part;
}
