#ifndef INSTRUCTION_BUILDER_H
#define INSTRUCTION_BUILDER_H

#include "variable_with_id.h" //entity_id
#include "run_reader.h"
#include "run_container.h"
#include "instruction_arguments_classes.h"
#include "functions.h"
#include "jump_table_builder.h"
#include "memory_layouts_builder.h"
#include "program_compilation_error.h"

#include <utility> //std::pair
#include <cstdint> //uintX_t, intX_t types
#include <vector>
#include <map>
#include <cassert>

class instruction_builder {
private:
	std::uint8_t current_variable_type;
	std::uint8_t argument_index;

	run_reader<runs_container>::run& run;
	std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>& function_memory_layout;

	std::vector<std::uint8_t> instruction_types;
	std::vector<std::unique_ptr<variable>> type_objects;

	std::vector<char> translated_instruction_symbols;
	jump_table_builder& jump_table;

	runs_container& general_file_information;

	template<typename T>
	void generic_create_imm() {
		this->type_objects.push_back(
			std::unique_ptr<variable>{
				new variable_imm<T>{ this->run.get_object<T>() }
			}
		);
	}

	template<typename T>
	void generic_create_object() {
		this->type_objects.push_back(
			std::unique_ptr<variable>{
				new T{ this->run.get_object<entity_id>() }
			}
		);
	}

	void add_new_type_object(std::uint8_t type_bits) {
		std::uint8_t type_group = (type_bits >> 2) & 0b11;
		std::uint8_t active_type = type_bits & 0b11;

		if (type_group == 0b00) {
			entity_id id = this->run.get_object<entity_id>();
			this->type_objects.push_back(
				std::unique_ptr<variable>{
					new signed_variable(this->function_memory_layout[id].second, active_type, id)
				}
			);
		}
		else if (type_group == 0b10) {
			entity_id id = this->run.get_object<entity_id>();
			std::uint8_t real_type = this->function_memory_layout[id].second;

			if (real_type == 4) { //if used variable has POINTER type
				std::uint8_t dereferenced_variables_count = this->run.get_object<std::uint8_t>();
				std::vector<entity_id> dereference_indexes;
				for (std::uint8_t counter = 0; counter < dereferenced_variables_count; ++counter) {
					dereference_indexes.push_back(this->run.get_object<entity_id>());
				}

				this->type_objects.push_back(
					std::unique_ptr<variable>{
						new dereferenced_pointer(active_type, id, std::move(dereference_indexes))
					}
				);
			}
			else {
				this->type_objects.push_back(
					std::unique_ptr<variable>{
						new regular_variable(real_type, active_type, id)
					}
				);
			}
		}
		else {
			std::map<std::uint8_t, void(instruction_builder::*)()> object_create_map{
				{ 0b0100, & instruction_builder::generic_create_imm<std::uint8_t> },
				{ 0b0101, &instruction_builder::generic_create_imm<std::uint16_t> },
				{ 0b0110, &instruction_builder::generic_create_imm<std::uint32_t> },
				{ 0b0111, &instruction_builder::generic_create_imm<std::uint64_t> },
				{ 0b1100, &instruction_builder::generic_create_object<module> },
				{ 0b1101, &instruction_builder::generic_create_object<specialized_variable> },
				{ 0b1110, &instruction_builder::generic_create_object<function> },
				{ 0b1111, &instruction_builder::generic_create_object<pointer> }
			};

			(this->*(object_create_map[type_bits]))();
		}
	}
	void create_new_type(std::uint8_t type_bits) {
		this->instruction_types.push_back(type_bits);
		this->add_new_type_object(type_bits);
	}

