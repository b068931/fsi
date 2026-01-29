#ifndef MODULE_PART_H
#define MODULE_PART_H

#include <string>
#include <tuple>
#include <cassert>
#include <vector>
#include <utility>
#include <memory>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "../submodule_typename_array/typename-array/typename-array-primitives/include-all-namespace.hpp"

namespace module_mediator {
    using return_value = std::uint64_t;
    using arguments_string_type = unsigned char*;
    using arguments_string_element = unsigned char;
    using arguments_array_type = std::vector<std::pair<arguments_string_element, void*>>;

    inline constexpr return_value module_success = std::numeric_limits<return_value>::max();
    inline constexpr return_value module_failure = std::numeric_limits<return_value>::max() - 1;

    struct callback_bundle {
        char* module_name;
        char* function_name;
        arguments_string_type arguments_string;
    };

    //this class will be passed to modules so that they can call functions in other modules
    class module_part {
    public:
        enum class call_error {
            function_is_not_visible,
            unknown_index,
            invalid_arguments_string,
            no_error
        };

        module_part() = default;

        module_part(const module_part&) = delete;
        module_part& operator= (const module_part&) = delete;

        module_part(module_part&&) noexcept = delete;
        module_part& operator= (module_part&&) noexcept = delete;

        static constexpr std::size_t function_not_found = std::numeric_limits<std::size_t>::max();
        static constexpr std::size_t module_not_found = std::numeric_limits<std::size_t>::max();

        virtual std::size_t find_function_index(std::size_t module_index, const char* name) const = 0;
        virtual std::size_t find_module_index(const char* name) const = 0;

        virtual return_value call_module(std::size_t module_index, std::size_t function_index, arguments_string_type arguments_string) = 0;
        virtual return_value call_module_visible_only(std::size_t module_index, std::size_t function_index, arguments_string_type arguments_string, void(*error_callback)(call_error)) = 0;

        virtual ~module_part() = default;
    };

    class arguments_string_builder {
        template<typename... types>
        struct enumerate_type_sizes {
            static constexpr std::size_t sizes[]{ sizeof(types)... };
        };

        using map_array = typename_array_primitives::typename_array<
            std::int8_t,
            std::uint8_t,
            std::int16_t,
            std::uint16_t,
            std::int32_t,
            std::uint32_t,
            long, //refer to #file_builder.h for more information
            unsigned long,
            std::int64_t,
            std::uint64_t,
            void* //refer to #file_builder.h for more information
        >;

        using type_sizes_container = map_array::acquire<enumerate_type_sizes>;

        template<typename type>
        struct functor_sum {
            static constexpr decltype(auto) value = sizeof(type);
        };

        template<typename type, typename_array_primitives::typename_array_size_type index>
        struct get_arguments_types {
            using new_value =
                typename_array_primitives::typename_array<
                typename_array_primitives::value_wrapper<typename_array_primitives::find<map_array, type>::index>,
                typename_array_primitives::value_wrapper<index + 1> //+1 because the first byte is the amount of arguments in a string
                >;
        };

        template<typename... types>
        struct assign_arguments_types {
        private:
            template<typename T>
            static void do_assign(arguments_string_type string) {
                string[typename_array_primitives::get<1, T>::value::get_value] = typename_array_primitives::get<0, T>::value::get_value;
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
        static arguments_string_type build_types_string(std::size_t size) {
            using types_array = typename_array_primitives::typename_array<types...>;

            arguments_string_type types_string = new arguments_string_element[size]{};
            types_string[0] = sizeof... (types);

            using types_assign = 
                typename typename_array_primitives::apply<types_array, get_arguments_types>::new_array
            ::template acquire<assign_arguments_types>;

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
        static constexpr typename_array_primitives::typename_array_size_type get_type_index = 
            typename_array_primitives::find<map_array, type>::index;

        static std::size_t get_type_size_by_index(arguments_string_element index) {
            assert(index < map_array::size && "invalid index");
            return type_sizes_container::sizes[index];
        }

        static bool check_if_arguments_strings_match(const arguments_string_element* first, const arguments_string_element* second) {
            assert(first && second && "nullptr passed as arguments string");
            if (first[0] == second[0]) {
                return std::memcmp(first, second, static_cast<std::size_t>(first[0]) + 1) == 0;
            }

            return false;
        }

        static arguments_string_element convert_program_type_to_arguments_string_type(std::uint8_t type) {
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

            default: 
                throw std::invalid_argument(
                    "Invalid program type provided for conversion to arguments string type.");
            }

            return result;
        }

        template<typename... types>
        static arguments_string_type get_types_string() {
            constexpr std::size_t types_string_size = sizeof... (types) + 1;
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
                arguments_array.emplace_back(*types, values);

                values += get_type_size_by_index(*types);
                ++types;
            }

            return arguments_array;
        }
        static std::pair<arguments_string_type, std::size_t> convert_from_arguments_array(
            arguments_array_type::iterator begin,
            arguments_array_type::iterator end
        ) {
            auto saved_begin = begin;
            std::size_t size_to_allocate = 1;
            while (begin != end) {
                size_to_allocate += 1 + get_type_size_by_index(begin->first);
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
                    get_type_size_by_index(saved_begin->first)
                );

                ++arguments_string_types;
                arguments_string_values += get_type_size_by_index(saved_begin->first);

                ++saved_begin;
            }

