#ifdef __clang__
#pragma clang diagnostic push 
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#include <Windows.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <iostream>
#include <cstdlib>
#include <new>
#include <string>
#include <cwchar>
#include <variant>
#include <format>

using error_type = std::string;

// A rather janky way to adapt this component to be used even with GUI applications.
// This expects that all applications use CONSOLE subsystem, but GUI applications 
// can disable the console window at runtime if they want to.
extern const char* STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE;

namespace startup_components {
    // It is expected that applications will define this functions as their
    // entry point, instead of the regular main function.
    // This allows the applications to be Unicode-aware from the start.
    extern int u8main(int argc, char** argv);

    // This one is already defined by the library.
    extern std::string dump_build_information();
}

namespace {
    template<class... Ts>
    struct overloads : Ts... { using Ts::operator()...; };

    std::variant<std::wstring, error_type> get_current_executable_path() {
        constexpr std::size_t max_buffer_size = 32768;
        std::wstring buffer(MAX_PATH, L'\0');

        while (true) {
            DWORD size = static_cast<DWORD>(buffer.size());
            DWORD length = GetModuleFileNameW(nullptr, buffer.data(), size);

            if (length == 0) {
                return std::format(
                    R"*(Function "GetModuleFileNameW" has failed with value {}.)*",
                    GetLastError());
            }

            if (length < size - 1) {
                buffer.resize(length);
                return buffer;
            }

            if (buffer.size() > max_buffer_size) {
                return "Maximum buffer size exceeded.";
            }

            buffer.resize(buffer.size() * 2, L'\0');
        }
    }

    void append_command_line_argument(std::wstring& command_line, const wchar_t* argument) {
        if (!command_line.empty()) {
            command_line.push_back(L' ');
        }

        bool needsQuotes = std::wcspbrk(argument, L" \t\"") != nullptr;
        if (!needsQuotes) {
            command_line.append(argument);
            return;
        }

        command_line.push_back(L'"');

        std::size_t backslash_count = 0;
        for (const wchar_t* current = argument; *current != L'\0'; ++current) {
            if (*current == L'\\') {
                ++backslash_count;
                continue;
            }

            if (*current == L'"') {
                command_line.append(backslash_count * 2 + 1, L'\\');
                command_line.push_back(L'"');
                backslash_count = 0;
                continue;
            }

            if (backslash_count > 0) {
                command_line.append(backslash_count, L'\\');
                backslash_count = 0;
            }

            command_line.push_back(*current);
        }

        if (backslash_count > 0) {
            command_line.append(backslash_count * 2, L'\\');
        }

        command_line.push_back(L'"');
    }

    std::variant<std::wstring, error_type> build_relaunch_command_line(
        int arguments_count, wchar_t** arguments
    ) {
        return std::visit<std::variant<std::wstring, error_type>>(overloads{
            [=](const std::wstring& executable_path) {
                std::wstring command_line;

                append_command_line_argument(command_line, executable_path.c_str());
                for (int index = 1; index < arguments_count; ++index) {
                    append_command_line_argument(command_line, arguments[index]);
                }

                return command_line;
            },
            [](error_type error_message) {
                return error_message;
            }
        }, get_current_executable_path());
    }

    char** convert_to_utf8(int argumentCount, wchar_t** arguments) {
        // Store passed arguments in one contiguous block of memory.
        int totalBytesRequired = 0;

        for (int index = 0; index < argumentCount; ++index) {
            totalBytesRequired += WideCharToMultiByte(
                CP_UTF8, 
                0, 
                arguments[index], 
                -1, 
                nullptr, 
                0, 
                nullptr, 
                nullptr);
        }

        int pointerArraySize = (argumentCount + 1) * static_cast<int>(sizeof(char*));
        char* memoryBlock = new(std::align_val_t{ alignof(char*) }, std::nothrow) char[
            static_cast<std::size_t>(totalBytesRequired + pointerArraySize)]{};

        if (!memoryBlock) {
            return nullptr;
        }

        char* utf8Arguments = memoryBlock;
        char* stringBuffer = memoryBlock + pointerArraySize;

        for (std::ptrdiff_t index = 0; index < argumentCount; ++index) {
            // Avoid breaking strict aliasing by using placement new to construct
            // objects directly in the allocated memory.
            new(utf8Arguments + index * static_cast<std::ptrdiff_t>(sizeof(char*))) char* (stringBuffer);
            static_assert(sizeof(char*) == alignof(char*), 
                "Unexpected alignment of char* is not equal to its size.");

            int bytesConverted = WideCharToMultiByte(
                CP_UTF8, 
                0, 
                arguments[index], 
                -1, 
                stringBuffer, 
                totalBytesRequired, 
                nullptr, 
                nullptr);

            if (bytesConverted == 0) {
                std::cerr << "Error encountered while translating the character stream.\n";
                std::quick_exit(EXIT_FAILURE);
            }

            stringBuffer += bytesConverted;
        }

        // Use std::launder because we abandoned values returned by placement new.
        // Acquire pointer to the array of objects "char*".
        return std::launder(reinterpret_cast<char**>(utf8Arguments));
    }
}

