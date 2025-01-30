#include "pch.h"
#include "logging.h"

enum class log_type : std::uint8_t {
	info,
	warning,
	error
};

module_mediator::return_value generic_log_message(module_mediator::arguments_string_type bundle, log_type log) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer>(bundle);
	auto [pointer, size] = decay_pointer(std::get<0>(arguments));
	
	std::size_t log_endpoints[]{ index_getter::logger_program_info(), index_getter::logger_program_warning(), index_getter::logger_program_error() };
	if (pointer == nullptr) {
		log_program_error(get_module_part(), "Nullpointer passed to logging function.");
		return module_mediator::execution_result_terminate;
	}

	char* data = static_cast<char*>(pointer);
	data[size - 1] = '\0';

	module_mediator::fast_call<void*>(
		get_module_part(),
		index_getter::logger(),
		log_endpoints[static_cast<std::uint8_t>(log)],
		data
	);

	return module_mediator::execution_result_continue;
}

module_mediator::return_value info(module_mediator::arguments_string_type bundle) {
	return generic_log_message(bundle, log_type::info);
}
module_mediator::return_value warning(module_mediator::arguments_string_type bundle) {
	return generic_log_message(bundle, log_type::warning);
}
module_mediator::return_value error(module_mediator::arguments_string_type bundle) {
	return generic_log_message(bundle, log_type::error);
}