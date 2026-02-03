#include "pch.h"
#include "program_loader.h"
#include "exposed_functions_management.h"

#include "run_reader.h"
#include "run_container.h"
#include "program_functions.h"
#include "instruction_builder_classes.h"
#include "module_interoperation.h"

#include "jump_table_builder.h"
#include "memory_layouts_builder.h"
#include "compiled_program.h"
#include "application_image.h"
#include "program_compilation_error.h"

#include "../logger_module/logging.h"
#include "../module_mediator/fsi_types.h"
#include "../execution_module/unwind_info.h"

namespace {
    module_mediator::return_value add_program(
        void* image_base, void* runtime_functions,
        std::uint64_t preferred_stack_size, std::uint32_t main_function_index,
        void** code, std::uint32_t functions_count,
        void** exposed_functions_addresses, std::uint32_t exposed_functions_count,
        void* jump_table, std::uint64_t jump_table_size,
        void** program_strings, std::uint64_t program_strings_count
    ) {
        return module_mediator::fast_call<
            module_mediator::memory, module_mediator::memory,
            module_mediator::eight_bytes, module_mediator::four_bytes,
            module_mediator::memory, module_mediator::four_bytes,
            module_mediator::memory, module_mediator::four_bytes,
            module_mediator::memory, module_mediator::eight_bytes,
            module_mediator::memory, module_mediator::eight_bytes
        >(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_create_new_program_container(),
            image_base, runtime_functions,
            preferred_stack_size, main_function_index,
            code, functions_count,
            exposed_functions_addresses, exposed_functions_count,
            jump_table, jump_table_size,
            program_strings, program_strings_count
        );
    }

    std::vector<
        std::pair<memory_layouts_builder::memory_addresses, memory_layouts_builder::memory_addresses>
    > construct_memory_layout(const runs_container& container) {
        memory_layouts_builder memory_builder{}; //create memory layouts for functions
        for (const auto& fnc : container.function_bodies) {
            auto found_signature = container.function_signatures.find(fnc.function_signature);
            if (found_signature != container.function_signatures.end()) { //create memory layout for function only if we recognize its signature
                memory_builder.add_new_function();
                for (const auto& local : fnc.locals) {
                    memory_builder.add_new_variable(local);
                }

                memory_builder.move_to_function_arguments();
                for (const auto& argument : found_signature->second.argument_types) {
                    memory_builder.add_new_variable(argument);
                }
            }
            else {
                throw program_compilation_error{ "Incorrect structure of the binary file. Function with an unknown signature." };
            }
        }

        return memory_builder.get_memory_layouts();
    }

    jump_table_builder construct_jump_table(const runs_container& container) {
        jump_table_builder jump_table{}; //create jump_table builder and initialize it
        for (const auto& fnc : container.function_bodies) {
            jump_table.add_new_function_address(fnc.function_signature, 
                reinterpret_cast<std::uintptr_t>(nullptr));
        }

        for (const auto& jump_point : container.jump_points) {
            jump_table.add_new_jump_address(
                std::get<0>(jump_point),
                std::get<1>(jump_point),
                std::get<2>(jump_point)
            );
        }

        return jump_table;
    }

    std::map<std::uint8_t, std::vector<char>> get_machine_codes() {
        return {
                {0, {'\x28', '\x29', '\x29', '\x29'}},
                {1, {'\x28', '\x29', '\x29', '\x29'}},
                {2, {'\xf6', '\xf7', '\xf7', '\xf7', '\x06'}}, //6
                {3, {'\xf6', '\xf7', '\xf7', '\xf7', '\x07'}}, //7
                {4, {'\x3b'}},
                {5, {'\x88', '\x89', '\x89', '\x89'}},
                {6, {'\x00', '\x01', '\x01', '\x01'}},
                {7, {'\x00', '\x01', '\x01', '\x01'}},
                {8, {'\xf6', '\xf7', '\xf7', '\xf7', '\x04'}}, //4
                {9, {'\xf6', '\xf7', '\xf7', '\xf7', '\x05'}}, //5
                {10, {'\xfe', '\xff', '\xff', '\xff', '\x00'}}, //0
                {11, {'\xfe', '\xff', '\xff', '\xff', '\x01'}}, //1
                {12, {'\x30', '\x31', '\x31', '\x31'}},
                {13, {'\x20', '\x21', '\x21', '\x21'}},
                {14, {'\x08', '\x09', '\x09', '\x09'}},
                {15, {'\xff', '\x04'}}, //jump instructions information
                {16, {'\x0f', '\x85'}},
                {17, {'\x0f', '\x84'}},
                {18, {'\x0f', '\x8e'}},
                {19, {'\x0f', '\x8c'}},
                {20, {'\x0f', '\x8f'}},
                {27, {'\x0f', '\x8d'}},
                {21, {'\x0f', '\x86'}},
                {22, {'\x0f', '\x82'}},
                {23, {'\x0f', '\x83'}},
                {24, {'\x0f', '\x87'}},
                {28, {'\xf6', '\xf7', '\xf7', '\xf7', '\x02'}}, //2
                {32, {'\xd2', '\xd3', '\xd3', '\xd3', '\x04'}}, //4
                {33, {'\xd2', '\xd3', '\xd3', '\xd3', '\x05'}} //5
        };
    }

