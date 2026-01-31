#include <algorithm>
#include <string>
#include <string_view>

// Must be defined by the application that consumes this library.
// Describes the name of the component. E.g. "VISUAL ENVIRONMENT", "MODULE MEDIATOR", etc.
extern const char* STARTUP_COMPONENTS_COMPONENT_NAME;

// Must be defined by the application that consumes this library.
// Provides a human-readable version string for the component.
extern const char* STARTUP_COMPONENTS_BUILD_VERSION;

// Look into startup_components/unicode_main.cpp for explanation.
extern const char* STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE;

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
        constexpr char header_message[]{ "BUILD>INFO" };
        constexpr char footer_message[]{ "CUT<HERE" };

        static_assert(header_indentation_size > std::size(header_message),
            "Header indentation size must be larger than the header message size.");

        static_assert(header_indentation_size > std::size(footer_message),
            "Header indentation size must be larger than the footer message size.");

#if defined(NDEBUG) && !defined(APPLICATION_RELEASE)
        constexpr char build_type[]{ "PRE-RELEASE" };
#elif defined(NDEBUG) && defined(APPLICATION_RELEASE)
        constexpr char build_type[]{ "RELEASE" };
#elif !defined(NDEBUG)
        constexpr char build_type[]{ "DEBUG" };
#endif

        std::string header(header_indentation_size, '>');
        std::copy_n(header_message, std::size(header_message), 
            header.end() - std::size(header_message));

        std::string footer(header.size(), '<');
        std::copy_n(footer_message, std::size(footer_message), footer.begin());

        std::string output;
        output.append(header);
        output.push_back('\n');

        std::string build_information = std::string(STARTUP_COMPONENTS_COMPONENT_NAME) + " " +
            STARTUP_COMPONENTS_BUILD_VERSION + " " + build_type;
        append_wrapped_line(output, build_information, header.size());

        std::string timestamp = std::string("TIMESTAMP: ") + __DATE__ + " " + __TIME__;
        append_wrapped_line(output, timestamp, header.size());

        output.append(footer);
        output.push_back('\n');
        output.push_back('\n');
        return output;
    }
}
