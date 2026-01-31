#ifndef MODULE_MEDIATOR_FILE_BUILDER_H
#define MODULE_MEDIATOR_FILE_BUILDER_H

#include <vector>
#include <string>

#include "module_part.h"
#include "file_components.h"
#include "../generic_parser/token_generator.h"
#include "../generic_parser/read_map.h"

namespace module_mediator {
    class engine_module_mediator;
}

namespace module_mediator::parser::components {
    class engine_module_builder {
    public:
        struct builder_parameters {
            module_part* module_part{};
            std::vector<std::string> arguments{
                "signed-one-byte",
                "one-byte",
                "signed-two-bytes",
                "two-bytes",
                "signed-four-bytes",
                "four-bytes",
                /*
                 * The only reason why this (long, ulong) exists is to keep compatibility with old implementation.
                 * The entire implementation depends on this module system. And unfortunately enough, I didn't care
                 * much about it: it is extremely rigid due to heavy reliance on the "type indexes" (that is, char is 0,
                 * uchar is 1, etc.). So, I can rename types quite easily, but God forbid I change their indexes.
                 *
                 * Another thing is that it expects that there will be no padding between the values (which is technically UB).
                 * There are numerous other problems, and, frankly, it is simply not worth fixing them right now. Like
                 * the fact that all pointers are just treated as void*, thus they do not preserve the type information.
                 *
                 * Modules themselves are not that reliant on these ideas because they use "arguments_string_builder" to
                 * work with the arguments string. Compilation in program_loader, however, is heavily reliant on this.
                 */
                "long",
                "ulong",
                "signed-eight-bytes",
                "eight-bytes",
                "memory"
            };

            std::string module_name;

            bool is_visible{ false };
            std::string function_name;
            std::string function_exported_name;
        };

        enum class file_tokens {
            end_of_file,
            name, //this token is ignored, because configuration of token_generator for this class does not have base_separators
            new_line,
            header_open,
            header_close,
            value_assign,
            comment,
            name_and_public_name_separator,
            program_callable_function
        };
        enum class dynamic_parameters_keys {}; //unused
        enum class context_keys {
            main_context
        };

        using result_type = std::vector<engine_module>;
        using read_map_type = generic_parser::read_map<file_tokens, context_keys, result_type, builder_parameters, dynamic_parameters_keys>;
    private:
        result_type modules;
        builder_parameters parameters;

        engine_module_mediator* mediator;

        read_map_type parse_map;
        void configure_parse_map();

    public:
        engine_module_builder(
            std::vector<std::pair<std::string, file_tokens>>*,
            generic_parser::token_generator<file_tokens, context_keys>* token_generator,
            engine_module_mediator* module_mediator
        ) //"mediator" will be used to initialize engine_module objects
            :mediator{ module_mediator },
            parse_map{ file_tokens::end_of_file, file_tokens::name, token_generator }
        {
            this->configure_parse_map();
        }

        const std::string& error() { return this->parse_map.error(); }
        bool is_working() { return this->parse_map.is_working(); }
        void handle_token(file_tokens token) {
            this->parse_map.handle_token(&this->modules, token, &this->parameters);
        }
        result_type get_value() { return std::move(this->modules); }
    };
}

#endif