    std::uint32_t calculate_function_locals_stack_space(const runs_container::function& function) {
        std::uint32_t stack_space = 0; //calculate how much stack space this function needs
        for (const auto& variable_type : function.locals | std::views::values) {
            stack_space += static_cast<std::uint8_t>(memory_layouts_builder::get_variable_size(variable_type));
        }

        return stack_space;
    }

    std::uint32_t calculate_function_arguments_stack_space(const memory_layouts_builder::memory_addresses& function_arguments_information) {
        std::uint32_t function_arguments_size = 0;
        for (const auto& variable_data : function_arguments_information | std::views::values) {
            function_arguments_size += static_cast<std::uint8_t>(memory_layouts_builder::get_variable_size(variable_data.second));
        }

        return function_arguments_size;
    }

    std::pair<void**, std::uint64_t> create_program_strings(runs_container& container) {
        std::uint64_t program_strings_size = container.program_strings.size();

        void** program_strings = new void* [program_strings_size] {};
        auto current_element = container.program_strings.begin();
        for (std::uint64_t index = 0; index < program_strings_size; ++index) {
            program_strings[index] = current_element->second.first.release();
            ++current_element;
        }

        return { program_strings, program_strings_size };
    }

    std::variant<std::string, application_image> create_application_image(
        const std::vector<std::vector<char>>& sources,
        std::unique_ptr<UNWIND_INFO_PROLOGUE> unwind_info,
        std::size_t unwind_info_size
    ) {
        // Application image has the following format:
        // 1. Function bodies, placed one after another.
        // 2. Runtime function entries, placed one after another.
        //    These entries must start at a new page boundary.
        //    This is required in order to be able to mark executable code with W^X policy,
        //    while keeping unwind information readable and writable without execution.
        // 3. Unwind information provided in "unwind_info" parameter. It must also start at a new page boundary.
        //    There is only one UNWIND_INFO instance in the image, all RUNTIME_FUNCTIONS point to it.
        // This whole data is allocated as a one contiguous block of memory, then registered with the system.
        // Note that RUNTIME_FUNCTIONs and UNWIND_INFORMATION usually require DWORD alignment. However,
        // we align them at a page boundary, which is usually an even larger alignment, so it satisfies this requirement as well.

        if (sources.empty()) {
            return "No function bodies provided.";
        }

        SYSTEM_INFO system_info{};
        GetSystemInfo(&system_info);
        const std::size_t page_size = system_info.dwPageSize;

        static_assert(alignof(UNWIND_INFO_PROLOGUE) == alignof(RUNTIME_FUNCTION),
            "Unexpected alignment requirements for system objects.");

        if (page_size < alignof(UNWIND_INFO_PROLOGUE) || 
            page_size % alignof(UNWIND_INFO_PROLOGUE) != 0 ||
            page_size % alignof(RUNTIME_FUNCTION) != 0) {
            return std::format("Failed to satisfy alignment requirements for RUNTIME_FUNCTION "
                   "or UNWIND_INFO_PROLOGUE with the system page size. System page: {}.", page_size);
        }

        auto align_to_page = [page_size](std::size_t value) -> std::size_t {
            return (value + page_size - 1) & ~(page_size - 1);
        };

        std::size_t total_code_size = 0;
        for (const auto& source : sources) {
            total_code_size += source.size();
        }

        const std::size_t code_section_size = align_to_page(total_code_size);
        const std::size_t runtime_functions_size = 
            align_to_page(sources.size() * sizeof(RUNTIME_FUNCTION));

        const std::size_t unwind_info_section_size = align_to_page(unwind_info_size);

        assert(code_section_size % page_size == 0 && "Invalid alignment for code section");
        assert(runtime_functions_size % page_size == 0 && "Invalid alignment for runtime functions section.");
        assert(unwind_info_section_size % page_size == 0 && "Invalid alignment for unwind info.");

        const std::size_t runtime_functions_offset = code_section_size;
        const std::size_t unwind_info_offset = runtime_functions_offset + runtime_functions_size;
        const std::size_t total_image_size = unwind_info_offset + unwind_info_section_size;

        // Allocate memory for the entire image
        char* image_base = static_cast<char*>(VirtualAlloc(
            nullptr,
            total_image_size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE));

        if (image_base == nullptr) {
            return std::format("Failed to allocate memory for the application image "
                               "(VirtualAlloc: {})", GetLastError());
        }

        // Populate the code section with dynamic functions.
        char* current_code_position = image_base;
        std::unique_ptr<void* []> function_addresses{ new void* [sources.size()] };
        for (std::size_t index = 0; index < sources.size(); ++index) {
            function_addresses[index] = current_code_position;

            std::ranges::copy(sources[index], current_code_position);
            current_code_position += sources[index].size();
        }

        // Populate our XDATA (unwind information).
        UNWIND_INFO_PROLOGUE* unwind_info_destination = 
            reinterpret_cast<UNWIND_INFO_PROLOGUE*>(image_base + unwind_info_offset);

        std::memcpy(unwind_info_destination, unwind_info.get(), unwind_info_size);
        DWORD unwind_info_rva = static_cast<DWORD>(unwind_info_offset);

        // Populate our PDATA (runtime function entries).
        PRUNTIME_FUNCTION runtime_functions = 
            reinterpret_cast<PRUNTIME_FUNCTION>(image_base + runtime_functions_offset);

        DWORD current_function_offset = 0;
        for (std::size_t index = 0; index < sources.size(); ++index) {
            PRUNTIME_FUNCTION runtime_function = 
                new(runtime_functions + index) RUNTIME_FUNCTION{};

            // Don't need std::launder here, because we use pointer from placement new.
            runtime_function->BeginAddress = current_function_offset;
            runtime_function->UnwindInfoAddress = unwind_info_rva;
            runtime_function->EndAddress = current_function_offset + 
                static_cast<DWORD>(sources[index].size());

            current_function_offset += static_cast<DWORD>(sources[index].size());
        }

        auto free_image_base_on_fail = [](char* image_base) {
            if (!VirtualFree(image_base, 0, MEM_RELEASE)) {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(),
                    "Failed to free application image memory. This may lead to resource leaks."
                );
            }
        };

