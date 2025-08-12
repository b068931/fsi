#include "pch.h"
#include "logger_module.h"
#include "module_interoperation.h"

#include "../module_mediator/module_part.h"

namespace {
	module_mediator::module_part* part = nullptr;
}

module_mediator::module_part* get_module_part() {
	return ::part;
}

extern std::chrono::steady_clock::time_point starting_time;
void initialize_m(module_mediator::module_part* module_part) {
	::part = module_part;
	::starting_time = std::chrono::steady_clock::now();
}