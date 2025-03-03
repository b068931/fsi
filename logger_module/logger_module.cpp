#include "pch.h"
#include "logger_module.h"
#include "module_interoperation.h"

std::chrono::steady_clock::time_point starting_time;
std::mutex global_debug_output_lock;

enum class message_type {
	info,
	warning,
	error,
	fatal
};

void decode_message_type(message_type type) {
	std::cerr << '[';
	switch (type) {
	case message_type::info:
		std::cerr << "\x1b[34m" << "INFO" << "\033[0m";
		break;

	case message_type::warning:
		std::cerr << "\x1b[33m" << "WARNING" << "\033[0m";
		break;

	case message_type::error:
		std::cerr << "\x1b[91m" << "ERROR" << "\033[0m";
		break;

	case message_type::fatal:
		std::cerr << "\x1b[31m" << "FATAL" << "\033[0m";
		break;
	}
	std::cerr << ']';
}
void log_message(message_type type, const char* file_name, std::size_t file_line, const char* function_name, const char* message) {
	auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - ::starting_time).count();
	std::string result_message = std::format(
		" [TIME: {}] [FILE NAME: {}, FILE LINE: {}, FUNCTION NAME: {}] \x1b[97m{}\033[0m",
		timestamp,
		(file_name == nullptr) ? "UNSPECIFIED" : file_name,
		(file_line == 0) ? "UNSPECIFIED" : std::to_string(file_line),
		(function_name == nullptr) ? "UNSPECIFIED" : function_name,
		message
	);

	std::lock_guard<std::mutex> lock{ ::global_debug_output_lock };
	decode_message_type(type);
	std::cerr << result_message << std::endl;
}

void generic_log_message(message_type type, module_mediator::arguments_string_type bundle) {
	auto [file_name, file_line, function_name, message] = 
		module_mediator::arguments_string_builder::unpack<void*, module_mediator::return_value, void*, void*>(bundle);

	log_message(
		type,
		static_cast<char*>(file_name),
		static_cast<std::size_t>(file_line),
		static_cast<char*>(function_name),
		static_cast<char*>(message)
	);
}
void generic_log_message_with_thread_information(message_type type, module_mediator::arguments_string_type bundle) {
	auto [file_name, file_line, function_name, message] = 
		module_mediator::arguments_string_builder::unpack<void*, module_mediator::return_value, void*, void*>(bundle);

	module_mediator::return_value current_thread_id = module_mediator::fast_call(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_get_current_thread_id()
	);

	module_mediator::return_value current_thread_group_id = module_mediator::fast_call(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_get_current_thread_group_id()
	);

	std::stringstream stream{};
	if ((current_thread_id == 0) && (current_thread_group_id == 0)) {
		stream << "[ENGINE";
	}
	else {
		stream << "[THREAD: " << current_thread_id;
		stream << ", THREAD GROUP: " << current_thread_group_id;
	}

	stream << "] " << static_cast<char*>(message);
	log_message(
		type, 
		static_cast<char*>(file_name),
		static_cast<std::size_t>(file_line),
		static_cast<char*>(function_name),
		stream.str().c_str()
	);
}

module_mediator::return_value info(module_mediator::arguments_string_type bundle) {
	generic_log_message(message_type::info, bundle);
	return 0;
}
module_mediator::return_value warning(module_mediator::arguments_string_type bundle) {
	generic_log_message(message_type::warning, bundle);
	return 0;
}
module_mediator::return_value error(module_mediator::arguments_string_type bundle) {
	generic_log_message(message_type::error, bundle);
	return 0;
}
module_mediator::return_value fatal(module_mediator::arguments_string_type bundle) {
	generic_log_message(message_type::fatal, bundle);
	return 0;
}

module_mediator::return_value program_info(module_mediator::arguments_string_type bundle) {
	generic_log_message_with_thread_information(message_type::info, bundle);
	return 0;
}
module_mediator::return_value program_warning(module_mediator::arguments_string_type bundle) {
	generic_log_message_with_thread_information(message_type::warning, bundle);
	return 0;
}
module_mediator::return_value program_error(module_mediator::arguments_string_type bundle) {
	generic_log_message_with_thread_information(message_type::error, bundle);
	return 0;
}
module_mediator::return_value program_fatal(module_mediator::arguments_string_type bundle) {
	generic_log_message_with_thread_information(message_type::fatal, bundle);
	return 0;
}