        // Set executable protection on the code section (W^X policy)
        DWORD dwPreviousProtection = 0;
        if (!VirtualProtect(image_base, total_code_size, PAGE_EXECUTE_READ, &dwPreviousProtection)) {
            free_image_base_on_fail(image_base);
            return std::format("Failed to change code section protection. "
                                "(VirtualProtect: {})", GetLastError());
        }

        assert(dwPreviousProtection == PAGE_READWRITE && "Unexpected previous protection for image.");

        // Flush instruction cache for the code section
        if (!FlushInstructionCache(GetCurrentProcess(), image_base, total_code_size)) {
            free_image_base_on_fail(image_base);
            return std::format("Was unable to flush existing instruction cache. "
                               "(FlushInstructionCache: {})", GetLastError());
        }

        // Register the function table with the system for exception handling
        if (!RtlAddFunctionTable(runtime_functions, static_cast<DWORD>(sources.size()), reinterpret_cast<DWORD64>(image_base))) {
            free_image_base_on_fail(image_base);
            return std::format("Was unable to register new runtime functions. "
                               "(RtlAddFunctionTable: {})", GetLastError());
        }

        return application_image{
            .image_base = image_base,
            .image_size = total_image_size,
            .function_addresses = function_addresses.get(),
            .runtime_functions = runtime_functions,
            .unwind_info = unwind_info_destination
        };
    }

    std::vector<char> compile_function_body(
        std::uint32_t function_index,
        runs_container& container,
        run_reader<runs_container>::run& function_run,
        memory_layouts_builder::memory_addresses& merged_layouts,
        jump_table_builder& jump_table,
        std::map<std::uint8_t, std::vector<char>>& machine_codes
    ) {
        std::vector<char> function_body_symbols{};

        std::uint32_t instruction_index = 0;
        std::uint64_t translated_address = 0;
        while (function_run.get_run_position() != function_run.get_run_size()) {
            std::pair builder_info{
                create_builder(
                    function_run,
                    merged_layouts,
                    jump_table,
                    container,
                    machine_codes
                )
            };

            if (builder_info.first != nullptr) { //if builder was created correctly
                try {
                    builder_info.first->build();
                }
                catch (const program_compilation_error& exc) {
                    throw program_compilation_error{
                        std::format(
                            "{}: {} On instruction with index {}",
                            builder_info.second,
                            exc.what(),
                            instruction_index
                        ),
                        exc.get_associated_id()
                    };
                }

                const std::vector<char>& translated_instruction = builder_info.first->get_translated_instruction();
                std::ranges::copy( //copy from builder to function_code_container
                    translated_instruction,
                    std::back_inserter(function_body_symbols)
                );

                jump_table.remap_jump_address(function_index, instruction_index, translated_address);

                translated_address += translated_instruction.size();
                ++instruction_index;
            }
            else {
                throw program_compilation_error{
                    std::format(
                        "Unknown instruction/can not create a new builder. Instruction index: {}",
                        instruction_index
                    )
                };
            }
        }

        jump_table.remap_jump_address(function_index, instruction_index, translated_address); //jump-point after last instruction
        if (!jump_table.verify_function_jump_addresses(function_index, instruction_index)) {
            throw program_compilation_error{ "Jump point outside of function body" };
        }

        return function_body_symbols;
    }

    std::pair<std::vector<char>, std::uint32_t> compile_function(
        std::uint32_t function_index,
        runs_container::function& current_function,
        runs_container& container,
        jump_table_builder& jump_table,
        std::map<std::uint8_t, std::vector<char>>& machine_codes,
        std::vector<std::pair<memory_layouts_builder::memory_addresses, memory_layouts_builder::memory_addresses>>& memory_layouts
    ) {
        std::vector<char> compiled_function{};
        std::pair<memory_layouts_builder::memory_addresses, memory_layouts_builder::memory_addresses>& function_memory_layout =
            memory_layouts[function_index];

        std::uint32_t stack_space = calculate_function_locals_stack_space(current_function);
        std::uint32_t function_arguments_size = calculate_function_arguments_stack_space(function_memory_layout.second);

        std::uint32_t prologue_size = generate_function_prologue(
            compiled_function,
            stack_space,
            function_memory_layout.first
        );

        run_reader<runs_container>::run& function_run = current_function.function_body; //get all information associated with current function
        memory_layouts_builder::memory_addresses merged_layouts = memory_layouts_builder::merge_memory_layouts(function_memory_layout);

        std::vector<char> compiled_function_body{};
        try {
            compiled_function_body =
                compile_function_body(function_index, container, function_run, merged_layouts, jump_table, machine_codes);
        }
        catch (const program_compilation_error& exc) {
            auto found_function_name = container.entities_names.find(current_function.function_signature);
            if (found_function_name != container.entities_names.end()) {
                throw program_compilation_error{
                    std::format(
                        "{} in function '{}'.",
                        exc.what(),
                        found_function_name->second
                    ),
                    exc.get_associated_id()
                };
            }

            auto found_exposed_function_name = container.exposed_functions.find(current_function.function_signature);
            if (found_exposed_function_name != container.exposed_functions.end()) {
                throw program_compilation_error{
                    std::format(
                        "{} in function {}",
                        exc.what(),
                        found_exposed_function_name->second
                    ),
                    exc.get_associated_id()
                };
            }

            throw program_compilation_error{
                std::format(
                    "{} in function with id {}.",
                    exc.what(),
                    current_function.function_signature
                ),
                exc.get_associated_id()
            };
        }

        if (compiled_function_body.empty()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Function with id {} has no executable code.",
                    current_function.function_signature
                )
            );
        }

        std::ranges::copy(
            compiled_function_body,
            std::back_inserter(compiled_function)
        );

        generate_function_epilogue(compiled_function, stack_space, function_arguments_size);
        return { compiled_function, prologue_size };
    }

    std::unique_ptr<module_mediator::arguments_string_element[]> create_function_signature(
        const runs_container::function_signature& function_signature
    ) {
        module_mediator::arguments_string_type signature_string = new module_mediator::arguments_string_element[function_signature.argument_types.size() + 2];
        signature_string[0] = static_cast<module_mediator::arguments_string_element>(function_signature.argument_types.size());

        std::size_t signature_string_index = 1;
        for (const auto& argument_type : function_signature.argument_types | std::views::values) {
            signature_string[signature_string_index] =
                module_mediator::arguments_string_builder::convert_program_type_to_arguments_string_type(argument_type);

            ++signature_string_index;
        }

        return std::unique_ptr<module_mediator::arguments_string_element[]>{ signature_string };
    }

    void free_resources_on_fail(application_image& image, std::uint32_t functions_count) {
        if (!RtlDeleteFunctionTable(image.runtime_functions)) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                "Failed to free runtime function table. This may lead to resource leaks."
            );
        }

        delete[] image.function_addresses;
        if (!VirtualFree(image.image_base, 0, MEM_RELEASE)) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                "Failed to free image memory. This may lead to memory leaks."
            );
        }
    }

    compiled_program compile(
        runs_container& container,
        jump_table_builder& jump_table,
        std::map<std::uint8_t, std::vector<char>>& machine_codes,
        std::vector<std::pair<memory_layouts_builder::memory_addresses, memory_layouts_builder::memory_addresses>>& memory_layouts
    ) {
        constexpr std::uint64_t minimum_stack_size = 32;
        if (container.preferred_stack_size < minimum_stack_size) {
            throw program_compilation_error{ std::format("Minimum stack size is {} bytes.", minimum_stack_size) };
        }

        std::uint32_t functions_count = static_cast<std::uint32_t>(container.function_bodies.size());
        if (functions_count == 0) {
            throw program_compilation_error{ "Loaded program has no executable code." };
        }

        if (container.function_signatures.size() != container.function_bodies.size()) {
            throw program_compilation_error{
                std::format(
                    "Loaded program has {} function signatures, but {} functions.",
                    container.function_signatures.size(),
                    container.function_bodies.size()
                )
            };
        }

        // It is declared here so that we can free its resources on failure.
        // We can't really rely on RAII because it uses WinApi functions.
        application_image image{};

        try {
            std::vector<std::vector<char>> compiled_functions{};
            std::vector<uint32_t> function_prologue_sizes{};

            std::uint32_t main_function_index = functions_count;
            for (std::uint32_t function_index = 0; function_index < functions_count; ++function_index) {
                runs_container::function& current_function = container.function_bodies[function_index];
                if (current_function.function_signature == container.main_function_id) {
                    main_function_index = function_index;
                }

                auto [function_body, prologue_size] = compile_function(
                    function_index,
                    current_function,
                    container,
                    jump_table,
                    machine_codes,
                    memory_layouts
                );

                compiled_functions.emplace_back(std::move(function_body));
                function_prologue_sizes.push_back(prologue_size);
            }

            std::unique_ptr<char[]> unwind_info_buffer{ new char[UNWIND_INFO_MAXIMUM_SIZE] {} };
            module_mediator::return_value unwind_info_size = module_mediator::fast_call<
                module_mediator::memory,
                module_mediator::eight_bytes
            >(interoperation::get_module_part(),
                interoperation::index_getter::execution_module(),
                interoperation::index_getter::execution_module_build_unwind_info(),
                unwind_info_buffer.get(), 
                UNWIND_INFO_MAXIMUM_SIZE);

            if (unwind_info_size == module_mediator::module_failure) {
                throw program_compilation_error{ "Failed to build unwind information for the program." };
            }

            auto application_image_or_error = create_application_image(
                compiled_functions, 
                std::unique_ptr<UNWIND_INFO_PROLOGUE>{ 
                    reinterpret_cast<UNWIND_INFO_PROLOGUE*>(
                        std::launder(unwind_info_buffer.release())) 
                },
                unwind_info_size);

            if (std::holds_alternative<std::string>(application_image_or_error)) {
                throw program_compilation_error{
                    std::format("Failed to create application image: {}", 
                        std::get<std::string>(application_image_or_error))
                };
            }

            image = std::get<application_image>(application_image_or_error);
            LOG_PROGRAM_INFO(interoperation::get_module_part(),
                std::format("Built application image at {}, length {}.",
                    std::bit_cast<std::uint64_t>(image.image_base), image.image_size));

            module_mediator::return_value image_verification_result = module_mediator::fast_call<
                module_mediator::memory, module_mediator::eight_bytes,
                module_mediator::memory, module_mediator::four_bytes,
                module_mediator::memory, module_mediator::memory
            >(interoperation::get_module_part(),
                interoperation::index_getter::execution_module(),
                interoperation::index_getter::execution_module_verify_application_image(),
                image.image_base, image.image_size, 
                image.function_addresses, functions_count,
                image.runtime_functions, image.unwind_info);

            if (image_verification_result == module_mediator::module_failure) {
                throw program_compilation_error{ "Failed to verify application image." };
            }

            std::unordered_map<std::uintptr_t, exposed_function_data> loaded_exposed_functions{};
            std::unique_ptr<void* []> exposed_functions_addresses{ 
                new void* [container.exposed_functions.size()] {} 
            };

            for (std::uint32_t function_index = 0; function_index < functions_count; ++function_index) {
                runs_container::function& current_function = container.function_bodies[function_index];
                void* loaded_function = image.function_addresses[function_index];

                auto found_exposed_function_information = container.exposed_functions.find(current_function.function_signature);
                if (found_exposed_function_information != container.exposed_functions.end()) {
                    exposed_functions_addresses[loaded_exposed_functions.size()] = loaded_function;
                    loaded_exposed_functions[reinterpret_cast<std::uintptr_t>(loaded_function)] =
                        exposed_function_data{
                            .function_name = std::move(found_exposed_function_information->second),
                            .function_signature = create_function_signature(
                                container.function_signatures[current_function.function_signature])
                    };
                }

                std::uint32_t prologue_size = function_prologue_sizes[function_index];

                jump_table.add_jump_base_address(function_index, 
                    reinterpret_cast<std::uintptr_t>(loaded_function) + prologue_size);

                jump_table.remap_function_address(current_function.function_signature, 
                    reinterpret_cast<std::uintptr_t>(loaded_function));
            }

            for (std::uint32_t function_index = 0; function_index < container.exposed_functions.size(); ++function_index) {
                if (exposed_functions_addresses[function_index] == nullptr) {
                    throw program_compilation_error{
                        "Was unable to load all exposed functions properly."
                    };
                }
            }

            if (main_function_index >= functions_count) {
                throw program_compilation_error{ std::format("Unknown starting function id specified.") };
            }

            auto [program_strings, program_strings_size] = create_program_strings(container);
            auto [jump_table_address, jump_table_size] = jump_table.create_raw_table(
                reinterpret_cast<std::uintptr_t>(nullptr));

            merge_exposed_functions(loaded_exposed_functions);
            return compiled_program{
                .image_base = image.image_base,
                .runtime_functions = image.runtime_functions,
                .main_function_index = main_function_index,
                .preferred_stack_size = container.preferred_stack_size,
                .compiled_functions = image.function_addresses,
                .functions_count = functions_count,
                .exposed_functions = exposed_functions_addresses.release(),
                .exposed_functions_count = static_cast<uint32_t>(container.exposed_functions.size()),
                .jump_table = jump_table_address,
                .jump_table_size = jump_table_size,
                .program_strings = program_strings,
                .program_strings_count = program_strings_size
            };
        }
        catch (const program_compilation_error&) {
            free_resources_on_fail(image, functions_count);
            throw;
        }
    }
}

