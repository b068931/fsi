#include "pch.h"
#include "module_interoperation.h"
#include "execution_module.h"
#include "program_state_manager.h"
#include "execution_backend_functions.h"
#include "thread_local_structure.h"
#include "thread_manager.h"
#include "unwind_info.h"

#include "../module_mediator/fsi_types.h"
#include "../module_mediator/module_part.h"
#include "../program_loader/program_functions.h"
#include "../logger_module/logging.h"

module_mediator::return_value on_thread_creation(module_mediator::arguments_string_type bundle) {
    constexpr std::uint64_t program_start_function_index = 1;
    constexpr std::uint64_t program_jump_table_address_index = 2;
    constexpr std::uint64_t thread_state_address_index = 3;
    constexpr std::uint64_t current_stack_position_index = 4;
    constexpr std::uint64_t stack_end_position_index = 5;
    constexpr std::uint64_t trap_table_address_index = 6;

    auto [container_id, thread_id, preferred_stack_size] =
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, module_mediator::return_value, std::uint64_t>(bundle);

    thread_local_structure* thread_structure = backend::get_thread_local_structure();

    char* thread_state_memory = backend::allocate_thread_memory(thread_id, program_state_manager::thread_state_size);
    char* thread_stack_memory = backend::allocate_thread_memory(thread_id, preferred_stack_size);
    
    // One byte + eight bytes are reserved to be used with "save" and "load" instructions.
    char* thread_stack_end = thread_stack_memory + preferred_stack_size - (sizeof(module_mediator::one_byte) + sizeof(std::uint64_t));

    void* program_jump_table = std::bit_cast<void*>(
        module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_get_jump_table(),
            container_id
        )
    );

    // Fill in state_buffer - start.
    // Fill in program main function address.
    backend::fill_in_register_array_entry( 
        program_start_function_index, 
        thread_state_memory, 
        reinterpret_cast<std::uintptr_t>(thread_structure->program_function_address)
    );

    // Fill in jump table address.
    backend::fill_in_register_array_entry(
        program_jump_table_address_index,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(program_jump_table)
    );

    // Fill in thread state address. I don't quite remember what purpose this serves.
    backend::fill_in_register_array_entry(
        thread_state_address_index,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(thread_state_memory)
    );

    std::uintptr_t result = backend::initialize_thread_stack(
        thread_structure->program_function_address, 
        thread_stack_memory, 
        thread_stack_end, 
        thread_id
    );

    if (result == reinterpret_cast<std::uintptr_t>(nullptr)) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "Thread stack initialization has failed.");

        module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread_memory(),
            thread_id,
            thread_state_memory
        );
        
        module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_deallocate_thread_memory(),
            thread_id,
            thread_stack_memory
        );

        module_mediator::return_value result_container_id = backend::deallocate_thread(thread_id);
        if (backend::get_container_running_threads_count(result_container_id) == 0) {
            backend::get_thread_manager().forget_thread_group(result_container_id);
            backend::deallocate_program_container(result_container_id);
        }

        return module_mediator::module_failure;
    }

    // Fill in current stack position with the value obtained from stack initializer.
    backend::fill_in_register_array_entry(
        current_stack_position_index,
        thread_state_memory,
        result
    );

    // Fill in stack end address. Notice that it had to account for the space that 
    // will be used to save the state of one variable between function calls.
    backend::fill_in_register_array_entry(
        stack_end_position_index,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(thread_stack_end)
    );

    // Fill in runtime trap table address. This is mostly a limitation of the current design,
    // as "program_loader" is a separate module, and it can't directly access "execution_module" data.
    backend::fill_in_register_array_entry(
        trap_table_address_index,
        thread_state_memory,
        reinterpret_cast<std::uintptr_t>(backend::get_runtime_trap_table())
    );
    // Fill in thread state - end.

    backend::get_thread_manager().add_thread(
        container_id,
        thread_id,
        thread_structure->priority,
        thread_state_memory,
        program_jump_table
    );

    LOG_PROGRAM_INFO(interoperation::get_module_part(), "New thread has been successfully created.");
    return module_mediator::module_success;
}

module_mediator::return_value on_container_creation(module_mediator::arguments_string_type bundle) {
    auto [container_id, program_main, preferred_stack_size] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, std::uint64_t>(bundle);

    backend::get_thread_manager().add_thread_group(container_id, preferred_stack_size);
    LOG_PROGRAM_INFO(interoperation::get_module_part(), "New thread group has been successfully created.");

    return backend::create_thread(container_id, 0, program_main);
}

