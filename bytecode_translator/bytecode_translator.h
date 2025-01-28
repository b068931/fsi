#ifndef BYTECODE_TRANSLATOR
#define BYTECODE_TRANSLATOR

#include "structure_builder.h"
#include <vector>
#include <limits>
#include <cstdint>

constexpr auto max_functions_count = std::numeric_limits<uint32_t>::max();
constexpr auto max_instructions_count = std::numeric_limits<uint32_t>::max();
constexpr auto max_instruction_arguments_count = 15;
constexpr auto max_name_length = std::numeric_limits<uint8_t>::max();
constexpr auto max_function_arguments_count = std::numeric_limits<uint8_t>::max();

bool check_instructions_arugments(structure_builder::file& file) {
	for (const structure_builder::function& func : file.functions) {
		for (const structure_builder::instruction& inst : func.body) {
			if (inst.operands_in_order.size() > max_instruction_arguments_count) {
				return false;
			}
		}
	}

	return true;
}
bool check_functions_count(structure_builder::file& file) { return file.functions.size() <= max_functions_count; }
bool check_functions_size(structure_builder::file& file) { 
	for (const structure_builder::function& func : file.functions) {
		if (func.body.size() > max_instructions_count) {
			return false;
		}
	}

	return true;
}
std::vector<std::string> check_functions_bodies(structure_builder::file& file) {
	std::vector<std::string> result{};
	for (const structure_builder::function& function : file.functions) {
		if (function.body.size() == 0) {
			result.push_back(function.name);
		}
	}

	return result;
}

//This class will generate logic errors. Programs with logic errors can be translated to byte code but they won't be compiled to machine code
class bytecode_translator {
public:
	//there are four types of runs: modules run(0), jump points run(1), function signatures run(2), function body run(3)
	//run structure: 1 byte: type, 8 bytes: run length.

	enum class error_t {
		name_too_long,
		too_many_function_arguments,
		general_instruction,
		jump_instruction,
		data_instruction,
		var_instruction,
		apply_on_first_operand_instruction,
		same_type_instruction,
		binary_instruction,
		multi_instruction,
		different_type_instruction,
		different_type_multi_instruction,
		empty_instruction,
		pointer_instruction,
		function_call,
		program_function_call,
		module_function_call,
		save_variable_state_instruction,
		load_variable_state_instruction,
		pointer_ref_instruction,
		bit_shift_instruction,
		ctjtd_instruction,
		non_string_instruction,
		string_instruction,
		unknown_instruction
	};

	using entity_id = structure_builder::entity_id;
private:
	template<typename... filters>
	struct create_filter {
	private:
		template<typename filter>
		struct filter_wrapper;

		template<template<typename> class templ, typename filter>
		struct filter_wrapper<templ<filter>> {
			error_t error_message;
			bool check(const structure_builder::instruction& instruction) {
				if (!filter::check(instruction)) { //and only after that we apply our current filter. this way we will not modify error message if one of the filters higher in class hierarchy returns false
					this->error_message = filter::error_message;
					return false;
				}

				return true;
			}
		};

		template<template<typename, typename...> class templ, typename filter, typename... other>
		struct filter_wrapper<templ<filter, other...>> : public filter_wrapper<typename_array::typename_array<other...>> {
			using base_class = filter_wrapper<typename_array::typename_array<other...>>;
			bool check(const structure_builder::instruction& instruction) {
				if (this->base_class::check(instruction)) { //at first we use base class filters
					if (!filter::check(instruction)) { //and only after that we apply our current filter. this way we will not modify error message if one of the filters higher in class hierarchy returns false
						this->error_message = filter::error_message;
						return false;
					}

					return true;
				}

				return false;
			}
		};

