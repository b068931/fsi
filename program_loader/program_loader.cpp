#include "pch.h"
#include "program_loader.h"

#include "run_reader.h"
#include "run_container.h"
#include "program_functions.h"
#include "instruction_builder_classes.h"
#include "module_interoperation.h"

#include "jump_table_builder.h"
#include "memory_layouts_builder.h"
#include "compiled_program.h"
#include "program_compilation_error.h"

#include "../logger_module/logging.h"
#include "../module_mediator/fsi_types.h"

namespace {
    std::mutex exposed_functions_mutex{};
    std::unordered_map<std::uintptr_t, std::pair<std::unique_ptr<module_mediator::arguments_string_element[]>, std::string>> exposed_functions{};
    void merge_exposed_functions(std::unordered_map<std::uintptr_t, std::pair<std::unique_ptr<module_mediator::arguments_string_element[]>, std::string>>& new_exposed_functions) {
        std::lock_guard<std::mutex> lock{ ::exposed_functions_mutex };
        ::exposed_functions.merge(new_exposed_functions);
    }
}

namespace {
    module_mediator::return_value add_program(
        std::uint64_t preferred_stack_size, std::uint32_t main_function_index,
        void** code, std::uint32_t functions_count,
        void** exposed_functions_addresses, std::uint32_t exposed_functions_count,
        void* jump_table, std::uint64_t jump_table_size,
        void** program_strings, std::uint64_t program_strings_count
    ) {
        return module_mediator::fast_call<
            module_mediator::eight_bytes, module_mediator::four_bytes,
            module_mediator::memory, module_mediator::four_bytes,
            module_mediator::memory, module_mediator::four_bytes,
            module_mediator::memory, module_mediator::eight_bytes,
            module_mediator::memory, module_mediator::eight_bytes
        >(
            interoperation::get_module_part(),
            interoperation::index_getter::resource_module(),
            interoperation::index_getter::resource_module_create_new_program_container(),
            preferred_stack_size, main_function_index,
            static_cast<void*>(code), functions_count,
            static_cast<void*>(exposed_functions_addresses), exposed_functions_count,
            jump_table, jump_table_size,
            static_cast<void*>(program_strings), program_strings_count
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
            jump_table.add_new_function_address(fnc.function_signature, reinterpret_cast<std::uintptr_t>(get_default_function_address()));
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
            std::pair<std::unique_ptr<instruction_builder>, std::string> builder_info{
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
                throw program_compilation_error{ "Unknown instruction/can not create a new builder." };
            }
        }

        jump_table.remap_jump_address(function_index, instruction_index, translated_address); //jump-point after last instruction
        return function_body_symbols;
    }

    std::pair<void*, std::uint32_t> compile_function(
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
        return { create_executable_function(compiled_function), prologue_size };
    }

    module_mediator::arguments_string_type create_function_signature(const runs_container::function_signature& function_signature) {
        module_mediator::arguments_string_type signature_string = new module_mediator::arguments_string_element[function_signature.argument_types.size() + 2];
        signature_string[0] = static_cast<module_mediator::arguments_string_element>(function_signature.argument_types.size());

        std::size_t signature_string_index = 1;
        for (const auto& argument_type : function_signature.argument_types | std::views::values) {
            signature_string[signature_string_index] =
                module_mediator::arguments_string_builder::convert_program_type_to_arguments_string_type(argument_type);

            ++signature_string_index;
        }

        return signature_string;
    }

    void free_resources_on_fail(std::size_t loaded_functions_size, void** loaded_functions, void** exposed_functions_addresses) {
        delete[] exposed_functions_addresses;
        for (std::size_t index = 0; index < loaded_functions_size; ++index) {
            VirtualFree(loaded_functions[index], 0, MEM_RELEASE);
        }

        delete[] loaded_functions;
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

        void** exposed_functions_addresses = new void* [container.exposed_functions.size()] { nullptr };
        void** loaded_functions_addresses = new void* [functions_count] { nullptr };

        std::uint32_t main_function_index = functions_count;
        try {
            std::unordered_map<std::uintptr_t, std::pair<std::unique_ptr<module_mediator::arguments_string_element[]>, std::string>> loaded_exposed_functions{};
            for (std::uint32_t function_index = 0; function_index < functions_count; ++function_index) {
                runs_container::function& current_function = container.function_bodies[function_index];
                if (current_function.function_signature == container.main_function_id) {
                    main_function_index = function_index;
                }

                auto [loaded_function, prologue_size] = compile_function(
                    function_index,
                    current_function,
                    container,
                    jump_table,
                    machine_codes,
                    memory_layouts
                );

                auto found_exposed_function_information = container.exposed_functions.find(current_function.function_signature);
                if (found_exposed_function_information != container.exposed_functions.end()) {
                    exposed_functions_addresses[loaded_exposed_functions.size()] = loaded_function;
                    loaded_exposed_functions[reinterpret_cast<std::uintptr_t>(loaded_function)] =
                        std::pair<std::unique_ptr<module_mediator::arguments_string_element[]>, std::string>{
                            create_function_signature(container.function_signatures[current_function.function_signature]),
                            std::move(found_exposed_function_information->second)
                    };
                }

                loaded_functions_addresses[function_index] = loaded_function;
                jump_table.add_jump_base_address(static_cast<std::uint32_t>(function_index), reinterpret_cast<std::uintptr_t>(loaded_function) + prologue_size);
                jump_table.remap_function_address(current_function.function_signature, reinterpret_cast<std::uintptr_t>(loaded_function));
            }

            if (main_function_index >= functions_count) {
                throw program_compilation_error{ std::format("Unknown starting function id specified.") };
            }

            std::pair<void*, std::uint64_t> program_jump_table = jump_table.create_raw_table(reinterpret_cast<std::uintptr_t>(get_default_function_address));
            auto [program_strings, program_strings_size] = create_program_strings(container);

            merge_exposed_functions(loaded_exposed_functions);
            return compiled_program{
                .main_function_index = main_function_index,
                .preferred_stack_size = container.preferred_stack_size,
                .compiled_functions = loaded_functions_addresses,
                .functions_count = functions_count,
                .exposed_functions = exposed_functions_addresses,
                .exposed_functions_count = static_cast<uint32_t>(container.exposed_functions.size()),
                .jump_table = program_jump_table.first,
                .jump_table_size = program_jump_table.second,
                .program_strings = program_strings,
                .program_strings_count = program_strings_size
            };
        }
        catch (const program_compilation_error&) {
            free_resources_on_fail(functions_count, loaded_functions_addresses, exposed_functions_addresses);
            throw;
        }
    }
}