module_mediator::return_value register_deferred_callback(module_mediator::arguments_string_type bundle) {
    auto [callback_info] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::memory>(bundle);

    module_mediator::callback_bundle* callback = static_cast<module_mediator::callback_bundle*>(callback_info);
    backend::get_thread_local_structure()->deferred_callbacks.push_back(callback);

    return module_mediator::module_success;
}

module_mediator::return_value self_duplicate(module_mediator::arguments_string_type bundle) {
    auto [main_function_address, main_function_parameters, parameters_size] =
        module_mediator::arguments_string_builder::unpack<void*, void*, std::uint64_t>(bundle);

    module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[parameters_size]{};
    std::memcpy(copy, main_function_parameters, parameters_size);

    return backend::self_duplicate(main_function_address, copy);
}

module_mediator::return_value self_priority(module_mediator::arguments_string_type) {
    return backend::get_thread_local_structure()->currently_running_thread_information.priority;
}

module_mediator::return_value get_thread_saved_variable(module_mediator::arguments_string_type) {
    return program_state_manager{ 
        static_cast<char*>(
            backend::get_thread_local_structure()->currently_running_thread_information.thread_state)
    }.get_stack_end();
}

module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle) {
    auto [function_address, function_arguments_data] =
        module_mediator::arguments_string_builder::unpack<void*, void*>(bundle);

    module_mediator::arguments_string_type function_arguments_string =
        static_cast<module_mediator::arguments_string_type>(function_arguments_data);

    if (backend::check_function_signature(function_address, function_arguments_string) != module_mediator::module_success) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(), "A function signature for function '" + backend::get_exposed_function_name(function_address) +
            "' does not match passed arguments. (dynamic call)"
        );

        return module_mediator::module_failure;
    }

    program_state_manager state_manager{ 
        static_cast<char*>(backend::get_thread_local_structure()->currently_running_thread_information.thread_state) 
    };

    std::uintptr_t program_return_address = state_manager.get_return_address();
    module_mediator::arguments_array_type function_arguments = module_mediator::arguments_string_builder::convert_to_arguments_array(
        function_arguments_string
    );

    function_arguments.insert(
        function_arguments.cbegin(),
        {
            static_cast<module_mediator::arguments_string_element>(
                module_mediator::arguments_string_builder::get_type_index<std::uintptr_t>
            ),
            &program_return_address
        }
    );

    std::uintptr_t new_current_stack_position = backend::apply_initializer_on_thread_stack(
        std::bit_cast<char*>(state_manager.get_current_stack_position()),
        std::bit_cast<char*>(state_manager.get_stack_end()),
        function_arguments,
        backend::get_thread_local_structure()->currently_running_thread_information.thread_id
    );

    if (new_current_stack_position == reinterpret_cast<std::uintptr_t>(nullptr)) {
        LOG_PROGRAM_ERROR(interoperation::get_module_part(), "The thread stack initialization has failed. (dynamic call)");
        return module_mediator::module_failure;
    }

    state_manager.set_current_stack_position(
        new_current_stack_position - module_mediator::arguments_string_builder::get_type_size_by_index(
            module_mediator::arguments_string_builder::get_type_index<std::uintptr_t>
        )
    );
    
    state_manager.set_return_address(
        reinterpret_cast<std::uintptr_t>(static_cast<char*>(function_address) + function_save_return_address_size)
    );

    return module_mediator::module_success;
}

module_mediator::return_value get_current_thread_id(module_mediator::arguments_string_type) {
    return backend::get_thread_local_structure()->currently_running_thread_information.thread_id;
}

module_mediator::return_value get_current_thread_group_id(module_mediator::arguments_string_type) {
    return backend::get_thread_local_structure()->currently_running_thread_information.thread_group_id;
}

module_mediator::return_value make_runnable(module_mediator::arguments_string_type bundle) {
    auto [thread_id] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value>(bundle);

    if (backend::get_thread_manager().make_runnable(thread_id)) {
        LOG_PROGRAM_INFO(interoperation::get_module_part(), "Made a thread with id '" + std::to_string(thread_id) + "' runnable again.");
    }
    else {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Was unable to make a thread with {} runnable again. This most likely indicates a data race.",
                thread_id
            )
        );

        return module_mediator::module_failure;
    }

    return module_mediator::module_success;
}

module_mediator::return_value start(module_mediator::arguments_string_type bundle) {
    auto [thread_count] =
        module_mediator::arguments_string_builder::unpack<std::uint16_t>(bundle);

    LOG_INFO(
        interoperation::get_module_part(), 
        "Creating execution daemons. Count: " + std::to_string(thread_count)
    );

    backend::get_thread_manager().startup(thread_count);
    return module_mediator::module_success;
}