module_mediator::return_value load_program_to_memory(module_mediator::arguments_string_type bundle) {
    //program loader(load_program_to_memory)->resource module(create_new_prog_container)->execution module(on_container_creation)

    runs_container container; //load file information to memory
    std::string program_name = std::filesystem::canonical(
        static_cast<const char*>(
            std::get<0>(module_mediator::arguments_string_builder::unpack<void*>(bundle)))
        ).generic_string();

    try {
        LOG_PROGRAM_INFO(
            interoperation::get_module_part(),
            std::format(
                "Starting a new program: \"{}\"...",
                program_name
            )
        );

        // janky use of constructor for its side effects, I don't care about refactoring this 
        run_reader(program_name, &container,
            {
                {0, &runs_container::modules_reader},
                {1, &runs_container::jump_points_reader},
                {2, &runs_container::function_signatures_reader},
                {3, &runs_container::function_bodies_reader},
                {4, &runs_container::exposed_functions_reader},
                {5, &runs_container::program_strings_reader},
                {6, &runs_container::debug_run_reader}
            }
        );

        std::vector memory_layouts{ construct_memory_layout(container) };

        jump_table_builder jump_table{ construct_jump_table(container) };
        std::map machine_codes{ get_machine_codes() };

        compiled_program result = compile(
            container,
            jump_table,
            machine_codes,
            memory_layouts
        );

        LOG_PROGRAM_INFO(
            interoperation::get_module_part(),
            std::format(
                "Successfully compiled \"{}\":" \
                "\n--> Preferred stack size:      {}" \
                "\n--> Total function signatures: {}{}"
                "\n--> Compiled functions count:  {}" \
                "\n--> Exposed functions count:   {}" \
                "\n--> Total loaded strings:      {}" \
                "\n--> Total module dependencies: {}",
                program_name,
                result.preferred_stack_size,
                container.function_signatures.size(),
                container.function_signatures.size() == result.functions_count ? "" : " - DOES NOT MATCH FUNCTIONS COUNT",
                result.functions_count,
                result.exposed_functions_count,
                result.program_strings_count,
                container.modules.size()
            )
        );

        return add_program(
            result.image_base,
            result.runtime_functions,
            result.preferred_stack_size,
            result.main_function_index,
            result.compiled_functions,
            result.functions_count,
            result.exposed_functions,
            result.exposed_functions_count,
            result.jump_table,
            result.jump_table_size,
            result.program_strings,
            result.program_strings_count
        );
    }
    catch (const program_compilation_error &exc) {
        auto found_entity_name = container.entities_names.find(exc.get_associated_id());
        if (found_entity_name != container.entities_names.end()) {
            LOG_ERROR(
                interoperation::get_module_part(),
                std::format(
                    "Failed to compile \"{}\": {} " \
                    "(It appears that this error is related to the following object: '{}')",
                    program_name,
                    exc.what(),
                    found_entity_name->second
                )
            );
        }
        else if (exc.get_associated_id() != 0) { // Id 0 means that the error is not associated with any entity
            LOG_ERROR(
                interoperation::get_module_part(),
                std::format(
                    "Failed to compile \"{}\": {} (The error is associated with the following id: '{}')",
                    program_name,
                    exc.what(),
                    exc.get_associated_id()
                )
            );
        }
        else {
           LOG_ERROR(
                interoperation::get_module_part(),
                std::format(
                    "Failed to compile \"{}\": {}",
                    program_name,
                    exc.what()
                )
           );
        }
    }
    catch ([[maybe_unused]] const std::exception& exc) {
        LOG_ERROR(
            interoperation::get_module_part(),
            std::format(
                "Failed to compile \"{}\": {}",
                program_name,
                exc.what()
            )
        );
    }
    catch (...) {
        LOG_ERROR(
            interoperation::get_module_part(),
            std::format(
                "\"{}\": Program compilation has failed with an unexpected exception.",
                program_name
            )
        );
    }

    LOG_ERROR(
        interoperation::get_module_part(),
        std::format(
            "\"{}\": Program compilation has failed.",
            program_name
        )
    );

    return module_mediator::module_failure;
}