	std::uint8_t find_type_index(std::uint8_t type_bits) {
		for (std::uint8_t index = 0, instruction_types_count = static_cast<std::uint8_t>(this->instruction_types.size()); index < instruction_types_count; ++index) {
			if (type_bits == this->instruction_types[index]) {
				return index;
			}
		}

		this->assert_statement(false, "Instruction must have at least one argument with specific type.");
		return 0;
	}
	void self_call_by_index(std::uint8_t index) {
		this->current_variable_type = this->instruction_types[index];

		this->type_objects[index].release()->visit(this);

		this->type_objects.erase(this->type_objects.begin() + index);
		this->instruction_types.erase(this->instruction_types.begin() + index);

		this->increment_argument_index();
	}

	void write_variable_relative_address(generic_variable* variable, std::int32_t additional_displacement) {
		this->write_bytes<std::int32_t>(
			this->get_variable_info(variable->get_id()).first - additional_displacement
		);
	}
	void increment_argument_index() { ++this->argument_index; }
protected:
	instruction_builder(
		std::uint8_t instruction_prefix,
		run_reader<runs_container>::run& run,
		std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>& function_memory_layout,
		jump_table_builder& jump_table,
		runs_container& general_file_information
	)
		:run{ run },
		function_memory_layout{ function_memory_layout },
		jump_table{ jump_table },
		general_file_information{ general_file_information },
		argument_index{ 0 },
		current_variable_type{}
	{
		std::uint8_t instruction_arguments_count = (instruction_prefix >> 4) & 0b1111;
		for (std::uint8_t counter = 0, type_bytes_count = instruction_arguments_count / 2; counter < type_bytes_count; ++counter) {
			std::uint8_t type_byte = run.get_object<std::uint8_t>();
			std::uint8_t first_type_bits = (type_byte >> 4) & 0b1111;
			std::uint8_t second_type_bits = type_byte & 0b1111;

			this->instruction_types.push_back(first_type_bits);
			this->instruction_types.push_back(second_type_bits);
		}

		for (std::uint8_t type_bits : this->instruction_types) {
			this->add_new_type_object(type_bits);
		}

		if (instruction_arguments_count % 2 == 1) { //if arguments count is odd
			this->create_new_type(instruction_prefix & 0b1111);
		}
	}

	void generate_stack_allocation_code(std::uint32_t size) {
		::generate_stack_allocation_code(this->translated_instruction_symbols, size);
	}
	void generate_stack_deallocation_code(std::uint32_t size) {
		::generate_stack_deallocation_code(this->translated_instruction_symbols, size);
	}
	void generate_program_termination_code(termination_codes error_code) {
		::generate_program_termination_code(this->translated_instruction_symbols, error_code);
	}
	void save_type_to_program_stack(std::uint8_t active_type) {
		this->write_bytes('\x41'); //mov byte ptr [r9 + 8], active_type
		this->write_bytes('\xc6');
		this->write_bytes('\x41');
		this->write_bytes('\x08');
		this->write_bytes(active_type);
	}

	std::uint8_t get_current_variable_type() const { return this->current_variable_type; }
	const runs_container& get_file_info() const { return this->general_file_information; }

	template<typename T>
	void write_bytes(T value) {
		::write_bytes(value, this->translated_instruction_symbols);
	}

	void zero_rax() {
		this->translated_instruction_symbols.push_back('\x48');
		this->translated_instruction_symbols.push_back('\x33');
		this->translated_instruction_symbols.push_back('\xc0');
	}
	void zero_r8() {
		this->translated_instruction_symbols.push_back('\x4d');
		this->translated_instruction_symbols.push_back('\x33');
		this->translated_instruction_symbols.push_back('\xc0');
	}
	void zero_r15() {
		this->translated_instruction_symbols.push_back('\x4d');
		this->translated_instruction_symbols.push_back('\x33');
		this->translated_instruction_symbols.push_back('\xff');
	}
	void zero_rdx() {
		this->write_bytes('\x48');
		this->write_bytes('\x31');
		this->write_bytes('\xd2');
	}

	void move_rcx_to_rbx() {
		this->write_bytes('\x48');
		this->write_bytes('\x89');
		this->write_bytes('\xcb');
	}
	void move_rbx_to_rcx() {
		this->write_bytes('\x48');
		this->write_bytes('\x89');
		this->write_bytes('\xd9');
	}

