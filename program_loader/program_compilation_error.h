#ifndef PROGRAM_COMPILATION_ERROR_H
#define PROGRAM_COMPILATION_ERROR_H

#include <stdexcept>
#include "variable_with_id.h"

class program_compilation_error : public std::logic_error {
private:
	entity_id id_associated_with_error{};

public:
	program_compilation_error(const std::string& message)
		:std::logic_error{ message },
		id_associated_with_error{ 0 }
	{}

	program_compilation_error(const std::string& message, entity_id id_associated_with_error)
		:std::logic_error{ message },
		id_associated_with_error{ id_associated_with_error }
	{}

	entity_id get_associated_id() const {
		return this->id_associated_with_error;
	}
};

#endif