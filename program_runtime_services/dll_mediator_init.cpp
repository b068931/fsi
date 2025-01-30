#include "pch.h"
#include "declarations.h"
#include "module_mediator_declarations.h"

module_mediator::module_part* part = nullptr;
module_mediator::module_part* get_module_part() {
	return ::part;
}

void initialize_m(module_mediator::module_part* part) {
	::part = part;
}