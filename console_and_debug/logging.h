#ifndef MODULES_LOGGING_H
#define MODULES_LOGGING_H

#ifdef DISABLE_LOGGING
#define log_info(part, message) ((void)0)
#define log_warning(part, message) ((void)0)
#define log_error(part, message) ((void)0)
#define log_fatal(part, message) ((void)0)

#define log_program_info(part, message) ((void)0)
#define log_program_warning(part, message) ((void)0)
#define log_program_error(part, message) ((void)0)
#define log_program_fatal(part, message) ((void)0)
#else

#include <string>
#include <iostream>
#include "../module_mediator/module_part.h"

inline void generic_log_message(module_mediator::module_part* part, std::size_t message_type, std::string message) {
	static std::size_t logger = part->find_module_index("logger");
	if (logger == module_mediator::module_part::module_not_found) {
		std::cerr << "One of the modules requires uses logging. 'logger' was not found. Terminating with std::abort." << std::endl;
		std::abort();
	}

	static std::size_t info = part->find_function_index(logger, "info");
	static std::size_t warning = part->find_function_index(logger, "warning");
	static std::size_t error = part->find_function_index(logger, "error");
	static std::size_t fatal = part->find_function_index(logger, "fatal");

	static std::size_t program_info = part->find_function_index(logger, "program_info");
	static std::size_t program_warning = part->find_function_index(logger, "program_warning");
	static std::size_t program_error = part->find_function_index(logger, "program_error");
	static std::size_t program_fatal = part->find_function_index(logger, "program_fatal");

	std::size_t logger_indexes[]{ info, warning, error, fatal, program_info, program_warning, program_error, program_fatal };
	assert(message_type < 8);

	std::size_t log_type = logger_indexes[message_type];
	if (log_type == module_mediator::module_part::function_not_found) {
		std::cerr << "One of the required logging functions was not found. Terminating with std::abort." << std::endl;
		std::abort();
	}

	module_mediator::fast_call<void*>(part, logger, log_type, message.data());
}

#define log_info(part, message) generic_log_message(part, 0, message)
#define log_warning(part, message) generic_log_message(part, 1, message)
#define log_error(part, message) generic_log_message(part, 2, message)
#define log_fatal(part, message) generic_log_message(part, 3, message)

#define log_program_info(part, message) generic_log_message(part, 4, message)
#define log_program_warning(part, message) generic_log_message(part, 5, message)
#define log_program_error(part, message) generic_log_message(part, 6, message)
#define log_program_fatal(part, message) generic_log_message(part, 7, message)
#endif

#endif