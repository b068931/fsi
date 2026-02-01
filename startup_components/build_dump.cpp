#include <algorithm>
#include <string>
#include <string_view>
#include <cassert>

// Must be defined by the application that consumes this library.
extern const char* STARTUP_COMPONENTS_COMPONENT_BUILD_TIME;

// Must be defined by the application that consumes this library.
extern const long STARTUP_COMPONENTS_CPLUSPLUS_VERSION;

// Must be defined by the application that consumes this library.
// Describes the name of the component. E.g. "VISUAL ENVIRONMENT", "MODULE MEDIATOR", etc.
extern const char* STARTUP_COMPONENTS_COMPONENT_NAME;

// Must be defined by the application that consumes this library.
// Provides a human-readable version string for the component.
extern const char* STARTUP_COMPONENTS_BUILD_VERSION;

// Must be defined by the application that consumes this library.
// Look into startup_components/unicode_main.cpp for explanation.
extern const char* STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE;

// Must be defined by the application that consumes this library.
// Specifies whether the build is a DEBUG, PRE-RELEASE or RELEASE build.
// Determined by the values of different macros defined during compilation.
extern const char* STARTUP_COMPONENTS_BUILD_TYPE;

// Must be defined by the application that consumes this library.
// Provides information about the compiler used to build the component.
// Detects either MSVC or Clang-CL and provides their version information.
extern const std::string STARTUP_COMPONENTS_COMPILER_INFORMATION;

namespace {
    void append_wrapped_line(std::string& output, std::string_view line, std::size_t width) {
        if (width == 0) {
            output.append(line);
            output.push_back('\n');
            return;
        }

        std::size_t position = 0;
        while (position < line.size()) {
            std::size_t chunk_size = std::min(width, line.size() - position);
            output.append(line.substr(position, chunk_size));
            output.push_back('\n');
            position += chunk_size;
        }
    }
}

namespace startup_components {
    extern std::string dump_build_information();
    std::string dump_build_information() {
        constexpr unsigned header_indentation_size = 80;

        const std::string header_message{ "BUILD>INFO" };
        const std::string footer_message{ "CUT<HERE" };

        assert(header_indentation_size > header_message.size() &&
            "Header indentation size must be larger than the header message size.");

        assert(header_indentation_size > footer_message.size() &&
            "Header indentation size must be larger than the footer message size.");

        std::string header(header_indentation_size, '>');
        std::copy_n(header_message.begin(), header_message.size(), 
            header.end() - static_cast<std::ptrdiff_t>(header_message.size()));

        std::string footer(header.size(), '<');
        std::copy_n(footer_message.begin(), 
            footer_message.size(), footer.begin());

        std::string output;
        output.append(header);
        output.push_back('\n');

        std::string build_information = std::string(STARTUP_COMPONENTS_COMPONENT_NAME) + " " +
            STARTUP_COMPONENTS_BUILD_VERSION + " " + STARTUP_COMPONENTS_BUILD_TYPE;

        append_wrapped_line(output, build_information, header.size());

        std::string timestamp = std::string("TIMESTAMP: ") + STARTUP_COMPONENTS_COMPONENT_BUILD_TIME;
        append_wrapped_line(output, timestamp, header.size());

        std::string compiler = std::string("COMPILER/STANDARD: ") + STARTUP_COMPONENTS_COMPILER_INFORMATION + 
            " / C++" + std::to_string(STARTUP_COMPONENTS_CPLUSPLUS_VERSION);

        append_wrapped_line(output, compiler, header.size());

        std::string console_state = std::string("CONSOLE STATE: ") + 
            (std::string(STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE) == "HIDE" 
                ? "HIDDEN. MAY CAUSE WINDOW FLICKERING ON STARTUP." : "VISIBLE");

        append_wrapped_line(output, console_state, header.size());

        output.append(footer);
        output.push_back('\n');
        output.push_back('\n');
        return output;
    }
}