module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle) {
    auto [priority, main_function_address, main_function_parameters, parameters_size] =
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, void*, std::uint64_t>(bundle);

    module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[parameters_size] {};
    std::memcpy(copy, main_function_parameters, parameters_size);

    return backend::create_thread_initializer(priority, main_function_address, copy);
}

module_mediator::return_value build_unwind_info(module_mediator::arguments_string_type bundle) {
    auto [unwind_info_buffer, buffer_size] =
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, std::uint64_t>(bundle);

    DWORD64 dw64LoadProgramBase = 0;
    PRUNTIME_FUNCTION prfLoadProgram = RtlLookupFunctionEntry(
        std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_LOAD_PROGRAM),
        &dw64LoadProgramBase,
        nullptr
    );

    if (prfLoadProgram == nullptr) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "RUNTIME_FUNCTION entry for LOAD_PROGRAM is unavailable."
        );

        return module_mediator::module_failure;
    }

    UNWIND_INFO_DISPATCHER_PROLOGUE* loadProgramUnwindInfo = std::launder(
        std::bit_cast<UNWIND_INFO_DISPATCHER_PROLOGUE*>(dw64LoadProgramBase + prfLoadProgram->UnwindData));

#ifndef NDEBUG

    // Align to an even number, as required by Windows ABI.
    UBYTE ubUnwindCodeCount = static_cast<UBYTE>(
        (loadProgramUnwindInfo->Header.CountOfCodes + 1) & ~1);

    std::size_t unwind_info_size = sizeof(UNWIND_INFO_HEADER) +
        (ubUnwindCodeCount * sizeof(USHORT));

    // This is used to verify that our understanding of the unwind info that
    // was generated by MASM is correct.
    assert(unwind_info_size == sizeof(UNWIND_INFO_DISPATCHER_PROLOGUE) && 
        "Unwind info definition in the header file is incorrect.");

#else
    
    constexpr std::size_t unwind_info_size = sizeof(UNWIND_INFO_DISPATCHER_PROLOGUE);

#endif

    if (buffer_size < unwind_info_size) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format("Provided buffer size of {} bytes is insufficient for unwind info of size {} bytes.",
                buffer_size,
                unwind_info_size
            )
        );

        return module_mediator::module_failure;
    }

    // The wording at https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64?view=msvc-170#chained-unwind-info-structures
    // Seems to suggest that the following use case is possible.
    UNWIND_INFO_DISPATCHER_PROLOGUE* unwindInfoCopy =
        new(unwind_info_buffer) UNWIND_INFO_DISPATCHER_PROLOGUE{ *loadProgramUnwindInfo };

    // That is, to unwind a dynamic function you must perform the same actions as for
    // LOAD_PROGRAM, but it has no prologue code.
    unwindInfoCopy->Header.SizeOfProlog = 0;

    return unwind_info_size;
}

