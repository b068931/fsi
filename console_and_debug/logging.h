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

inline void generic_log_message(module_mediator::module_part* part, size_t message_type, std::string message) {
	static size_t logger = part->find_module_index("logger");
	if (logger == module_mediator::module_part::module_not_found) {
		std::cerr << "One of the modules requires uses logging. 'logger' was not found. Terminating with std::abort." << std::endl;
		std::abort();
	}

	static size_t info = part->find_function_index(logger, "info");
	static size_t warning = part->find_function_index(logger, "warning");
	static size_t error = part->find_function_index(logger, "error");
	static size_t fatal = part->find_function_index(logger, "fatal");

	static size_t program_info = part->find_function_index(logger, "program_info");
	static size_t program_warning = part->find_function_index(logger, "program_warning");
	static size_t program_error = part->find_function_index(logger, "program_error");
	static size_t program_fatal = part->find_function_index(logger, "program_fatal");

	size_t logger_indexes[]{ info, warning, error, fatal, program_info, program_warning, program_error, program_fatal };
	assert(message_type < 8);

	size_t log_type = logger_indexes[message_type];
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