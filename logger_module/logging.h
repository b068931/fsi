#ifndef MODULES_LOGGING_H
#define MODULES_LOGGING_H

#ifdef DISABLE_LOGGING
#define LOG_INFO(part, message) ((void)0)
#define LOG_WARNING(part, message) ((void)0)
#define LOG_ERROR(part, message) ((void)0)
#define LOG_FATAL(part, message) ((void)0)

#define LOG_PROGRAM_INFO(part, message) ((void)0)
#define LOG_PROGRAM_WARNING(part, message) ((void)0)
#define LOG_PROGRAM_ERROR(part, message) ((void)0)
#define LOG_PROGRAM_FATAL(part, message) ((void)0)
#else

#include <string>
#include <iostream>

#include "../module_mediator/fsi_types.h"
#include "../module_mediator/module_part.h"

#define ONLY_FILE_NAME strrchr("\\" __FILE__, '\\') + 1 

namespace logger_module {
	inline void generic_log_message(
		module_mediator::module_part* part,
		std::size_t message_type,
		std::string file_name,
		std::size_t file_line,
		std::string function_name,
		std::string message
	) {
		assert(part != nullptr && "Null module part");
		assert(!message.empty() && "Empty log message");

		static std::size_t logger = part->find_module_index("logger");
		if (logger == module_mediator::module_part::module_not_found) {
			std::cerr << "One of the modules uses logging. 'logger' was not found. Terminating with std::abort." << std::endl;
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

		module_mediator::fast_call<
			module_mediator::memory,
	        module_mediator::return_value,
	        module_mediator::memory,
	        module_mediator::memory
	    >(
			part,
			logger,
			log_type,
			file_name.data(),
			file_line,
			function_name.data(),
			message.data()
		);
	}
}

#define LOG_INFO(part, message) logger_module::generic_log_message(part, 0, ONLY_FILE_NAME, __LINE__, __func__, message)
#define LOG_WARNING(part, message) logger_module::generic_log_message(part, 1, ONLY_FILE_NAME, __LINE__, __func__, message)
#define LOG_ERROR(part, message) logger_module::generic_log_message(part, 2, ONLY_FILE_NAME, __LINE__, __func__, message)
#define LOG_FATAL(part, message) logger_module::generic_log_message(part, 3, ONLY_FILE_NAME, __LINE__, __func__, message)

#define LOG_PROGRAM_INFO(part, message) logger_module::generic_log_message(part, 4, ONLY_FILE_NAME, __LINE__, __func__, message)
#define LOG_PROGRAM_WARNING(part, message) logger_module::generic_log_message(part, 5, ONLY_FILE_NAME, __LINE__, __func__, message)
#define LOG_PROGRAM_ERROR(part, message) logger_module::generic_log_message(part, 6, ONLY_FILE_NAME, __LINE__, __func__, message)
#define LOG_PROGRAM_FATAL(part, message) logger_module::generic_log_message(part, 7, ONLY_FILE_NAME, __LINE__, __func__, message)
#endif

#endif