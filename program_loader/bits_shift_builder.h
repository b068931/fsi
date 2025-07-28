#ifndef BITS_SHIFT_BUILDER_H
#define BITS_SHIFT_BUILDER_H

#include "apply_right_hand_bits_on_left_hand_binary.h"

class bits_shift_builder : public apply_right_hand_bits_on_left_hand_binary {
private:
	void move_r8_to_rcx() {
		this->write_bytes('\x4c');
		this->write_bytes('\x89');
		this->write_bytes('\xc1');
	}

public:
	template<typename... args>
	bits_shift_builder(
		args&&... instruction_builder_args
	)
		:apply_right_hand_bits_on_left_hand_binary{ std::forward<args>(instruction_builder_args)... }
	{}

	virtual void build() override {
		this->self_call_next();
		this->self_call_next();

		this->move_rcx_to_rbx();
		this->move_r8_to_rcx();

		std::uint8_t rex = 0;
		switch (this->get_active_type())
		{
		case 0b01: {
			this->write_bytes('\x66');
			break;
		}
		case 0b11: {
			rex |= 0b01001000;
			break;
		}
		}

		if (rex != 0) {
			this->write_bytes(rex);
		}

		this->write_bytes<std::uint8_t>(this->get_code(this->get_active_type()));
		this->write_bytes<std::uint8_t>(0 | this->get_code_back() << 3 & 0b00111000);

		this->move_rbx_to_rcx();
	}
};

#endif