#include "pch.h"
#include "console_and_debug.h"

std::mutex global_output_lock;

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
void log_message(message_type type, const char* message) {
	std::lock_guard<std::mutex> lock{::global_output_lock};
	
	decode_message_type(type);
	std::cerr << ' ' << "\x1b[97m" << message << "\033[0m" << std::endl;
}

void generic_log_message(message_type type, module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<void*>(bundle);
	log_message(
		type,
		static_cast<char*>(std::get<0>(arguments))
	);
}
void generic_log_message_with_thread_information(message_type type, module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<void*>(bundle);
	char* message = static_cast<char*>(std::get<0>(arguments));

	module_mediator::return_value current_thread_id = module_mediator::fast_call(
		::part,
		index_getter::excm(),
		index_getter::excm_get_current_thread_id()
	);

	module_mediator::return_value current_thread_group_id = module_mediator::fast_call(
		::part,
		index_getter::excm(),
		index_getter::excm_get_current_thread_group_id()
	);

	std::stringstream stream{};
	if ((current_thread_id == 0) && (current_thread_group_id == 0)) {
		stream << "[ENGINE";
	}
	else {
		stream << "[thread: " << current_thread_id;
		stream << ", thread group: " << current_thread_group_id;
	}

	stream << "] " << message;
	log_message(type, stream.str().c_str());
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

void initialize_m(module_mediator::module_part* part) {
	::part = part;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Unable to set virtual terminal mode. Output may look strange." << std::endl;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		std::cerr << "Unable to set virtual terminal mode. Output may look strange." << std::endl;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		std::cerr << "Unable to set virtual terminal mode. Output may look strange." << std::endl;
	}
}