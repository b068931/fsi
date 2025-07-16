#ifndef RESOURCE_MODULE_ID_GENERATOR_H
#define RESOURCE_MODULE_ID_GENERATOR_H

#include "pch.h"
#include "../module_mediator/module_part.h"

class id_generator {
public:
	using id_type = module_mediator::return_value;

private:
	inline static std::stack<id_type> free_ids{};
	inline static std::mutex lock{};
	inline static id_type current_id = 1;

public:
	static id_type get_id() {
		std::lock_guard lock{ id_generator::lock };
		if (!free_ids.empty()) {
			id_type id = free_ids.top();
			free_ids.pop();

			return id;
		}

		return current_id++;
	}
	static void free_id(id_type id) {
		std::lock_guard lock{ id_generator::lock };
		free_ids.push(id);
		assert(id != 0);
	}
};

#endif
