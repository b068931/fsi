#include "pch.h"
#include "logging.h"

enum class log_type : uint8_t {
	info,
	warning,
	error
};

return_value generic_log_message(arguments_string_type bundle, log_type log) {
	auto arguments = arguments_string_builder::unpack<pointer>(bundle);
	auto [pointer, size] = decay_pointer(std::get<0>(arguments));
	
	size_t log_endpoints[]{ index_getter::logger_program_info(), index_getter::logger_program_warning(), index_getter::logger_program_error() };
	if (pointer == nullptr) {
		log_program_error(get_dll_part(), "Nullpointer passed to logging function.");
		return execution_result_terminate;
	}

	char* data = static_cast<char*>(pointer);
	data[size - 1] = '\0';

	fast_call_module<void*>(
		get_dll_part(),
		index_getter::logger(),
		log_endpoints[static_cast<uint8_t>(log)],
		data
	);

	return execution_result_continue;
}

return_value info(arguments_string_type bundle) {
	return generic_log_message(bundle, log_type::info);
}
return_value warning(arguments_string_type bundle) {
	return generic_log_message(bundle, log_type::warning);
}
return_value error(arguments_string_type bundle) {
	return generic_log_message(bundle, log_type::error);
}