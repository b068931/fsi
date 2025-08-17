#ifndef AUXILIARY_GENERIC_VARIABLE_H
#define AUXILIARY_GENERIC_VARIABLE_H

#include "pch.h"
#include "variable_with_id.h"

class generic_variable : public variable_with_id {
private:
	std::uint8_t real_type;
	std::uint8_t active_type;

public:
	generic_variable(std::uint8_t real_type, std::uint8_t active_type, entity_id id)
		:variable_with_id{ id },
		real_type{ real_type },
		active_type{ active_type }
	{}

	std::uint8_t get_active_type() const { return this->active_type; }
	bool is_valid_active_type() const { return this->active_type <= this->real_type; } //check if active type size is smaller or equal compared to real type size
};

#endif // !AUXILIARY_GENERIC_VARIABLE_H