module_mediator::return_value load_program_to_memory(module_mediator::arguments_string_type bundle) {
    //program loader(load_program_to_memory)->resource module(create_new_prog_container)->execution module(on_container_creation)

    runs_container container; //load file information to memory
    std::string program_name = std::filesystem::canonical(
        static_cast<char *>(
            std::get<0>(
                module_mediator::arguments_string_builder::unpack<void *>(bundle))
            )
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
        run_reader<runs_container>(program_name, &container,
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

        std::vector<
            std::pair<memory_layouts_builder::memory_addresses, memory_layouts_builder::memory_addresses>
        > memory_layouts{ construct_memory_layout(container) };

        jump_table_builder jump_table{ construct_jump_table(container) };
        std::map<std::uint8_t, std::vector<char>> machine_codes{ get_machine_codes() };

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
    catch (const std::exception &exc) {
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
    auto [compiled_functions, compiled_functions_count, 
          exposed_functions_addresses, exposed_functions_count, jump_table, 
          program_strings, program_strings_count] =
        module_mediator::arguments_string_builder::unpack<
            void*, std::uint32_t,
            void*, std::uint32_t,
            void*, void*, std::uint64_t
        >(bundle);

    {
        std::lock_guard<std::mutex> lock{ ::exposed_functions_mutex };
        for (std::uint32_t counter = 0; counter < exposed_functions_count; ++counter) {
            if (static_cast<void**>(exposed_functions_addresses)[counter] != nullptr) {
                ::exposed_functions.erase(
                    reinterpret_cast<std::uintptr_t>(static_cast<void**>(exposed_functions_addresses)[counter])
                );
            }
        }
    }

    for (std::uint64_t counter = 0; counter < program_strings_count; ++counter) {
        delete[] static_cast<char**>(program_strings)[counter];
    }

    for (std::uint32_t counter = 0; counter < compiled_functions_count; ++counter) {
        VirtualFree(static_cast<void**>(compiled_functions)[counter], 0, MEM_RELEASE);
    }

    delete[] static_cast<char*>(jump_table);
    delete[] static_cast<void**>(compiled_functions);
    delete[] static_cast<void**>(exposed_functions_addresses);
    delete[] static_cast<char**>(program_strings);

    return 0;
}

module_mediator::return_value check_function_arguments(module_mediator::arguments_string_type bundle) {
    auto [signature_string, function_address] =
        module_mediator::arguments_string_builder::unpack<void*, unsigned long long>(bundle);

    //std::unordered_map does not invalidate references and pointers to its objects unless they were erased
    module_mediator::arguments_string_type found_signature_string{};
    {
        std::lock_guard<std::mutex> lock{ ::exposed_functions_mutex };
        auto found_exposed_function_information = ::exposed_functions.find(function_address);
        if (found_exposed_function_information != ::exposed_functions.end()) {
            found_signature_string = found_exposed_function_information->second.first.get();
        }
    }

    if (found_signature_string == module_mediator::arguments_string_type{}) {
        return module_mediator::module_failure; //"function was not found"
    }

    if (module_mediator::arguments_string_builder::check_if_arguments_strings_match(
        static_cast<module_mediator::arguments_string_type>(signature_string), 
        found_signature_string)
        ) {
        return module_mediator::module_success; //"signatures match"
    }

    return module_mediator::module_failure; //"signatures do not match"
}
module_mediator::return_value get_function_name(module_mediator::arguments_string_type bundle) {
    auto [function_address] = 
        module_mediator::arguments_string_builder::unpack<std::uintptr_t>(bundle);

    char* found_name = nullptr;
    {
        std::lock_guard<std::mutex> lock{ ::exposed_functions_mutex };
        auto found_exposed_function_information = ::exposed_functions.find(function_address);
        if (found_exposed_function_information != ::exposed_functions.end()) {
            found_name = found_exposed_function_information->second.second.data();
        }
    }

    return reinterpret_cast<std::uintptr_t>(found_name);
}