	public:
		using type = filter_wrapper<typename_array::typename_array<filters...>>;
	};
	struct general_instruction {
		static constexpr error_t error_message{ error_t::general_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			//at first we make sure that this insturction won't use arguments specific to function calls
			bool function_arguments = (instruction.func_addresses.size() == 0) && (instruction.modules.size() == 0) && (instruction.module_functions.size() == 0);
			if (function_arguments) {
				for (size_t index = 0, size = instruction.operands_in_order.size(); index < size; ++index) {
					if (std::get<2>(instruction.operands_in_order[index])) { //now we make sure that this instruction won't use signed variables
						return false;
					}
				}
				return true;
			}

			return false;
		}
	};
	struct jump_instruction { //jump pnt name - example
		static constexpr error_t error_message{ error_t::jump_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.dereferences.size() == 0) && //jump instructions use only point variables
				(instruction.immediates.size() == 0) &&
				(instruction.jump_variables.size() == 1) &&
				(instruction.operands_in_order.size() == 1);
		}
	};
	struct data_instruction {
		static constexpr error_t error_message{ error_t::data_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.jump_variables.size() == 0);
		}
	};
	struct var_instruction { //var instructions can only dereference pointers and can not use them as pure arguments
		static constexpr error_t error_message{ error_t::var_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			for (size_t index = 0, size = instruction.operands_in_order.size(); index < size; ++index) {
				structure_builder::regular_variable* current_argument = 
					dynamic_cast<structure_builder::regular_variable*>(std::get<1>(instruction.operands_in_order[index]));
				
				if (current_argument && (current_argument->type == structure_builder::source_file_token::pointer_type_keyword)) {
					return false;
				}
			}

			return true;
		}
	};
	struct apply_on_first_operand_instruction {
		static constexpr error_t error_message{ error_t::apply_on_first_operand_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			if (instruction.operands_in_order.size() != 0) {
				return (typeid(*std::get<1>(instruction.operands_in_order[0])) !=
					typeid(structure_builder::imm_variable)) || (instruction.instruction_type == structure_builder::source_file_token::compare_instruction_keyword);
			}

			return false;
		}
	};
	struct same_type_instruction {
		static constexpr error_t error_message{ error_t::same_type_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			if (instruction.operands_in_order.size() >= 2) {
				structure_builder::source_file_token main_active_type = std::get<0>(instruction.operands_in_order[0]);
				for (size_t index = 1, count = instruction.operands_in_order.size(); index < count; ++index) {
					if (main_active_type != std::get<0>(instruction.operands_in_order[index])) {
						return false;
					}
				}

				return true;
			}

			return false;
		}
	};
	struct binary_instruction {
		static constexpr error_t error_message{ error_t::binary_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.operands_in_order.size() == 2);
		}
	};
	struct multi_instruction {
		static constexpr error_t error_message{ error_t::multi_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.operands_in_order.size() <= 15);
		}
	};
	struct different_type_instruction {
		static constexpr error_t error_message{ error_t::different_type_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.immediates.size() == 0);
		}
	};
	struct different_type_multi_instruction {
		static constexpr error_t error_message{ error_t::different_type_multi_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.operands_in_order.size() >= 1) && (instruction.operands_in_order.size() <= 15);
		}
	};
	struct save_variable_state_instruction {
		static constexpr error_t error_message{ error_t::save_variable_state_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.operands_in_order.size() == 1);
		}
	};
	struct load_variable_state_instruction {
		static constexpr error_t error_message{ error_t::load_variable_state_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.immediates.size() == 0);
		}
	};
	struct function_call_instruction {
		static constexpr error_t error_message{ error_t::function_call };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.operands_in_order.size() >= 0) && (instruction.operands_in_order.size() <= max_instruction_arguments_count);
		}
	};
	struct program_function_call_instruction {
		static constexpr error_t error_message{ error_t::program_function_call };
		static bool check(const structure_builder::instruction& instruction) {
			bool program_function_call = (instruction.modules.size() == 0) && (instruction.module_functions.size() == 0);
			if (program_function_call) {
				for (size_t index = 0, size = instruction.operands_in_order.size(); index < size; ++index) {
					if (std::get<2>(instruction.operands_in_order[index])) {
						return false;
					}
				}

				return true;
			}

			return false;
		}
	};
	struct module_function_call_instruction {
		static constexpr error_t error_message{ error_t::module_function_call };
		static bool check(const structure_builder::instruction& instruction) {
			return (instruction.modules.size() == 1) && (instruction.module_functions.size() == 1);
		}
	};
	struct pointer_ref_instruction {
		static constexpr error_t error_message{ error_t::pointer_ref_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			bool initial_check = instruction.immediates.size() == 0;

			if (initial_check) {
				for (size_t index = 0, size = instruction.operands_in_order.size(); index < size; ++index) {
					structure_builder::regular_variable* current_argument =
						dynamic_cast<structure_builder::regular_variable*>(std::get<1>(instruction.operands_in_order[index]));

					if (current_argument && (current_argument->type != structure_builder::source_file_token::pointer_type_keyword)) {
						return false;
					}
				}

				return true;
			}

			return false;
		}
	};
	struct ctjtd_instruction {
		static constexpr error_t error_message{ error_t::ctjtd_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			bool initial_check =
				(instruction.modules.size() == 0) && (instruction.module_functions.size() == 0);

			if (initial_check && (instruction.operands_in_order.size() >= 2)) {
				for (size_t index = 0, size = instruction.operands_in_order.size(); index < size; ++index) {
					if (std::get<2>(instruction.operands_in_order[index])) {
						return false;
					}
				}

				return dynamic_cast<structure_builder::function_address*>(
					std::get<1>(instruction.operands_in_order[1])
				) != nullptr;
			}

			return false;
		}
	};
	struct string_instruction {
		static constexpr error_t error_message{ error_t::string_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			if ((instruction.strings.size() == 1) && (instruction.operands_in_order.size() >= 2)) {
				return (std::get<0>(instruction.operands_in_order[0]) == structure_builder::source_file_token::pointer_type_keyword) &&
					(std::get<0>(instruction.operands_in_order[1]) == structure_builder::source_file_token::string_argument_keyword);
			}

			return false;
		}
	};
	struct non_string_instruction {
		static constexpr error_t error_message{ error_t::non_string_instruction };
		static bool check(const structure_builder::instruction& instruction) {
			return instruction.strings.size() == 0;
		}
	};

	class instruction_check {
	public:
		virtual bool is_match(structure_builder::source_file_token instr) = 0;
		virtual void check_errors(const structure_builder::instruction& instr, std::vector<error_t>& err_list) = 0;
		virtual ~instruction_check() = default;
	};
	template<typename filter_t>
	class generic_instruction_check : public instruction_check {
	private:
		std::vector<structure_builder::source_file_token> instruction_list;
		filter_t filter;

	public:
		generic_instruction_check(std::vector<structure_builder::source_file_token>&& instruction_list, filter_t filter = filter_t{})
			:instruction_list {std::move(instruction_list)},
			filter{filter}
		{}

		virtual bool is_match(structure_builder::source_file_token instr) override {
			return std::find(this->instruction_list.begin(), this->instruction_list.end(), instr) != this->instruction_list.end();
		}
		virtual void check_errors(const structure_builder::instruction& instr, std::vector<error_t>& err_list) {
			if (!this->filter.check(instr)) {
				err_list.push_back(this->filter.error_message);
			}
		}
	};

	void check_logic_errors(std::vector<instruction_check*>& filters_list, const structure_builder::instruction& current_instruction) {
		for (instruction_check* check : filters_list) {
			if (check->is_match(current_instruction.instruction_type)) {
				check->check_errors(current_instruction, this->logic_errors);
				return;
			}
		}

		this->logic_errors.push_back(error_t::unknown_instruction);
	}

	class instruction_encoder : public structure_builder::variable_visitor {
	private:
		std::vector<char> instruction_symbols;
		size_t position_in_type_bytes{2};

		instruction_encoder() = default;
		static uint8_t convert_type_to_uint8(structure_builder::source_file_token token_type) {
			uint8_t type = 0;
			switch (token_type) {
				case structure_builder::source_file_token::one_byte_type_keyword: {
					type = 0;
					break;
				}
				case structure_builder::source_file_token::two_bytes_type_keyword: {
					type = 1;
					break;
				}
				case structure_builder::source_file_token::four_bytes_type_keyword: {
					type = 2;
					break;
				}
				case structure_builder::source_file_token::eight_bytes_type_keyword: {
					type = 3;
					break;
				}
				case structure_builder::source_file_token::no_return_module_call_keyword: {
					type = 1;
					break;
				}
				case structure_builder::source_file_token::pointer_type_keyword: {
					type = 3;
					break;
				}
				default: {
					assert(false && "you should not see this");
					break;
				}
			}

			return type;
		}

		template<typename type>
		void write_bytes(type value) {
			char* bytes = reinterpret_cast<char*>(&value);
			for (size_t counter = 0; counter < sizeof(value); ++counter) {
				this->instruction_symbols.push_back(bytes[counter]);
			}
		}
		void write_id(entity_id id) {
			this->write_bytes<uint64_t>(static_cast<uint64_t>(id));
		}
		void encode_active_type(uint8_t active_type, uint8_t type_mod) {
			uint8_t type_bits =
				((type_mod & 0b11) << 2) | (0b11 & active_type);
			size_t byte_index = (this->position_in_type_bytes / 2 - 1) + 2;
			if ((this->position_in_type_bytes % 2) == 0) {
				type_bits <<= 4;
			}

			++this->position_in_type_bytes;
			this->instruction_symbols[byte_index] |= type_bits;
		}
	public:
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::pointer_dereference* variable, bool is_signed) {
			this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b10);
			this->write_id(variable->pointer_variable->id);

			this->instruction_symbols.push_back(static_cast<char>(variable->derefernce_indexes.size()));
			for (structure_builder::regular_variable* var : variable->derefernce_indexes) {
				this->write_id(var->id);
			}
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::regular_variable* variable, bool is_signed) {
			if (is_signed) {
				this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b00);
			}
			else {
				if (active_type == structure_builder::source_file_token::no_return_module_call_keyword) {
					this->encode_active_type(0b01, 0b11);
				}
				else if (active_type == structure_builder::source_file_token::pointer_type_keyword) {
					this->encode_active_type(0b11, 0b11);
				}
				else {
					this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b10);
				}
			}

			this->write_id(variable->id);
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::imm_variable* variable, bool is_signed) {
			this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b01);
			switch (variable->type) {
				case structure_builder::source_file_token::one_byte_type_keyword: {
					this->write_bytes<uint8_t>(static_cast<uint8_t>(variable->imm_val));
					break;
				}
				case structure_builder::source_file_token::two_bytes_type_keyword: {
					this->write_bytes<uint16_t>(static_cast<uint16_t>(variable->imm_val));
					break;
				}
				case structure_builder::source_file_token::four_bytes_type_keyword: {
					this->write_bytes<uint32_t>(static_cast<uint32_t>(variable->imm_val));
					break;
				}
				case structure_builder::source_file_token::eight_bytes_type_keyword: {
					this->write_bytes<uint64_t>(static_cast<uint64_t>(variable->imm_val));
					break;
				}
			}
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::function_address* variable, bool is_signed) {
			this->encode_active_type(2, 0b11);
			this->write_id(variable->func->id);
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::module_variable* variable, bool is_signed) {
			this->encode_active_type(0, 0b11);
			this->write_id(variable->mod->id);
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::module_function_variable* variable, bool is_signed) {
			this->encode_active_type(2, 0b11);
			this->write_id(variable->func->id);
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::jump_point_variable* variable, bool is_signed) {
			this->encode_active_type(1, 0b11);
			this->write_id(variable->point->id);
		}
		virtual void visit(structure_builder::source_file_token active_type, const structure_builder::string_constant* variable, bool is_signed) override {
			this->encode_active_type(1, 0b11);
			this->write_id(variable->value->id);
		}

		static std::vector<char> encode_instruction(const structure_builder::instruction& current_instruction, const std::map<structure_builder::source_file_token, uint8_t>& operation_codes) {
			instruction_encoder self{};
			bool is_odd = current_instruction.operands_in_order.size() % 2;

			auto found_opcode = operation_codes.find(current_instruction.instruction_type);
			if (found_opcode != operation_codes.end()) {
				self.instruction_symbols.push_back(static_cast<uint8_t>(current_instruction.operands_in_order.size()) << 4); //higher four bits: arguments count, lower four bits: additional type bits if argument count is odd
				self.instruction_symbols.push_back(found_opcode->second);
				self.instruction_symbols.resize(self.instruction_symbols.size() + is_odd + (current_instruction.operands_in_order.size() / 2));

				for (const auto& var : current_instruction.operands_in_order) {
					structure_builder::variable* variable_to_visit = std::get<1>(var);
					if (variable_to_visit) { //if variable_to_visit is nullptr then entity_id equals to 0
						variable_to_visit->visit(
							std::get<0>(var), &self, std::get<2>(var));
					}
					else {
						self.encode_active_type(0b01, 0b11); //do not return from module call
						self.write_id(0);
					}
				}

				if ((current_instruction.operands_in_order.size() % 2) == 1) { //if number of arguments is odd
					size_t additional_type_bits_index = 2 + (current_instruction.operands_in_order.size() / 2); //move half filled type bits to prefix byte
					self.instruction_symbols[0] |= (self.instruction_symbols[additional_type_bits_index] >> 4) & 0b1111;

					self.instruction_symbols.erase(self.instruction_symbols.begin() + additional_type_bits_index);
				}
			}

			return self.instruction_symbols;
		}
	};

