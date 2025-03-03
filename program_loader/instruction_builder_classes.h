#ifndef INSTRUCTION_BUILDER_CLASSES_H
#define INSTRUCTION_BUILDER_CLASSES_H

#include "apply_on_memory.h"
#include "cmp_builder.h"
#include "jmp_builder.h"
#include "apply_right_hand_bits_on_left_hand_binary.h"
#include "jcc_builder.h"
#include "add_sadd_builder.h"
#include "mul_smul_builder.h"
#include "sdiv_builder.h"
#include "div_builder.h"
#include "function_call_builder.h"
#include "module_function_call_builder.h"
#include "save_builder.h"
#include "load_builder.h"
#include "ref_builder.h"
#include "bits_shift_builder.h"
#include "ctjtd_builder.h"
#include "copy_string_builder.h"

template<typename T>
instruction_builder* construct_builder_generic(
	const std::vector<char>* machine_codes,
	std::uint8_t prefix,
	run_reader<runs_container>::run& run,
	std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>& function_memory_layout,
	jump_table_builder& jump_table,
	runs_container& general_file_information
) {
	return new T{
			machine_codes,
			prefix,
			run,
			function_memory_layout,
			jump_table,
			general_file_information
	};
}

std::map<std::uint8_t,
	instruction_builder* (*) (
		const std::vector<char>*,
		std::uint8_t,
		run_reader<runs_container>::run&,
		std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>&,
		jump_table_builder&,
		runs_container&
		)
> get_builders_mapping() {
	return {
		{ 10, & construct_builder_generic<apply_on_memory> }, //inc, dec, not
		{ 11, &construct_builder_generic<apply_on_memory> },
		{ 28, &construct_builder_generic<apply_on_memory> },
		{ 4, &construct_builder_generic<cmp_builder> },
		{ 15, &construct_builder_generic<jmp_builder> },
		{ 5, &construct_builder_generic<apply_right_hand_bits_on_left_hand_binary> }, //mov, sub, ssub(no difference from the previous one), and, or, xor
		{ 0, &construct_builder_generic<apply_right_hand_bits_on_left_hand_binary> },
		{ 1, &construct_builder_generic<apply_right_hand_bits_on_left_hand_binary> },
		{ 12, &construct_builder_generic<apply_right_hand_bits_on_left_hand_binary> },
		{ 13, &construct_builder_generic<apply_right_hand_bits_on_left_hand_binary> },
		{ 14, &construct_builder_generic<apply_right_hand_bits_on_left_hand_binary> },
		{ 8, &construct_builder_generic<mul_smul_builder> },
		{ 9, &construct_builder_generic<mul_smul_builder> },
		{ 6, &construct_builder_generic<add_sadd_builder> },
		{ 7, &construct_builder_generic<add_sadd_builder> },
		{ 16, &construct_builder_generic<jcc_builder> },
		{ 17, &construct_builder_generic<jcc_builder> },
		{ 18, &construct_builder_generic<jcc_builder> },
		{ 19, &construct_builder_generic<jcc_builder> },
		{ 20, &construct_builder_generic<jcc_builder> },
		{ 21, &construct_builder_generic<jcc_builder> },
		{ 22, &construct_builder_generic<jcc_builder> },
		{ 23, &construct_builder_generic<jcc_builder> },
		{ 24, &construct_builder_generic<jcc_builder> },
		{ 27, &construct_builder_generic<jcc_builder> },
		{ 2, &construct_builder_generic<div_builder> },
		{ 3, &construct_builder_generic<sdiv_builder> },
		{ 25, &construct_builder_generic<function_call_builder> },
		{ 26, &construct_builder_generic<module_function_call_builder> },
		{ 29, &construct_builder_generic<save_builder> },
		{ 30, &construct_builder_generic<load_builder> },
		{ 31, &construct_builder_generic<ref_builder> },
		{ 32, &construct_builder_generic<bits_shift_builder> },
		{ 33, &construct_builder_generic<bits_shift_builder> },
		{ 34, &construct_builder_generic<ctjtd_builder> },
		{ 35, &construct_builder_generic<copy_string_builder> }
	};
}
std::map<std::uint8_t, std::string> get_builders_names() {
	return {
		{ 0, "subtract" },
		{ 1, "signed-subtract" },
		{ 2, "divide" },
		{ 3, "signed-divide" },
		{ 4, "compare" },
		{ 5, "move" },
		{ 6, "add" },
		{ 7, "signed-add" },
		{ 8, "multiply" },
		{ 9, "signed-multiply" },
		{ 10, "increment" },
		{ 11, "decrement" },
		{ 12, "xor" },
		{ 13, "and" },
		{ 14, "or" },
		{ 15, "jump" },
		{ 16, "jump-equal" },
		{ 17, "jump-not-equal" },
		{ 18, "jump-greater" },
		{ 19, "jump-greater-equal" },
		{ 20, "jump-less-equal" },
		{ 27, "jump-less" },
		{ 21, "jump-above" },
		{ 22, "jump-above-equal" },
		{ 23, "jump-below" },
		{ 24, "jump-below-equal" },
		{ 25, "function-call" },
		{ 26, "module-call" },
		{ 28, "not" },
		{ 29, "save" },
		{ 30, "load" },
		{ 31, "move-pointer" },
		{ 32, "shift-left" },
		{ 33, "shift-right" },
		{ 34, "get-function-address" },
		{ 35, "copy-string" }
	};
}