module_mediator::return_value verify_application_image(module_mediator::arguments_string_type bundle) {
    auto [image_base, image_size, function_addresses, functions_count, 
        runtime_function_table, unwind_info] = module_mediator::arguments_string_builder::unpack<
        module_mediator::memory, std::uint64_t, 
        module_mediator::memory, std::uint32_t, 
        module_mediator::memory, module_mediator::memory>(bundle);

    if (image_base == nullptr) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "Image base must not be nullptr."
        );

        return module_mediator::module_failure;
    }

    if (function_addresses == nullptr) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "Function table must not be nullptr."
        );

        return module_mediator::module_failure;
    }

    if (unwind_info == nullptr) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "Provided unwind info must not be null."
        );

        return module_mediator::module_failure;
    }

    if (image_size > std::numeric_limits<ULONG>::max()) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            std::format("Image size {} exceeds maximum supported size.", image_size)
        );

        return module_mediator::module_failure;
    }

    // Load reference UNWIND_INFO from CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.
    DWORD64 dw64LoadProgramBase = 0;
    PRUNTIME_FUNCTION prfLoadProgram = RtlLookupFunctionEntry(
        std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_LOAD_PROGRAM),
        &dw64LoadProgramBase,
        nullptr
    );

    if (prfLoadProgram == nullptr) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "RUNTIME_FUNCTION entry for CONTROL_CODE_TEMPLATE_LOAD_PROGRAM is unavailable."
        );

        return module_mediator::module_failure;
    }

    constexpr std::size_t reference_unwind_info_size = sizeof(UNWIND_INFO_DISPATCHER_PROLOGUE);
    UNWIND_INFO_HEADER* providedUnwindInfo =
        static_cast<UNWIND_INFO_HEADER*>(unwind_info);

    // Verify that provided unwind_info matches the reference UNWIND_INFO from CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.
    {
        UNWIND_INFO_DISPATCHER_PROLOGUE referenceUnwindInfo = *std::launder(std::bit_cast<UNWIND_INFO_DISPATCHER_PROLOGUE*>(
            dw64LoadProgramBase + prfLoadProgram->UnwindData));

#ifndef NDEBUG

        UBYTE ubUnwindCodeCount = static_cast<UBYTE>(
            (referenceUnwindInfo.Header.CountOfCodes + 1) & ~1);

        std::size_t actual_reference_unwind_info_size = sizeof(UNWIND_INFO_HEADER) +
            (ubUnwindCodeCount * sizeof(USHORT));

        assert(actual_reference_unwind_info_size == reference_unwind_info_size &&
            "Unwind info definition in the header file is incorrect.");

#endif

        // Dynamic functions have no prologue code on their own.
        // They just copy unwind codes from LOAD_PROGRAM.
        referenceUnwindInfo.Header.SizeOfProlog = 0;

        if (std::memcmp(providedUnwindInfo, &referenceUnwindInfo, reference_unwind_info_size) != 0) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                "Provided unwind info does not match CONTROL_CODE_TEMPLATE_LOAD_PROGRAM's unwind info."
            );

            return module_mediator::module_failure;
        }
    }

    DWORD64 dw64ImageBase = std::bit_cast<DWORD64>(image_base);
    void** function_addresses_array = static_cast<void**>(function_addresses);
    PRUNTIME_FUNCTION prfExpectedRuntimeFunctionTable = 
        static_cast<PRUNTIME_FUNCTION>(runtime_function_table);

    for (std::uint32_t index = 0; index < functions_count; ++index) {
        void* function_address = function_addresses_array[index];
        DWORD64 dw64FunctionBase = 0;

        PRUNTIME_FUNCTION prfCurrent = RtlLookupFunctionEntry(
            std::bit_cast<DWORD64>(function_address),
            &dw64FunctionBase,
            nullptr
        );

        if (prfCurrent == nullptr) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                std::format("RUNTIME_FUNCTION entry for function at {:#x} is unavailable.", 
                    std::bit_cast<std::uintptr_t>(function_address))
            );

            return module_mediator::module_failure;
        }

        if (dw64FunctionBase != dw64ImageBase) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                std::format("Function at index {} has mismatched base address. Expected: {:#x}, Actual: {:#x}.",
                    index, dw64ImageBase, dw64FunctionBase)
            );

            return module_mediator::module_failure;
        }

        PRUNTIME_FUNCTION prfExpectedEntry = &prfExpectedRuntimeFunctionTable[index];
        if (prfCurrent->BeginAddress != prfExpectedEntry->BeginAddress ||
            prfCurrent->EndAddress != prfExpectedEntry->EndAddress ||
            prfCurrent->UnwindData != prfExpectedEntry->UnwindData) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                std::format("RUNTIME_FUNCTION entry at index {} does not match expected values.", index)
            );

            return module_mediator::module_failure;
        }

        if (prfCurrent->BeginAddress >= image_size || prfCurrent->EndAddress > image_size) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                std::format("RUNTIME_FUNCTION entry at index {} has offsets outside image bounds. "
                    "BeginAddress: {:#x}, EndAddress: {:#x}, ImageSize: {:#x}.",
                    index, prfCurrent->BeginAddress, prfCurrent->EndAddress, image_size)
            );

            return module_mediator::module_failure;
        }

        if (prfCurrent->UnwindData >= image_size) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                std::format("RUNTIME_FUNCTION entry at index {} has UnwindData offset outside image bounds. "
                    "UnwindData: {:#x}, ImageSize: {:#x}.",
                    index, prfCurrent->UnwindData, image_size)
            );

            return module_mediator::module_failure;
        }

        UNWIND_INFO_HEADER* actualUnwindInfo = std::launder(std::bit_cast<UNWIND_INFO_HEADER*>(
            dw64FunctionBase + prfCurrent->UnwindData));

        if (std::memcmp(actualUnwindInfo, providedUnwindInfo, reference_unwind_info_size) != 0) {
            LOG_PROGRAM_ERROR(
                interoperation::get_module_part(),
                std::format("Unwind info for function at index {} does not match expected values.", index)
            );

            return module_mediator::module_failure;
        }
    }

    return module_mediator::module_success;
}
