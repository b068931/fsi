#ifndef CRT_LOCAL_CRASH_HANDLE_SETUP_H
#define CRT_LOCAL_CRASH_HANDLE_SETUP_H

// ReSharper disable CppClangTidyBugproneEmptyCatch
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include <exception>
#include <iostream>
#include <syncstream>
#include <atomic>

// Handle std::terminate
namespace module_mediator::crash_handling {
    inline std::atomic<std::terminate_handler> previous_terminate_handler = nullptr;
    [[noreturn]] inline void notify_fatal_termination() {
        try { // Ensure that the error stream is flushed
            std::osyncstream synchronized_error_stream{ std::cerr };
            try
            {
                if (std::exception_ptr received_exception{ std::current_exception() })
                {
                    std::rethrow_exception(received_exception);
                }

                synchronized_error_stream << "*** FATAL ERROR: REQUESTED TERMINATION\n";
            }
            catch (const std::exception& ex)
            {
                synchronized_error_stream << "*** FATAL ERROR: UNHANDLED C++ EXCEPTION" << '\n';
                synchronized_error_stream << "--> EXCEPTION TYPE: " << typeid(ex).name() << '\n';
                synchronized_error_stream << "--> EXCEPTION MESSAGE: " << ex.what() << '\n';
            }
            catch (...)
            {
                synchronized_error_stream << "*** FATAL ERROR: UNKNOWN UNHANDLED C++ EXCEPTION" << '\n';
            }
        }
        catch (...) {
            // It is possible that std::cerr got configured to throw exceptions
            // We can't really do anything about that
        }

        // The only data race that we can actually encounter is that we crash before we set previous_terminate_handler.
        // That is, std::set_terminate succeeds, but atomic operator= is not executed yet. This way we fail to call
        // the previous handler. Frankly, I don't even think that it is possible to avoid this data race, considering
        // the fact that we are in a fatal error handler.
        std::terminate_handler current_handler = previous_terminate_handler.load(std::memory_order_relaxed);
        if (current_handler && current_handler != &notify_fatal_termination) {
            current_handler();
        }

        // Technically useless because previous handler should terminate the process, but just in case.
        std::abort(); 
    }
}

// Handle CRT specific errors
#ifdef _MSC_VER
namespace module_mediator::crash_handling {
    inline std::atomic<_purecall_handler> previous_pure_call_handler = nullptr;
    [[noreturn]] inline void notify_fatal_pure_call() {
        try {
            std::osyncstream synchronized_error_stream{ std::cerr };
            synchronized_error_stream << "*** FATAL ERROR: PURE VIRTUAL FUNCTION CALL\n";
        }
        catch (...) {}

        _purecall_handler current_handler = previous_pure_call_handler.load(std::memory_order_relaxed);
        if (current_handler && current_handler != &notify_fatal_pure_call) {
            current_handler();
        }

        std::terminate();
    }

    inline std::atomic<_invalid_parameter_handler> previous_invalid_parameter_handler = nullptr;
    [[noreturn]] inline void notify_fatal_invalid_parameter(
        const wchar_t* expression,
        const wchar_t* function,
        const wchar_t* file,
        unsigned int line,
        uintptr_t reserved
    ) {
        try {
            std::wosyncstream synchronized_error_stream{ std::wcerr };
            synchronized_error_stream << "*** FATAL ERROR: INVALID PARAMETER\n";
            if (expression) {
                synchronized_error_stream << "--> EXPRESSION: " << expression << '\n';
            }
            if (function) {
                synchronized_error_stream << "--> FUNCTION: " << function << '\n';
            }
            if (file) {
                synchronized_error_stream << "--> FILE: " << file << '\n';
            }
            if (line) {
                synchronized_error_stream << "--> LINE: " << line << '\n';
            }
        }
        catch (...) {}

        _invalid_parameter_handler current_handler = previous_invalid_parameter_handler.load(std::memory_order_relaxed);
        if (current_handler && current_handler != &notify_fatal_invalid_parameter) {
            current_handler(expression, function, file, line, reserved);
        }

        std::terminate();
    }
}
#endif // _MSC_VER

namespace module_mediator::crash_handling {
    inline void install_crash_handlers() {
        std::terminate_handler terminate_old = std::set_terminate(notify_fatal_termination);
        if (terminate_old != &notify_fatal_termination) {
            previous_terminate_handler.store(terminate_old, std::memory_order_relaxed);
        }

#ifdef _MSC_VER
        _purecall_handler pure_call_old= _set_purecall_handler(notify_fatal_pure_call);
        if (pure_call_old != &notify_fatal_pure_call) {
            previous_pure_call_handler.store(pure_call_old, std::memory_order_relaxed);
        }

        _invalid_parameter_handler invalid_parameter_old = _set_invalid_parameter_handler(notify_fatal_invalid_parameter);
        if (invalid_parameter_old != &notify_fatal_invalid_parameter) {
            previous_invalid_parameter_handler.store(invalid_parameter_old, std::memory_order_relaxed);
        }
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
    _CrtSetReportMode(_CRT_ASSERT, 0);
#endif
    }
}

#endif