// Windows requires this specific signature for Unicode-aware main functions.
// However, working with wide characters is a pain, so we convert the wide character
// array to UTF-8 and call u8main instead.
int wmain(int arguments_count, wchar_t** arguments) {
    if (STARTUP_COMPONENTS_SHOW_APPLICATION_CONSOLE == std::string("HIDE")) {
        // First try to hide the console window if it exists.
        // This will minimize it into the taskbar for a brief moment,
        // then relaunch application without the console.
        HWND hConsoleWindow = GetConsoleWindow();
        if (hConsoleWindow != nullptr) {
            if (!ShowWindowAsync(hConsoleWindow, SW_HIDE)) {
                std::cerr << "Failed to hide the application console window. Error code: " 
                    << GetLastError() << ". Continuing application startup...\n";
            }

            if (!FreeConsole()) {
                std::cerr << "Failed to remove the application console. Error code: " 
                    << GetLastError() << ". Exiting...\n";

                return EXIT_FAILURE;
            }

            // Microsoft are genuinely insane: they abandon functions like GetConsoleWindow,
            // so that "Windows platform" aligns with other operating systems. But then they
            // have a strict delineation between console and GUI applications at the linker level.
            return std::visit<int>(overloads{
                [](std::wstring command_line) {
                    STARTUPINFOW startupInfo{};
                    startupInfo.cb = sizeof(startupInfo);
                    PROCESS_INFORMATION processInfo{};
                    DWORD creationFlags = DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;

                    BOOL created = CreateProcessW(
                        nullptr,
                        command_line.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        creationFlags,
                        nullptr,
                        nullptr,
                        &startupInfo,
                        &processInfo);

                    if (!created) {
                        std::cerr << "Failed to relaunch the application without a console window. "
                            "Error code: " << GetLastError() << ". Exiting...\n";

                        return EXIT_FAILURE;
                    }

                    CloseHandle(processInfo.hThread);
                    CloseHandle(processInfo.hProcess);

                    return EXIT_SUCCESS;
                },
                [](const error_type& error_message) {
                    std::cerr << std::format("Failed application restart with error:\n{}",
                        error_message);

                    return EXIT_FAILURE;
                }
            }, build_relaunch_command_line(arguments_count, arguments));
        }
    }
    else {
        if (!IsValidCodePage(CP_UTF8)) {
            std::cerr << "The system does not support UTF-8 code page. "
                "Can't start the application without UTF-8 support.";

            return EXIT_FAILURE;
        }

        // Notify Windows that we are going to use UTF-8 encoding for console input and output.
        // UTF-8 is nice because it works with ASCII without any special handling.
        if (!SetConsoleCP(CP_UTF8)) {
            std::cerr << "Failed to set console input code page to UTF-8. "
                "Input may be processed incorrectly.";
        }

        if (!SetConsoleOutputCP(CP_UTF8)) {
            std::cerr << "Failed to set console output code page to UTF-8. "
                "Output may look garbled.";
        }
    }

    char** utf8Arguments = convert_to_utf8(arguments_count, arguments);
    if (!utf8Arguments) {
        std::cerr << "Failed to allocate memory for UTF-8 arguments. "
                     "Can't start the application without conversion.";

        return EXIT_FAILURE;
    }

    std::cerr << startup_components::dump_build_information();
    int exitCode = startup_components::u8main(arguments_count, utf8Arguments);

    // All objects are trivially destructible, so we can just deallocate the memory block.
    operator delete[](utf8Arguments, std::align_val_t{ alignof(char*) });
    return exitCode;
}
