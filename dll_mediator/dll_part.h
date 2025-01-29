#ifndef DLL_PART
#define DLL_PART

#include <string>
#include <tuple>
#include <cassert>
#include <vector>
#include <utility>
#include <memory>

#include "../typename_array/typename_array.h"

namespace module_mediator {
	using return_value = uint64_t;
	using arguments_string_type = unsigned char*;
	using arguments_string_element = unsigned char;
	using arguments_array_type = std::vector<std::pair<arguments_string_element, void*>>;

	//this class will be passed to dlls so that they can call functions in other dlls
	class dll_part {
	public:
		enum class call_error {
			function_is_not_visible,
			unknown_index,
			invalid_arguments_string,
			no_error
		};

		static constexpr size_t function_not_found = std::numeric_limits<size_t>::max();
		static constexpr size_t dll_not_found = std::numeric_limits<size_t>::max();

		virtual size_t find_function_index(size_t dll_index, const char* name) const = 0;
		virtual size_t find_dll_index(const char* name) const = 0;

		virtual return_value call_module(size_t module_index, size_t function_index, arguments_string_type arguments_string) = 0;
		virtual return_value call_module_visible_only(size_t module_index, size_t function_index, arguments_string_type arguments_string, void(*error_callback)(call_error)) = 0;

		virtual ~dll_part() = default;
	};

	class arguments_string_builder {
	private:
		template<typename... types>
		struct enumarate_type_sizes {
			static constexpr size_t sizes[]{ sizeof(types)... };
		};

		using map_array = typename_array::typename_array<char, unsigned char, short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, void*>;
		using type_sizes_container = map_array::template acquire<enumarate_type_sizes>;

		template<typename type>
		struct functor_sum {
			static constexpr decltype(auto) value = sizeof(type);
		};

		template<typename type, typename_array::typename_array_size_t index>
		struct get_arguments_types {
			using new_value =
				typename_array::typename_array<
				typename_array::value<typename_array::find<map_array, type>::index>,
				typename_array::value<index + 1> //+1 because the first byte is the amount of arguments in a string
				>;
		};

		template<typename... types>
		struct assign_arguments_types {
		private:
			template<typename T>
			static void do_assign(arguments_string_type string) {
				string[typename_array::get<1, T>::value::get_value] = typename_array::get<0, T>::value::get_value;
			}

		public:
			static void assign(arguments_string_type string) {
				(..., do_assign<types>(string));
			}
		};

		template<typename type>
		static void write_value(arguments_string_element** arguments_string_copy, type value) {
			std::memcpy(*arguments_string_copy, &value, sizeof(value));
			*arguments_string_copy += sizeof(value);
		}

		template<typename... types>
		static arguments_string_type build_types_string(size_t size) {
			using types_array = typename_array::typename_array<types...>;

			arguments_string_type types_string = new arguments_string_element[size]{};
			types_string[0] = sizeof... (types);

			using types_assign = typename typename_array::apply<types_array, get_arguments_types>::new_array::template acquire<assign_arguments_types>;
			types_assign::assign(types_string);

			return types_string;
		}

		template<typename... types>
		static bool check_arguments_string_types(const arguments_string_element* arguments_string) {
			arguments_string_type types_string = get_types_string<types...>();
			bool result = check_if_arguments_strings_match(arguments_string, types_string);

			delete[] types_string;
			return result;
		}

		template<typename type>
		static type read_value(arguments_string_element** arguments_string_copy) {
			type result{};
			std::memcpy(&result, *arguments_string_copy, sizeof(type));

			*arguments_string_copy += sizeof(type);
			return result;
		}

	public:
		template<typename type>
		static constexpr typename_array::typename_array_size_t get_type_index = typename_array::find<map_array, type>::index;

		static size_t get_type_size_by_index(arguments_string_element index) {
			assert(index < map_array::size && "invalid index");
			return type_sizes_container::sizes[index];
		}

