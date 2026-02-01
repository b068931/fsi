#ifndef STARTUP_COMPONENTS_UNICODE_PUNNING_H
#define STARTUP_COMPONENTS_UNICODE_PUNNING_H

#include <string>
#include <filesystem>

namespace startup_components {
    inline std::u8string pretend_char_char8_are_same(const std::string& input) {
        return std::u8string{ input.begin(), input.end() };
    }
}

#define UTF8_STRING(VALUE) startup_components::pretend_char_char8_are_same(VALUE)
#define UTF8_PATH(VALUE) std::filesystem::path{ UTF8_STRING(VALUE) }

#endif
