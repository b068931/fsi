#ifndef PROGRAM_TERMINATION_CODES_H
#define PROGRAM_TERMINATION_CODES_H

namespace program_loader {
	enum class termination_codes : std::int32_t {
		stack_overflow = 1,
		nullptr_dereference,
		pointer_out_of_bounds,
		undefined_function_call,
		incorrect_saved_variable_type,
		division_by_zero
	};
}

#endif // !PROGRAM_TERMINATION_CODES_H
