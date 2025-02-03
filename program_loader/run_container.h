#ifndef RUN_CONTAINER_H
#define RUN_CONTAINER_H

#include <unordered_map>
#include <map>
#include <vector>

#include "variable_with_id.h"
#include "run_reader.h"

struct runs_container {
public:
	struct engine_module {
		std::size_t module_id{};
		std::map<entity_id, std::size_t> module_functions;
	};

	struct function_signature {
		std::vector<std::pair<entity_id, std::uint8_t>> argument_types;
	};

	struct function {
		entity_id function_signature{};
		std::vector<std::pair<entity_id, std::uint8_t>> locals;

		run_reader<runs_container>::run function_body;
	};

	entity_id main_function_id{};
	std::uint64_t preferred_stack_size{};
	std::map<entity_id, std::string> entities_names{};

	std::map<entity_id, engine_module> modules;
	std::vector<std::tuple<entity_id, std::uint32_t, std::uint32_t>> jump_points;
	std::map<entity_id, std::pair<std::unique_ptr<char[]>, std::size_t>> program_strings;

	std::map<entity_id, function_signature> function_signatures;
	std::map<entity_id, std::string> exposed_functions;
	std::vector<function> function_bodies;

	void modules_reader(run_reader<runs_container>::run);
	void jump_points_reader(run_reader<runs_container>::run);
	void function_signatures_reader(run_reader<runs_container>::run);
	void exposed_functions_reader(run_reader<runs_container>::run);
	void program_strings_reader(run_reader<runs_container>::run);
	void debug_run_reader(run_reader<runs_container>::run);
	void function_bodies_reader(run_reader<runs_container>::run);

private:
	std::string read_name(run_reader<runs_container>::run& run) {
		std::string name{};
		for (std::uint8_t name_counter = 0, name_length = run.get_object<std::uint8_t>(); name_counter < name_length; ++name_counter) {
			name += run.get_symbol();
		}

		return name;
	}
};

#endif // !RUN_CONTAINER_H