	void use_r8_on_reg(std::uint8_t opcode, bool is_dereference, std::uint8_t active_type, bool is_R = false, std::uint8_t reg = 0) {
		std::uint8_t rex = 0x41;

		switch (active_type) {
		case 0b01: { //add prefix that indicates that this is 16 bit instruction
			this->translated_instruction_symbols.push_back('\x66');
			break;
		}
		case 0b11: { //add REX prefix
			rex |= 0b01001000;
			break;
		}
		}

		if (is_R) {
			rex |= 0b01000100;
		}

		this->translated_instruction_symbols.push_back(static_cast<char>(rex)); //rex
		this->translated_instruction_symbols.push_back(static_cast<char>(opcode));

		if (is_dereference) {
			this->translated_instruction_symbols.push_back(0 | ((reg << 3) & 0b00111000));
		}
		else {
			this->translated_instruction_symbols.push_back('\xc0' | ((reg << 3) & 0b00111000)); //r/m
		}
	}
	void use_r8_on_reg_with_two_opcodes(std::uint8_t opcode00, std::uint8_t opcode, bool is_dereference, std::uint8_t active_type, bool is_R = false, std::uint8_t reg = 0) {
		if (active_type == 0b00) {
			this->use_r8_on_reg(opcode00, is_dereference, 0b00, is_R, reg);
		}
		else {
			this->use_r8_on_reg(opcode, is_dereference, active_type, is_R, reg);
		}
	}

	void create_variable_instruction_with_two_opcodes(std::uint8_t opcode00, std::uint8_t opcode, bool rex_R, generic_variable* variable, std::uint8_t reg = 0, std::uint32_t additional_displacement = 0) {
		if (variable->get_active_type() == 0b00) {
			this->create_variable_instruction(opcode00, rex_R, variable, reg, additional_displacement);
		}
		else {
			this->create_variable_instruction(opcode, rex_R, variable, reg, additional_displacement);
		}
	}
	void create_variable_instruction(std::uint8_t opcode, bool rex_R, generic_variable* variable, std::uint8_t reg = 0, std::uint32_t additional_displacement = 0) { //rax_or_r8 = rex_R
		std::uint8_t rex = 0;

		switch (variable->get_active_type()) {
		case 0b01: { //add prefix that indicates that this is 16 bit instruction
			this->translated_instruction_symbols.push_back('\x66');
			break;
		}
		case 0b11: { //add REX prefix
			rex |= 0b01001000;
			break;
		}
		}

		if (rex_R) {
			rex |= 0b01000100;
		}

		if (rex != 0) {
			this->translated_instruction_symbols.push_back(static_cast<char>(rex));
		}

		this->translated_instruction_symbols.push_back(static_cast<char>(opcode));
		this->translated_instruction_symbols.push_back('\x85' | ((reg << 3) & 0b00111000)); //r/m

		this->write_variable_relative_address(variable, additional_displacement);
	}
	void load_variable_address(entity_id id, bool is_R = false) {
		auto variable_info = this->get_variable_info(id);
		if (is_R) { //lea rax/r8, [rbp+disp32]
			this->write_bytes('\x4c');
		}
		else {
			this->write_bytes('\x48');
		}
		this->write_bytes('\x8d');
		this->write_bytes('\x85');
		this->write_bytes(variable_info.first);
	}