instruction_builder* generate_builder(
	std::uint8_t instruction_prefix,
	std::uint8_t instruction_operation_code,
	run_reader<runs_container>::run& run,
	std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>& function_memory_layout,
	jump_table_builder& jump_table,
	runs_container& container,
	std::map<std::uint8_t, std::vector<char>>& machine_codes
) {
	auto mapped_factories{ get_builders_mapping() };
	auto found_factory = mapped_factories.find(instruction_operation_code);
	if (found_factory != mapped_factories.end()) {
		auto found_machine_codes = machine_codes.find(instruction_operation_code);
		if (found_machine_codes != machine_codes.end()) {
			return found_factory->second(
				&(found_machine_codes->second),
				instruction_prefix,
				run,
				function_memory_layout,
				jump_table,
				container
			);
		}

		return found_factory->second(
			nullptr,
			instruction_prefix,
			run,
			function_memory_layout,
			jump_table,
			container
		);
	}

	return nullptr;
}

std::string decode_builder_name(std::uint8_t instruction_operation_code) {
	std::map<std::uint8_t, std::string> builders_names{ get_builders_names() };
	auto found_name = builders_names.find(instruction_operation_code);
	if (found_name != builders_names.end()) {
		return found_name->second;
	}

	return "[UNKNOWN_BUILDER_NAME]";
}

std::pair<std::unique_ptr<instruction_builder>, std::string> create_builder(
	run_reader<runs_container>::run& run,
	std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>& function_memory_layout,
	jump_table_builder& jump_table,
	runs_container& container,
	std::map<std::uint8_t, std::vector<char>>& machine_codes
) {
	std::uint8_t instruction_prefix = run.get_object<std::uint8_t>(); //arguments count and additional type bits
	std::uint8_t instruction_operation_code = run.get_object<std::uint8_t>();

	//0xff means that instruction consists from multiple bytes. right now this information is useless.
	if (instruction_operation_code != 0xff) {
		instruction_builder* created_builder =
			generate_builder(
				instruction_prefix, instruction_operation_code,
				run, function_memory_layout,
				jump_table, container,
				machine_codes
			);
		
		return { std::unique_ptr<instruction_builder>{ created_builder }, decode_builder_name(instruction_operation_code) };
	}

	return { nullptr, "" };
}

#endif // !INSTRUCTION_BUILDER_CLASSES_H