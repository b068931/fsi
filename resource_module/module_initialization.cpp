#include "pch.h"
#include "resource_module.h"
#include "module_interoperation.h"

#include "../module_mediator/module_part.h"

namespace {
	module_mediator::module_part* part = nullptr;
}

namespace interoperation {
	module_mediator::module_part* get_module_part() {
		return ::part;
	}
}

void initialize_m(module_mediator::module_part* module_part) {
	::part = module_part;
}