private:
	static constexpr uint8_t modules_run = 0;
	static constexpr uint8_t jump_points_run = 1;
	static constexpr uint8_t function_signatures_run = 2;
	static constexpr uint8_t function_body_run = 3;
	static constexpr uint8_t program_exposed_functions_run = 4;
	static constexpr uint8_t program_strings_run = 5;
	static constexpr uint8_t program_debug_run = 6;

	structure_builder::file* file_structure;
	std::ofstream* file_stream;

	std::vector<error_t> logic_errors;
	void add_new_logic_error(error_t err) { this->logic_errors.push_back(err); }

	template<typename type>
	void write_bytes(type value) {
		this->file_stream->write(reinterpret_cast<char*>(&value), sizeof(value));
	}

	void write_n_bytes(const char* bytes, size_t size) {
		this->file_stream->write(bytes, size);
	}
	void write_8_bytes(uint64_t value) {
		this->write_bytes<uint64_t>(value);
	}
	void write_4_bytes(uint32_t value) {
		this->write_bytes<uint32_t>(value);
	}
	void write_2_bytes(uint16_t value) {
		this->write_bytes<uint16_t>(value);
	}
	void write_1_byte(uint8_t value) {
		this->write_bytes<uint8_t>(value);
	}

	static uint8_t convert_type_to_uint8(structure_builder::source_file_token token_type) {
		uint8_t type = 0;
		switch (token_type) {
			case structure_builder::source_file_token::two_bytes_type_keyword: {
				type = 1;
				break;
			}
			case structure_builder::source_file_token::four_bytes_type_keyword: {
				type = 2;
				break;
			}
			case structure_builder::source_file_token::eight_bytes_type_keyword: {
				type = 3;
				break;
			}
			case structure_builder::source_file_token::pointer_type_keyword: {
				type = 4;
				break;
			}
		}

		return type;
	}

	auto write_run_header(uint8_t run_type) {
		this->write_1_byte(run_type);
		auto saved_position = this->file_stream->tellp(); //we will return here later after we calculate this run's size
		
		this->write_8_bytes(0);
		return saved_position;
	}
	void write_run_footer(std::streampos saved_position, uint64_t run_size) {
		this->file_stream->seekp(saved_position); //go back and write this run's size
		this->write_8_bytes(run_size);

		this->file_stream->seekp(0, std::ios::end); //go back to the end of this file
	}

	static std::map<structure_builder::source_file_token, uint8_t> get_operation_codes() {
		return {
			{structure_builder::source_file_token::subtract_instruction_keyword, 0},
			{structure_builder::source_file_token::signed_subtract_instruction_keyword, 1},
			{structure_builder::source_file_token::divide_instruction_keyword, 2},
			{structure_builder::source_file_token::signed_divide_instruction_keyword, 3},
			{structure_builder::source_file_token::compare_instruction_keyword, 4},
			{structure_builder::source_file_token::move_instruction_keyword, 5},
			{structure_builder::source_file_token::add_instruction_keyword, 6},
			{structure_builder::source_file_token::signed_add_instruction_keyword, 7},
			{structure_builder::source_file_token::multiply_instruction_keyword, 8},
			{structure_builder::source_file_token::signed_multiply_instruction_keyword, 9},
			{structure_builder::source_file_token::increment_instruction_keyword, 10},
			{structure_builder::source_file_token::decrement_instruction_keyword, 11},
			{structure_builder::source_file_token::bit_xor_instruction_keyword, 12},
			{structure_builder::source_file_token::bit_and_instruction_keyword, 13},
			{structure_builder::source_file_token::bit_or_instruction_keyword, 14},
			{structure_builder::source_file_token::jump_instruction_keyword, 15},
			{structure_builder::source_file_token::jump_equal_instruction_keyword, 16},
			{structure_builder::source_file_token::jump_not_equal_instruction_keyword, 17},
			{structure_builder::source_file_token::jump_greater_instruction_keyword, 18},
			{structure_builder::source_file_token::jump_greater_equal_instruction_keyword, 19},
			{structure_builder::source_file_token::jump_less_equal_instruction_keyword, 20},
			{structure_builder::source_file_token::jump_less_instruction_keyword, 27},
			{structure_builder::source_file_token::jump_above_instruction_keyword, 21},
			{structure_builder::source_file_token::jump_above_equal_instruction_keyword, 22},
			{structure_builder::source_file_token::jump_below_instruction_keyword, 23},
			{structure_builder::source_file_token::jump_below_equal_instruction_keyword, 24},
			{structure_builder::source_file_token::function_call, 25},
			{structure_builder::source_file_token::module_call, 26},
			{structure_builder::source_file_token::bit_not_instruction_keyword, 28},
			{structure_builder::source_file_token::save_value_instruction_keyword, 29},
			{structure_builder::source_file_token::load_value_instruction_keyword, 30},
			{structure_builder::source_file_token::move_pointer_instruction_keyword, 31},
			{structure_builder::source_file_token::bit_shift_left_instruction_keyword, 32},
			{structure_builder::source_file_token::bit_shift_right_instruction_keyword, 33},
			{structure_builder::source_file_token::get_function_address_instruction_keyword, 34},
			{structure_builder::source_file_token::copy_string_instruction_keyword, 35}
		};
	}
	static std::vector<instruction_check*> get_instruction_filters() {
		using binary_instruction_filter = create_filter<
			general_instruction, data_instruction,
			var_instruction, apply_on_first_operand_instruction,
			same_type_instruction, binary_instruction,
			non_string_instruction>::type;

		using multi_instruction_filter = create_filter<
			general_instruction, data_instruction,
			var_instruction, apply_on_first_operand_instruction,
			same_type_instruction, multi_instruction,
			non_string_instruction>::type;

		using different_type_multi_instruction_filter = create_filter<
			general_instruction, data_instruction,
			var_instruction, different_type_instruction,
			different_type_multi_instruction, non_string_instruction>::type;

		using jump_instruction_filter = create_filter<
			general_instruction, jump_instruction,
			non_string_instruction>::type;

		using save_variable_state_instruction_filter = create_filter<
			general_instruction, data_instruction,
			save_variable_state_instruction, non_string_instruction>::type;

		using load_variable_state_instruction_filter = create_filter<
			general_instruction, data_instruction,
			save_variable_state_instruction, load_variable_state_instruction,
			non_string_instruction>::type;

		using program_function_call_instruction_filter = create_filter<
			function_call_instruction, program_function_call_instruction,
			non_string_instruction>::type;

		using module_function_call_instruction_filter = create_filter<
			function_call_instruction, module_function_call_instruction,
			non_string_instruction>::type;

		using pointer_ref_instruction_filter = create_filter<
			general_instruction, data_instruction,
			binary_instruction, pointer_ref_instruction,
			non_string_instruction>::type;

		using bit_shift_instruction_filter = create_filter<
			general_instruction, data_instruction,
			var_instruction, apply_on_first_operand_instruction,
			binary_instruction, non_string_instruction>::type;

		using ctjtd_instruction_filter = create_filter<
			data_instruction, var_instruction,
			different_type_instruction, binary_instruction,
			ctjtd_instruction, non_string_instruction>::type;

		using string_instruction_filter = create_filter<
			string_instruction, general_instruction,
			data_instruction, different_type_instruction,
			binary_instruction, string_instruction>::type;

		return {
			new generic_instruction_check<binary_instruction_filter>{{
				structure_builder::source_file_token::subtract_instruction_keyword,
				structure_builder::source_file_token::signed_subtract_instruction_keyword,
				structure_builder::source_file_token::divide_instruction_keyword,
				structure_builder::source_file_token::signed_divide_instruction_keyword,
				structure_builder::source_file_token::compare_instruction_keyword,
				structure_builder::source_file_token::move_instruction_keyword,
				structure_builder::source_file_token::bit_and_instruction_keyword,
				structure_builder::source_file_token::bit_or_instruction_keyword,
				structure_builder::source_file_token::bit_xor_instruction_keyword
			}},
			new generic_instruction_check<multi_instruction_filter>{{
				structure_builder::source_file_token::add_instruction_keyword,
				structure_builder::source_file_token::signed_add_instruction_keyword,
				structure_builder::source_file_token::multiply_instruction_keyword,
				structure_builder::source_file_token::signed_multiply_instruction_keyword
			}},
			new generic_instruction_check<different_type_multi_instruction_filter>{{
				structure_builder::source_file_token::increment_instruction_keyword,
				structure_builder::source_file_token::decrement_instruction_keyword,
				structure_builder::source_file_token::bit_not_instruction_keyword
			}},
			new generic_instruction_check<jump_instruction_filter>{{
				structure_builder::source_file_token::jump_instruction_keyword,
				structure_builder::source_file_token::jump_equal_instruction_keyword,
				structure_builder::source_file_token::jump_not_equal_instruction_keyword,
				structure_builder::source_file_token::jump_greater_instruction_keyword,
				structure_builder::source_file_token::jump_greater_equal_instruction_keyword,
				structure_builder::source_file_token::jump_less_instruction_keyword,
				structure_builder::source_file_token::jump_less_equal_instruction_keyword,
				structure_builder::source_file_token::jump_above_instruction_keyword,
				structure_builder::source_file_token::jump_above_equal_instruction_keyword,
				structure_builder::source_file_token::jump_below_instruction_keyword,
				structure_builder::source_file_token::jump_below_equal_instruction_keyword
			}},
			new generic_instruction_check<program_function_call_instruction_filter>{{
				structure_builder::source_file_token::function_call
			}},
			new generic_instruction_check<module_function_call_instruction_filter>{{
				structure_builder::source_file_token::module_call
			}},
			new generic_instruction_check<save_variable_state_instruction_filter>{{
				structure_builder::source_file_token::save_value_instruction_keyword
			}},
			new generic_instruction_check<load_variable_state_instruction_filter>{ {
				structure_builder::source_file_token::load_value_instruction_keyword
			}},
			new generic_instruction_check<pointer_ref_instruction_filter>{{
				structure_builder::source_file_token::move_pointer_instruction_keyword
			}},
			new generic_instruction_check<bit_shift_instruction_filter>{ {
				structure_builder::source_file_token::bit_shift_left_instruction_keyword,
				structure_builder::source_file_token::bit_shift_right_instruction_keyword
			}},
			new generic_instruction_check<ctjtd_instruction_filter>{{
				structure_builder::source_file_token::get_function_address_instruction_keyword
			}},
			new generic_instruction_check<string_instruction_filter>{{
				structure_builder::source_file_token::copy_string_instruction_keyword
			}}
		};
	}

	void create_modules_run() { //8 bytes: modules count, 8 bytes: module's entity_id, 8 bytes: number of functions in this module, 1 byte: module name length, module name, 8 bytes: entity_id, 1 byte module function name length, module function name;
		uint64_t run_size = 8;
		auto saved_position = this->write_run_header(modules_run);

		this->write_8_bytes(static_cast<uint64_t>(this->file_structure->modules.size()));
		for (const structure_builder::module& mod : this->file_structure->modules) {
			this->write_8_bytes(static_cast<uint64_t>(mod.id));
			this->write_8_bytes(static_cast<uint64_t>(mod.functions_names.size()));

			size_t module_name_size = mod.name.size();
			if (module_name_size > max_name_length) {
				this->add_new_logic_error(error_t::name_too_long);
			}

			this->write_1_byte(static_cast<uint8_t>(mod.name.size()));
			this->write_n_bytes(mod.name.c_str(), mod.name.size());
			
			run_size += 17 + module_name_size;
			for (const structure_builder::module_function& mod_fnc : mod.functions_names) {
				this->write_8_bytes(static_cast<uint64_t>(mod_fnc.id));
				this->write_1_byte(static_cast<uint8_t>(mod_fnc.name.size()));

				size_t module_function_name_size = mod_fnc.name.size();
				if (module_function_name_size > max_name_length) {
					this->add_new_logic_error(error_t::name_too_long);
				}

				this->file_stream->write(mod_fnc.name.c_str(), module_function_name_size);
				run_size += 9 + module_function_name_size;
			}
		}

		this->write_run_footer(saved_position, run_size);
	}
	void create_jump_points_run() { //4 bytes: function index, 4 bytes instruction index, 8 bytes: entity_id
		uint64_t run_size = 0;
		auto saved_position = this->write_run_header(jump_points_run);
		
		uint32_t function_index = 0;
		for (structure_builder::function& fnc : this->file_structure->functions) {
			for (const structure_builder::jump_point jmp_point : fnc.jump_points) {
				this->write_4_bytes(function_index);
				this->write_4_bytes(jmp_point.index);
				this->write_8_bytes(static_cast<uint64_t>(jmp_point.id));

				run_size += 16; //8 + 4 + 4
			}

			++function_index;
		}

		this->write_run_footer(saved_position, run_size);
	}
	void create_function_signatures_run() { //4 bytes: signatures count, 8 bytes: entity_id, 1 byte: number of arguments, 1 byte: argument's type, 8 bytes: entity_id. 0 - byte, 1 - dbyte, 2 - fbyte, 3 - ebyte, 4 - pointer
		uint64_t run_size = sizeof(uint32_t);
		auto saved_position = this->write_run_header(function_signatures_run);

		this->write_4_bytes(static_cast<uint32_t>(this->file_structure->functions.size()));
		for (const structure_builder::function& fnc : this->file_structure->functions) {
			this->write_8_bytes(static_cast<uint64_t>(fnc.id));
			size_t function_arguments_count = fnc.arguments.size();
			if (function_arguments_count > max_function_arguments_count) {
				this->add_new_logic_error(error_t::too_many_function_arguments);
			}

			this->write_1_byte(static_cast<uint8_t>(function_arguments_count));
			for (const structure_builder::regular_variable& var: fnc.arguments) {
				this->write_1_byte(bytecode_translator::convert_type_to_uint8(var.type));
				this->write_8_bytes(var.id);

				run_size += 9;
			}

			run_size += 9;
		}

		this->write_run_footer(saved_position, run_size);
	}
	void create_function_body_run(structure_builder::function& func) { //8 bytes: signature's entity_id, 4 bytes: number of local variables, 1 byte: local variable_type, 8 bytes: local variable entity_id
		uint64_t run_size = 12; //8 bytes: signature's entity_id, 4 bytes: number of local variables
		auto saved_position = this->write_run_header(function_body_run);

		this->write_8_bytes(static_cast<uint64_t>(func.id));
		this->write_4_bytes(static_cast<uint32_t>(func.locals.size()));

		for (const structure_builder::regular_variable& var : func.locals) {
			this->write_1_byte(bytecode_translator::convert_type_to_uint8(var.type));
			this->write_8_bytes(var.id);

			run_size += 9;
		}

		std::map<structure_builder::source_file_token, uint8_t> operation_codes{ this->get_operation_codes() };
		std::vector<instruction_check*> filters_list{ this->get_instruction_filters() };
		for (const structure_builder::instruction& current_instruction : func.body) {
			this->check_logic_errors(filters_list, current_instruction);

			std::vector<char> instruction_symbols = instruction_encoder::encode_instruction(current_instruction, operation_codes);
			run_size += instruction_symbols.size();

			this->write_n_bytes(instruction_symbols.data(),
								instruction_symbols.size());
		}
		
		for (instruction_check* check : filters_list) {
			delete check;
		}

		this->write_run_footer(saved_position, run_size);
	}
	void create_exposed_functions_run() { //8 bytes: preferred stack size, 8 bytes: exposed functions count, 8 bytes: exposed function's entity_id, 1 byte: exposed function name size, exposed function name
		uint64_t run_size = 16;
		auto saved_position = this->write_run_header(program_exposed_functions_run);

		structure_builder::function& main_function = this->file_structure->functions.back();
		auto found_main =
			std::find(this->file_structure->exposed_functions.begin(), this->file_structure->exposed_functions.end(), &main_function);
		if (found_main == this->file_structure->exposed_functions.end()) {
			this->file_structure->exposed_functions.push_back(&main_function);
		}

		this->write_8_bytes(static_cast<uint64_t>(this->file_structure->stack_size));
		this->write_8_bytes(static_cast<uint64_t>(this->file_structure->exposed_functions.size()));
		for (structure_builder::function* exposed_function : this->file_structure->exposed_functions) {
			this->write_8_bytes(static_cast<uint64_t>(exposed_function->id));
			this->write_1_byte(static_cast<uint8_t>(exposed_function->name.size()));
			this->write_n_bytes(exposed_function->name.c_str(), exposed_function->name.size());

			run_size += 9 + exposed_function->name.size();
		}

		this->write_run_footer(saved_position, run_size);
	}
	void create_program_strings_run() { //8 bytes - amount of strings, 8 bytes - string id, 8 bytes - string size, string itself
		uint64_t run_size = 8;
		auto saved_position = this->write_run_header(program_strings_run);

		this->write_8_bytes(this->file_structure->program_strings.size());
		for (const auto& key_string : this->file_structure->program_strings) {
			size_t string_size = key_string.second.value.size();

			this->write_8_bytes(static_cast<uint64_t>(key_string.second.id));
			this->write_8_bytes(string_size);

			this->write_n_bytes(key_string.second.value.c_str(), string_size);
			run_size += 16 + string_size;
		}

		this->write_run_footer(saved_position, run_size);
	}

	template<typename T>
	uint64_t generic_write_element(const std::list<T>& list, const std::string& separator, const std::string& prefix_name) {
		uint64_t accumulator = 0;
		for (const T& element : list) {
			uint16_t element_name_size =
				static_cast<uint16_t>(element.name.size() + prefix_name.size() + separator.size());

			std::string combined_name{ prefix_name + separator + element.name };

			this->write_8_bytes(element.id);
			this->write_2_bytes(element_name_size);
			this->write_n_bytes(combined_name.c_str(), element_name_size);

			accumulator += 10 + element_name_size;
		}

		return accumulator;
	}

	void create_debug_run() {
		uint64_t run_size = 0;
		auto saved_position = this->write_run_header(program_debug_run);

		for (const auto& current_string : this->file_structure->program_strings) {
			uint16_t program_string_name_size =
				static_cast<uint16_t>(current_string.first.size());

			run_size += 10 + program_string_name_size;

			this->write_8_bytes(current_string.second.id);
			this->write_2_bytes(program_string_name_size);
			this->write_n_bytes(current_string.first.c_str(), program_string_name_size);
		}

		run_size += this->generic_write_element<structure_builder::module>(
			this->file_structure->modules,
			"",
			""
		);

		for (const structure_builder::module& current_module : this->file_structure->modules) {
			run_size += this->generic_write_element<structure_builder::module_function>(
				current_module.functions_names,
				"(module function)",
				current_module.name
			);
		}

		run_size += this->generic_write_element<structure_builder::function>(
			this->file_structure->functions,
			"",
			""
		);

		for (const structure_builder::function& current_function : this->file_structure->functions) {
			run_size += this->generic_write_element<structure_builder::regular_variable>(
				current_function.arguments,
				"(argument)",
				current_function.name
			);

			run_size += this->generic_write_element<structure_builder::regular_variable>(
				current_function.locals,
				"(local)",
				current_function.name
			);

			run_size += this->generic_write_element<structure_builder::jump_point>(
				current_function.jump_points,
				"(jump point)",
				current_function.name
			);
		}

		this->write_run_footer(saved_position, run_size);
	}