		static bool check_if_arguments_strings_match(const arguments_string_element* first, const arguments_string_element* second) {
			assert(first && second && "nullptr passed as arguments string");
			if (first[0] == second[0]) {
				return std::memcmp(first, second, static_cast<size_t>(first[0]) + 1) == 0;
			}

			return false;
		}
		static arguments_string_element convert_program_type_to_arguments_string_type(uint8_t type) {
			arguments_string_element result = arguments_string_element{};
			switch (type) {
			case 0:
				result = arguments_string_element{ 1 };
				break;

			case 1:
				result = arguments_string_element{ 3 };
				break;

			case 2:
				result = arguments_string_element{ 5 };
				break;

			case 3:
				result = arguments_string_element{ 9 };
				break;

			case 4:
				result = arguments_string_element{ 10 };
				break;
			}

			return result;
		}

		template<typename... types>
		static arguments_string_type get_types_string() {
			constexpr size_t types_string_size = sizeof... (types) + 1;
			arguments_string_type types_string = build_types_string<types...>(types_string_size);

			return types_string;
		}

		static arguments_array_type convert_to_arguments_array(arguments_string_type arguments_string) {
			assert(arguments_string && "nullptr");
			arguments_string_element arguments_count = arguments_string[0];

			std::vector<std::pair<arguments_string_element, void*>> arguments_array{};
			arguments_array.reserve(arguments_count);

			arguments_string_type values = arguments_string + arguments_count + 1;
			arguments_string_type types = arguments_string + 1;
			for (arguments_string_element counter = 0; counter < arguments_count; ++counter) {
				arguments_array.push_back({ *types, values });

				values += arguments_string_builder::get_type_size_by_index(*types);
				++types;
			}

			return arguments_array;
		}
		static std::pair<arguments_string_type, size_t> convert_from_arguments_array(
			arguments_array_type::iterator begin,
			arguments_array_type::iterator end
		) {
			auto saved_begin = begin;
			size_t size_to_allocate = 1;
			while (begin != end) {
				size_to_allocate += 1 + arguments_string_builder::get_type_size_by_index(begin->first);
				++begin;
			}

			arguments_string_type arguments_string = new arguments_string_element[size_to_allocate]{};
			arguments_string[0] = static_cast<arguments_string_element>(end - saved_begin);

			arguments_string_type arguments_string_types = arguments_string + 1;
			arguments_string_type arguments_string_values = arguments_string + arguments_string[0] + 1;
			while (saved_begin != end) {
				*arguments_string_types = saved_begin->first;
				std::memcpy(
					arguments_string_values,
					saved_begin->second,
					arguments_string_builder::get_type_size_by_index(saved_begin->first)
				);

				++arguments_string_types;
				arguments_string_values += arguments_string_builder::get_type_size_by_index(saved_begin->first);

				++saved_begin;
			}

			return { arguments_string, size_to_allocate };
		}

		template<typename destination_type>
		static bool extract_value_from_arguments_array(
			destination_type* out,
			size_t index,
			const arguments_array_type& arguments_array
		) {
			constexpr auto type_index = arguments_string_builder::get_type_index<destination_type>;
			static_assert(type_index != typename_array::npos);

			if (index < arguments_array.size()) {
				if (arguments_array[index].first == type_index) {
					std::memcpy(out, arguments_array[index].second, get_type_size_by_index(type_index));
					return true;
				}
			}

			return false;
		}

		template<typename... types>
		static arguments_string_type pack(types... values) {
			using types_array = typename_array::typename_array<types...>;
			constexpr size_t arguments_string_size = typename_array::sum<types_array, functor_sum, size_t>::new_value + sizeof... (types) + 1; //+1 because of the first byte

			arguments_string_type arguments_string = build_types_string<types...>(arguments_string_size);
			arguments_string_type arguments_string_pointer_copy = arguments_string + sizeof... (types) + 1;

			(..., write_value(&arguments_string_pointer_copy, values));
			return arguments_string;
		}

		template<typename... types>
		static std::tuple<types...> unpack(arguments_string_type arguments_string) {
			assert(check_arguments_string_types<types...>(arguments_string) && "types do not match. update dlls.txt.");
			arguments_string += sizeof... (types) + 1;

			return std::tuple<types...>{
				read_value<types>(&arguments_string)...
			};
		}
	};

	template<typename... args>
	return_value fast_call(
		dll_part* part,
		size_t module_index,
		size_t function_index,
		args... arguments
	) {
		std::unique_ptr<arguments_string_element[]> args_string{
			arguments_string_builder::pack(arguments...)
		};

		return part->call_module(
			module_index,
			function_index,
			args_string.get()
		);
	}
}

#endif