            return { arguments_string, size_to_allocate };
        }

        template<typename destination_type>
        static bool extract_value_from_arguments_array(
            destination_type* out,
            std::size_t index,
            const arguments_array_type& arguments_array
        ) {
            constexpr auto type_index = get_type_index<destination_type>;
            static_assert(type_index != typename_array_primitives::npos);

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
            using types_array = typename_array_primitives::typename_array<types...>;
            constexpr std::size_t arguments_string_size = typename_array_primitives::sum<types_array, functor_sum, std::size_t>::new_value + 
                sizeof... (types) + 1; //+1 because of the first byte

            arguments_string_type arguments_string = build_types_string<types...>(arguments_string_size);
            arguments_string_type arguments_string_pointer_copy = arguments_string + sizeof... (types) + 1;

            (..., write_value(&arguments_string_pointer_copy, values));
            return arguments_string;
        }

        template<typename... types>
        static std::tuple<types...> unpack(arguments_string_type arguments_string) {
            assert(check_arguments_string_types<types...>(arguments_string) && "Types do not match. Update modules descriptor file.");
            arguments_string += sizeof... (types) + 1;

            return std::tuple<types...>{
                read_value<types>(&arguments_string)...
            };
        }
    };

    template<typename... args>
    callback_bundle* create_callback(
        const std::string& module_name,
        const std::string& function_name,
        args... arguments
    ) {
        std::unique_ptr<callback_bundle> callback_info{ new callback_bundle{} };
        std::unique_ptr<arguments_string_element[]> arguments_string{
            arguments_string_builder::pack<args..., void*>(arguments..., callback_info.get())
        };

        std::unique_ptr<char[]> module_name_copy{ new char[module_name.size() + 1]{} };
        std::unique_ptr<char[]> function_name_copy{ new char[function_name.size() + 1]{} };

        std::memcpy(module_name_copy.get(), module_name.data(), module_name.size());
        std::memcpy(function_name_copy.get(), function_name.data(), function_name.size());

        callback_info->module_name = module_name_copy.release();
        callback_info->function_name = function_name_copy.release();
        callback_info->arguments_string = arguments_string.release();

        return callback_info.release();
    }

    template<typename... args>
    struct respond_callback {
    private:
        template<typename_array_primitives::typename_array_size_type start, typename_array_primitives::typename_array_size_type end>
        static void copy_tuple(auto& destination, auto& source) {
            if constexpr (start < end) {
                std::get<start>(destination) = std::get<start>(source);
                copy_tuple<start + 1, end>(destination, source);
            }
        }

    public:
        static std::tuple<args...> unpack(arguments_string_type arguments_string) {
            auto data = arguments_string_builder::unpack<args..., void*>(arguments_string);
            callback_bundle* callback_info = 
                static_cast<callback_bundle*>(std::get<sizeof... (args)>(data));

            delete[] callback_info->module_name;
            delete[] callback_info->function_name;
            delete[] callback_info->arguments_string;
            delete callback_info;

            std::tuple<args...> result{};
            copy_tuple<0, sizeof...(args)>(result, data);

            return result;
        }
    };

    template<typename... args>
    return_value fast_call(
        module_part* part,
        std::size_t module_index,
        std::size_t function_index,
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