public:
	bytecode_translator(structure_builder::file* file, std::ofstream* file_stream)
		:file_structure{file},
		file_stream{file_stream}
	{}

	void reset_file_structure(structure_builder::file* file) {
		this->file_structure = file;
	}
	void reset_file_stream(std::ofstream* file_stream) {
		this->file_stream = file_stream;
	}

	void start(bool include_debug) {
		this->create_modules_run();
		this->create_jump_points_run();
		this->create_function_signatures_run();
		this->create_exposed_functions_run();
		this->create_program_strings_run();

		if (include_debug) {
			this->create_debug_run();
		}

		for (structure_builder::function& function : this->file_structure->functions) {
			this->create_function_body_run(function);
		}
	}

	const std::vector<error_t>& errors() const { return this->logic_errors; }
};

void translate_error(bytecode_translator::error_t error, std::ostream& stream) {
	switch (error) {
		case bytecode_translator::error_t::general_instruction: {
			stream << "You can not use signed variables and function calls inside general instructions";
			break;
		}
		case bytecode_translator::error_t::jump_instruction: {
			stream << "You can use ONLY one point variable inside 'jump' instructions";
			break;
		}
		case bytecode_translator::error_t::data_instruction: {
			stream << "You can not use point variables inside data instructions";
			break;
		}
		case bytecode_translator::error_t::var_instruction: {
			stream << "You can not use pointer ARGUMENTS inside var instructions";
			break;
		}
		case bytecode_translator::error_t::apply_on_first_operand_instruction: {
			stream << "This instruction changes the value of the first operand, so you can not use immediate as the first operand here.";
			break;
		}
		case bytecode_translator::error_t::same_type_instruction: {
			stream << "You can not use different active types inside same type instructions and you can not use less than two arguments. Also you can not use immediate data as your first argument";
			break;
		}
		case bytecode_translator::error_t::binary_instruction: {
			stream << "Binary instructions use only one pair of arguments";
			break;
		}
		case bytecode_translator::error_t::multi_instruction: {
			stream << "Multi instructions can use up to 15 arguments";
			break;
		}
		case bytecode_translator::error_t::different_type_instruction: {
			stream << "Different type instructions can not use immediates";
			break;
		}
		case bytecode_translator::error_t::different_type_multi_instruction: {
			stream << "Different type multi instructions can use up to 15 arguments";
			break;
		}
		case bytecode_translator::error_t::empty_instruction: {
			stream << "You can not use arguments inside empty instruction";
			break;
		}
		case bytecode_translator::error_t::pointer_instruction: {
			stream << "You can not use more than two arguments inside pointer instruction. Also you need to specify one argument with type POINTER";
			break;
		}
		case bytecode_translator::error_t::function_call: {
			stream << "You can not use more than 250 arguments inside function call instruction";
			break;
		}
		case bytecode_translator::error_t::program_function_call: {
			stream << "You can not use signed variables inside program function call";
			break;
		}
		case bytecode_translator::error_t::module_function_call: {
			stream << "Invalid module function call";
			break;
		}
		case bytecode_translator::error_t::save_variable_state_instruction: {
			stream << "save instruction takes only one argument, excluding signed variables, jump points, etc.";
			break;
		}
		case bytecode_translator::error_t::load_variable_state_instruction: {
			stream << "load instruction takes only one argument, excluding immediates, signed variables, jump points, etc.";
			break;
		}
		case bytecode_translator::error_t::pointer_ref_instruction: {
			stream << "ref instruction must be used to store pointer value from one pointer variable to another or you can use it to store pointer value to another pointer's memory";
			break;
		}
		case bytecode_translator::error_t::ctjtd_instruction: {
			stream << "ctjtd (convert to jump table displacement) instruction must be used with fnc as the second argument while the first argument is the place to store the ebyte displacement";
			break;
		}
		case bytecode_translator::error_t::non_string_instruction: {
			stream << "Only copy_string instruction can use string as the second argument.";
			break;
		}
		case bytecode_translator::error_t::string_instruction: {
			stream << "copy_string instruction uses 'str' as its second argument.";
			break;
		}
		case bytecode_translator::error_t::unknown_instruction: {
			stream << "Unknown instruction";
			break;
		}
		default: {
			stream << "Unknown error code";
			break;
		}
	}
}

#endif