	void load_pointer_info(std::pair<std::int32_t, std::uint8_t> variable_info, std::uint32_t additional_displacement = 0) {
		this->translated_instruction_symbols.push_back('\x4c'); //mov r15, [rbp - n]
		this->translated_instruction_symbols.push_back('\x8b');
		this->translated_instruction_symbols.push_back('\xbd');

		this->write_bytes(variable_info.first - additional_displacement);
	}
	void generate_pointer_size_check() {
		this->translated_instruction_symbols.push_back('\x49'); //cmp r15, 0
		this->translated_instruction_symbols.push_back('\x83');
		this->translated_instruction_symbols.push_back('\xff');
		this->translated_instruction_symbols.push_back('\x00');

		this->translated_instruction_symbols.push_back('\x75'); //jne nullptr_check
		this->write_bytes<char>(program_termination_code_size);

		::generate_program_termination_code(this->translated_instruction_symbols, termination_codes::nullptr_dereference);
		//:nullptr_check

		this->translated_instruction_symbols.push_back('\x4d'); //cmp r8, [r15]
		this->translated_instruction_symbols.push_back('\x3b');
		this->translated_instruction_symbols.push_back('\x07');

		this->translated_instruction_symbols.push_back('\x72'); //jb end
		this->write_bytes<char>(program_termination_code_size);

		::generate_program_termination_code(this->translated_instruction_symbols, termination_codes::pointer_out_of_bounds);
		//:end
	}

	void accumulate_pointer_offset(dereferenced_pointer* pointer, std::uint32_t additional_displacement = 0) {
		this->zero_r8();
		for (entity_id dereference_index : pointer->get_dereference_indexes()) {
			auto variable_info = this->get_variable_info(dereference_index);
			std::uint8_t variable_type = variable_info.second;

			this->assert_statement(variable_type != 4, "You can not use pointer as offset index.", pointer->get_id());

			regular_variable reg_var{ variable_type, variable_type, dereference_index };
			this->create_variable_instruction_with_two_opcodes('\x02', '\x03', true, &reg_var, 0, additional_displacement);
		}
	}
	void add_base_address_to_pointer_dereference(dereferenced_pointer* pointer, std::uint32_t additional_displacement = 0) {
		this->load_pointer_info(
			this->get_variable_info(pointer->get_id()), 
			additional_displacement
		);

		std::uint32_t active_type_size = static_cast<std::uint32_t>(
			memory_layouts_builder::get_variable_size(pointer->get_active_type())
			);

		std::uint32_t additional_read_size = active_type_size - 1;
		if (additional_read_size > 0) {
			this->translated_instruction_symbols.push_back('\x49'); //add r8, {active_type_size - 1}
			this->translated_instruction_symbols.push_back('\x81');
			this->translated_instruction_symbols.push_back('\xc0');
			this->write_bytes<std::uint32_t>(additional_read_size);
		}

		this->generate_pointer_size_check();
		if (additional_read_size > 0) {
			this->translated_instruction_symbols.push_back('\x49'); //sub r8, {active_type_size - 1}
			this->translated_instruction_symbols.push_back('\x81');
			this->translated_instruction_symbols.push_back('\xe8');
			this->write_bytes<std::uint32_t>(additional_read_size);
		}

		this->translated_instruction_symbols.push_back('\x4d'); //add r8, [r15 + 8]
		this->translated_instruction_symbols.push_back('\x03');
		this->translated_instruction_symbols.push_back('\x47');
		this->translated_instruction_symbols.push_back('\x08');
	}

	template<typename T> //puts immediate value in r8
	void create_immediate_instruction(T value) {
		static_assert(
			std::is_same<T, std::uint8_t>::value ||
			std::is_same<T, std::uint16_t>::value ||
			std::is_same<T, std::uint32_t>::value ||
			std::is_same<T, std::uint64_t>::value
			);

		std::uint8_t rex = 0b01000001;
		if constexpr (std::is_same<T, std::uint8_t>::value) {
			this->write_bytes(rex);
			this->write_bytes('\xb0');
		}
		else {
			if constexpr (std::is_same<T, std::uint16_t>::value) {
				this->write_bytes('\x66');
			}
			else if (std::is_same<T, std::uint64_t>::value) {
				rex |= 0b00001000;
			}

			this->write_bytes(rex);
			this->write_bytes('\xb8');
		}

		this->write_bytes(value);
	}

