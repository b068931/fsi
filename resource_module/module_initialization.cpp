#include "pch.h"
#include "resource_module.h"
#include "module_interoperation.h"
#include "../logger_module/logging.h"

#include "../module_mediator/module_part.h"

namespace {
	module_mediator::module_part* part = nullptr;
}

namespace interoperation {
	module_mediator::module_part* get_module_part() {
		return part;
	}
}

void initialize_m(module_mediator::module_part* module_part) {
	part = module_part;
	logger_module::global_logging_instance::set_logging_enabled(true);
}

void free_m() {
	logger_module::global_logging_instance::set_logging_enabled(false);
}
