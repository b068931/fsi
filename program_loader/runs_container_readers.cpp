#include "pch.h"
#include "run_container.h"

extern module_mediator::module_part* part;

void runs_container::modules_reader(run_reader<runs_container>::run run) { //every run has its own specific structure, so we need to use different functions for each type of run (see bytecode_translator.h)
	for (std::uint64_t counter = 0, modules_count = run.get_object<std::uint64_t>(); counter < modules_count; ++counter) {
		engine_module new_module{}; //create new module, read its entity_id and read its information about functions

		entity_id module_entityid = run.get_object<entity_id>();
		std::uint64_t number_of_functions = run.get_object<std::uint64_t>();

		const std::string& module_name = this->read_name(run);
		std::size_t found_module = ::part->find_module_index(module_name.c_str()); //now we try to find module's index inside engine_module mediator
		if (found_module != ::part->module_not_found) {
			new_module.module_id = found_module; //add new_module to the list of modules

			for (std::uint64_t functions_counter = 0; functions_counter < number_of_functions; ++functions_counter) {
				entity_id function_entityid = run.get_object<entity_id>();
				const std::string& function_name = this->read_name(run);
				std::size_t found_function = ::part->find_function_index(found_module, function_name.c_str());
				if (found_function != ::part->function_not_found) {
					new_module.module_functions[function_entityid] = found_function;
				}
			}

			this->modules[module_entityid] = std::move(new_module);
		}
	}
}
void runs_container::jump_points_reader(run_reader <runs_container>::run run) {
	for (generic_parser::filepos counter = 0, jump_points_count = run.get_run_size() / (sizeof(std::uint32_t) + sizeof(std::uint32_t) + sizeof(std::uint64_t)); counter < jump_points_count; ++counter) {
		std::uint32_t function_index = run.get_object<std::uint32_t>();
		std::uint32_t instruction_index = run.get_object<std::uint32_t>();
		entity_id id = run.get_object<entity_id>();

		this->jump_points.push_back({ id, function_index, instruction_index });
	}
}
void runs_container::function_signatures_reader(run_reader<runs_container>::run run) {
	for (std::uint64_t counter = 0, signatures_count = run.get_object<std::uint32_t>(); counter < signatures_count; ++counter) {
		entity_id signature_entityid = run.get_object<entity_id>();
		function_signature args{};
		for (std::uint8_t args_counter = 0, args_count = run.get_object<std::uint8_t>(); args_counter < args_count; ++args_counter) {
			std::uint8_t arg_type = run.get_object<std::uint8_t>();
			entity_id arg_entityid = run.get_object<std::uint64_t>();

			args.argument_types.push_back({ arg_entityid, arg_type });
		}

		this->function_signatures[signature_entityid] = std::move(args);
	}
}
void runs_container::exposed_functions_reader(run_reader<runs_container>::run run) {
	this->preferred_stack_size = run.get_object<std::uint64_t>();
	for (std::uint64_t counter = 0, exposed_functions_count = run.get_object<std::uint64_t>(); counter < exposed_functions_count; ++counter) {
		entity_id function_entity_id = run.get_object<entity_id>();
		this->exposed_functions[function_entity_id] = this->read_name(run);
	}
}
void runs_container::program_strings_reader(run_reader<runs_container>::run run) {
	for (std::uint64_t counter = 0, strings_count = run.get_object<std::uint64_t>(); counter < strings_count; ++counter) {
		entity_id string_id = run.get_object<entity_id>();
		std::uint64_t string_size = run.get_object<std::uint64_t>();

		char* string = new char[string_size + 1] {}; //plus zero byte at the end
		for (std::uint64_t string_index = 0; string_index < string_size; ++string_index) {
			string[string_index] = run.get_symbol();
		}

		this->program_strings[string_id] = { std::unique_ptr<char[]>{ string }, string_size + 1 };
	}
}
void runs_container::debug_run_reader(run_reader<runs_container>::run run) {
	while (run.get_run_position() != run.get_run_size()) {
		entity_id object_id = run.get_object<entity_id>();
		std::uint16_t name_size = run.get_object<std::uint16_t>();

		std::string name{};
		for (std::uint16_t counter = 0; counter < name_size; ++counter) {
			name += run.get_symbol();
		}

		this->entities_names[object_id] = std::move(name);
	}
}
void runs_container::function_bodies_reader(run_reader<runs_container>::run run) {
	entity_id signature_id = run.get_object<entity_id>();
	std::vector<std::pair<entity_id, std::uint8_t>> locals;
	for (std::uint32_t counter = 0, locals_count = run.get_object<std::uint32_t>(); counter < locals_count; ++counter) {
		std::uint8_t local_type = run.get_object<std::uint8_t>();
		entity_id local_entityid = run.get_object<entity_id>();
		locals.push_back({ local_entityid, local_type });
	}

	this->function_bodies.push_back({ signature_id, std::move(locals), std::move(run) });
}