module_mediator::return_value free_program(module_mediator::arguments_string_type bundle) {
    auto [image_base, runtime_function_entries, compiled_functions, compiled_functions_count, 
          exposed_functions_addresses, exposed_functions_count, jump_table, 
          program_strings, program_strings_count] =
        module_mediator::arguments_string_builder::unpack<
            module_mediator::memory, module_mediator::memory,
            module_mediator::memory, std::uint32_t,
            module_mediator::memory, std::uint32_t,
            module_mediator::memory, module_mediator::memory, std::uint64_t
        >(bundle);

    remove_exposed_functions({ 
        static_cast<void**>(exposed_functions_addresses), 
        exposed_functions_count 
    });

    for (std::uint64_t counter = 0; counter < program_strings_count; ++counter) {
        delete[] static_cast<char**>(program_strings)[counter];
    }

    delete[] static_cast<char*>(jump_table);
    delete[] static_cast<void**>(compiled_functions);
    delete[] static_cast<void**>(exposed_functions_addresses);
    delete[] static_cast<char**>(program_strings);

    if (!RtlDeleteFunctionTable(static_cast<PRUNTIME_FUNCTION>(runtime_function_entries))) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            "Failed to remove function table. This may lead to resource leaks."
        );
    }

    if (!VirtualFree(image_base, 0, MEM_RELEASE)) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            "Failed to free image memory. This will lead memory leaks."
        );
    }

    return module_mediator::module_success;
}