	std::uint8_t get_arguments_count() const { return static_cast<std::uint8_t>(this->instruction_types.size()); }
	bool check_if_active_types_match() const { //makes sense only for group00 group01 group10
		assert(this->instruction_types.size() >= 2);

		std::uint8_t sample = this->instruction_types[0] & 0b11;
		for (std::size_t index = 1, instruction_types_count = this->instruction_types.size(); index < instruction_types_count; ++index) {
			std::uint8_t current_type = this->instruction_types[index];
			if ((current_type & 0b11) != sample) {
				return false;
			}
		}

		return true;
	}

	void assert_statement(bool result, std::string message, entity_id associated_id = 0) {
		if (!result) { //make changes to valid state only if it wasn't already invalid
			throw program_compilation_error{ message, associated_id };
		}
	}
	
	std::pair<std::int32_t, std::uint8_t> get_variable_info(entity_id id) {
		auto found_info = this->function_memory_layout.find(id);
		if (found_info == this->function_memory_layout.end()) {
			this->assert_statement(false, "Variable with this id does not exist.", id);
		}

		return found_info->second;
	}
	std::pair<std::unique_ptr<char[]>, std::size_t>& get_string_info(entity_id id) {
		auto found_info = this->general_file_information.program_strings.find(id);
		if (found_info == this->general_file_information.program_strings.end()) {
			this->assert_statement(false, "String with this id does not exist.", id);
		}

		return found_info->second;
	}

	std::size_t get_function_table_index(entity_id id) {
		return this->jump_table.get_function_table_index(id);
	}
	std::size_t get_jump_point_table_index(entity_id id) {
		return this->jump_table.get_jump_point_table_index(id);
	}

	void self_call_by_type(std::uint8_t type_bits) {
		this->self_call_by_index(this->find_type_index(type_bits));
	}
	void self_call_next() {
		this->self_call_by_index(0);
	}

	std::uint8_t get_argument_index() { return this->argument_index; }
public:
	virtual void visit(std::unique_ptr<variable_imm<std::uint8_t>>) { 
		this->assert_statement(false, "Instruction does not recognize arguments with type immediate (8 bits)."); 
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint16_t>>) { 
		this->assert_statement(false, "Instruction does not recognize arguments with type immediate (16 bits).");
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint32_t>>) { 
		this->assert_statement(false, "Instruction does not recognize arguments with type immediate (32 bits).");
	}
	virtual void visit(std::unique_ptr<variable_imm<std::uint64_t>>) { 
		this->assert_statement(false, "Instruction does not recognize arguments with type immediate (64 bits).");
	}
	virtual void visit(std::unique_ptr<regular_variable> val) { 
		this->assert_statement(false, "Instruction does not recognize regular variables.", val->get_id());
	}
	virtual void visit(std::unique_ptr<signed_variable> val) { 
		this->assert_statement(false, "Instruction does not recognize signed variables.", val->get_id());
	}
	virtual void visit(std::unique_ptr<dereferenced_pointer> val) { 
		this->assert_statement(false, "Instruction does not recognize pointer dereference.", val->get_id());
	}
	virtual void visit(std::unique_ptr<module> val) { 
		this->assert_statement(false, "Instruction does not recognize modules.", val->get_id());
	}
	virtual void visit(std::unique_ptr<specialized_variable> val) { 
		this->assert_statement(false, "Instruction does not recognize specialized variable (the meaning of this argument depends on the instruction)", val->get_id());
	}
	virtual void visit(std::unique_ptr<function> val) { 
		this->assert_statement(false, "Instruction does not recognize functions as arguments.", val->get_id());
	}
	virtual void visit(std::unique_ptr<pointer> val) { 
		this->assert_statement(false, "Instruction does not use pointers as arguments.", val->get_id());
	}

	virtual void build() = 0;
	const std::vector<char>& get_translated_instruction() const { return this->translated_instruction_symbols; }

	virtual ~instruction_builder() = default;
};

#endif