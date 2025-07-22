#include "pch.h"
#include "standard_input_output.h"
#include "backend_functions.h"

//This file describes IO logic for the FSI programs through PRTS (Program RunTime Services) module.
//I use global variables because they are isolated to this cpp file, they cannot be accessed elsewhere.
//You should view PRTS as an assortment of "classes" or "objects" that are isolated in their own cpp files.

//WinAPI thingies for console input/output management.
namespace {
    //Available only if attached.
    HANDLE capturedStdOut;
    HANDLE capturedStdIn;

    //The handle to the worker thread that is responsible for reading from stdin.
    //Allows to exit the thread gracefully when PRTS is detached from stdin.
    HANDLE IOCancellationSignal;

    //The state of the console before PRTS attached to it.
    DWORD savedConsoleState;

    bool InitializeConsoleInput(HANDLE hStdin, DWORD& outOriginalMode) {
        if (!GetConsoleMode(hStdin, &outOriginalMode))
            return false;

        //Keep only line input, echo, processed input; disable mouse/window events.
        //Enable ASCII control characters processing with wrapping at EOL.
        DWORD mode = ENABLE_LINE_INPUT
                   | ENABLE_ECHO_INPUT
                   | ENABLE_PROCESSED_INPUT
                   | ENABLE_PROCESSED_OUTPUT
                   | ENABLE_WRAP_AT_EOL_OUTPUT;  // NOLINT(misc-redundant-expression)
        if (!SetConsoleMode(hStdin, mode))
            return false;

        //Discard any pending input events (mouse, focus, etc.)
        return FlushConsoleInputBuffer(hStdin) != FALSE;
    } 

    BOOL RestoreConsoleInput(HANDLE hStdin, DWORD originalMode) {
        return SetConsoleMode(hStdin, originalMode);
    }

    void CloseHandleReport(HANDLE& handle, const std::string& handleName) {
        if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
            if (!CloseHandle(handle)) {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Failed to close {} handle with error code {}.",
                        handleName,
                        GetLastError()
                    )
                );
            }
            else {
                handle = NULL;
            }
        }
    }

    std::pair<std::vector<char>, bool> ConsumeConsoleInput(HANDLE hStdin, HANDLE hCancelIO) {
        HANDLE waitHandles[]{ hStdin, hCancelIO };

        //Wait for input from the console.
        //This works only for the console, it won't work for redirected input.
        DWORD waitResult = WaitForMultipleObjects(
            std::size(waitHandles),
            waitHandles, 
            FALSE, 
            INFINITE
        );

        switch (waitResult) {
            case WAIT_OBJECT_0: {
                //Input is available.
                break;
            }
            case WAIT_OBJECT_0 + 1: {
                return { {}, true };
            }
            default: {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "WaitForMultipleObjects failed with error code {}.",
                        GetLastError()
                    )
                );

                return { {}, false };
            }
        }

        DWORD bufferedCharacters = 0;
        if (!GetNumberOfConsoleInputEvents(hStdin, &bufferedCharacters)) {
            constexpr DWORD defaultBufferSize = 1024;
            bufferedCharacters = defaultBufferSize;

            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "GetNumberOfConsoleInputEvents failed with error code {}." \
                    "Using default buffer size of {} bytes.",
                    GetLastError(),
                    defaultBufferSize
                )
            );
        }

        std::unique_ptr<CHAR[]> inputBuffer{ new CHAR[bufferedCharacters] };
        DWORD charactersRead = 0;

        std::vector<char> result{};
        while (WaitForSingleObject(hStdin, 0) == WAIT_OBJECT_0) {
            BOOL ok = ReadConsoleA(
                hStdin,
                inputBuffer.get(),
                bufferedCharacters,
                &charactersRead,
                NULL 
            );

            if (!ok) {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "ReadFile failed with error code {}. " \
                        "Characters read: {}",
                        GetLastError(),
                        charactersRead
                    )
                );

                if (charactersRead == 0) {
                    return { result, false };
                }
            }

            for (DWORD index = 0; index < charactersRead; ++index) {
                CHAR inputCharacter = inputBuffer[index];
                char regularCharacter = (inputCharacter >= 0 && inputCharacter < 128)  // NOLINT(clang-diagnostic-tautological-constant-out-of-range-compare)
                    ? static_cast<char>(inputCharacter)
                    : '?';

                result.push_back(regularCharacter);
            }
        }

        return { result, false };
    }

    //This function is unified for both file and pipe input.
    //The absolute monster that handles both files and pipes. (Refactoring likely needed)
    //With all possible edge cases and fallback to synchronous IO if overlapped IO fails.
    template<auto file_type>
    std::pair<std::vector<char>, bool> ConsumeAsynchronous(HANDLE hStdin, HANDLE hCancelIO, DWORD& offset, DWORD& offsetHigh) {
        constexpr bool isPipe = (file_type == FILE_TYPE_PIPE);
        HANDLE waitHandles[]{ hStdin, hCancelIO };

        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEvent(
            NULL, 
            TRUE,
            FALSE,
            NULL
        );
        
        if (overlapped.hEvent == NULL) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "CreateEvent failed with error code {}. " \
                    "Cannot perform asynchronous read operation for file.",
                    GetLastError()
                )
            );

            return { {}, true };
        }
        
        if constexpr (isPipe) {
            overlapped.Offset = 0;
            overlapped.OffsetHigh = 0;
        }
        else {
            overlapped.Offset = offset;
            overlapped.OffsetHigh = offsetHigh;
        }

        constexpr DWORD defaultBufferSize = 4096;
        DWORD bufferSize = 0;

        if constexpr (isPipe) {
            DWORD messageLeft = 0;
            PeekNamedPipe(hStdin, NULL, 0, NULL, &bufferSize, &messageLeft);

            bufferSize = std::max(bufferSize, messageLeft);
            if (bufferSize == 0) { //If we still don't have any idea about what the buffer size should be.
                bufferSize = defaultBufferSize;
            }
        }
        else {
            bufferSize = defaultBufferSize;
        }

        std::unique_ptr<char[]> buffer{ new char[bufferSize] {} };
        DWORD bytesRead = 0;
        
        BOOL readResult = ReadFile(
            hStdin,
            buffer.get(),
            bufferSize - 1,
            &bytesRead,
            &overlapped
        );
        
        if (!readResult) { //This is just how ridiculously complicated overlapped I/O works in Windows.
            if (GetLastError() == ERROR_IO_PENDING) {
                DWORD waitResult = WaitForMultipleObjects(
                    std::size(waitHandles),
                    waitHandles,
                    FALSE,
                    INFINITE
                );

                if (waitResult == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(hStdin, &overlapped, &bytesRead, FALSE)) {
                        if (GetLastError() == ERROR_HANDLE_EOF) {
                            CloseHandleReport(overlapped.hEvent, "overlapped.hEvent");
                            return { {}, true };

                        }

                        if constexpr (isPipe) {
                            if (GetLastError() == ERROR_BROKEN_PIPE) {
                                LOG_PROGRAM_WARNING(
                                    interoperation::get_module_part(),
                                    std::format(
                                        "Pipe closed before detaching PRTS from stdio."
                                    )
                                );

                                CloseHandleReport(overlapped.hEvent, "overlapped.hEvent");
                                return { {}, true };
                            }

                            if (GetLastError() == ERROR_MORE_DATA) {
                                LOG_PROGRAM_WARNING(
                                    interoperation::get_module_part(),
                                    std::format(
                                        "Unexpected more data is available in a pipe. A second call will be required."
                                    )
                                );

                                auto [result, shutdown] = 
                                    ConsumeAsynchronous<isPipe>(hStdin, offset, offsetHigh);

                                result.insert(result.begin(), buffer.get(),
                                    buffer.get() + bytesRead);

                                return result;
                            }
                        }

                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "GetOverlappedResult failed with error code {}. "
                                "Cannot read from file.",
                                GetLastError()
                            )
                        );

                        CloseHandleReport(overlapped.hEvent, "overlapped.hEvent");
                        return { {}, false };
                    }
                }
                else if (waitResult == WAIT_OBJECT_0 + 1) {
                    BOOL IOCancelResult = CancelIo(hStdin);
                    if (!IOCancelResult) {
                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "CancelIo failed with error code {}. "
                                "Cannot cancel I/O operation.",
                                GetLastError()
                            )
                        );
                    }

                    CloseHandleReport(overlapped.hEvent, "overlapped.hEvent");
                    return { {}, true };
                }
                else {
                    LOG_PROGRAM_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "WaitForSingleObject failed with error code {}. " \
                            "Cannot read from file.",
                            GetLastError()
                        )
                    );

                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return { {}, false };
                }
            }
            else if (GetLastError() == ERROR_HANDLE_EOF) {
                CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                return { {}, true };
            }
            else {
                if constexpr (isPipe) {
                    if (GetLastError() == ERROR_BROKEN_PIPE) {
                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Pipe closed before detaching PRTS from stdio."
                            )
                        );

                        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                        return { {}, true };
                    }

                    if (GetLastError() == ERROR_MORE_DATA) {
                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Unexpected more data is available in a pipe. A second call will be required."
                            )
                        );

                        auto [result, shutdown] = 
                            ConsumeAsynchronous<isPipe>(hStdin, offset, offsetHigh);

                        result.insert(result.begin(), buffer.get(),
                            buffer.get() + bytesRead);

                        return result;
                    }
                }

                //Fallback to synchronous read if overlapped IO fails.
                //Technically this is not really a documented approach, but I think this should work.
                BOOL synchronousReadResult = ReadFile(
                    hStdin,
                    buffer.get(),
                    bufferSize - 1,
                    &bytesRead,
                    NULL
                );

                if (WaitForSingleObject(hCancelIO, 0) == WAIT_OBJECT_0) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return { {}, true };
                }

                //This also means that EOF was reached.
                if (synchronousReadResult && bytesRead == 0) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return { {}, true };
                }

                if (!synchronousReadResult) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    if constexpr (isPipe) {
                        if (GetLastError() == ERROR_BROKEN_PIPE) {
                            LOG_PROGRAM_WARNING(
                                interoperation::get_module_part(),
                                std::format(
                                    "Pipe closed before detaching PRTS from stdio."
                                )
                            );

                            return { {}, true };
                        }

                        if (GetLastError() == ERROR_MORE_DATA) {
                            LOG_PROGRAM_WARNING(
                                interoperation::get_module_part(),
                                std::format(
                                    "Unexpected more data is available in a pipe. A second call will be required."
                                )
                            );

                            auto [result, shutdown] = 
                                ConsumeAsynchronous<isPipe>(hStdin, offset, offsetHigh);

                            result.insert(result.begin(), buffer.get(),
                                buffer.get() + bytesRead);

                            return result;
                        }
                    }

                    LOG_PROGRAM_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "ReadFile failed with error code {}. " \
                            "Cannot read from file both async and sync have failed.",
                            GetLastError()
                        )
                    );

                    return { {}, false };
                }
            }
        }

        //Files must have their offsets updated after a successful read.
        if constexpr (!isPipe) {
            ULARGE_INTEGER newPosition;
            newPosition.LowPart = offset;
            newPosition.HighPart = offsetHigh;
            newPosition.QuadPart += bytesRead;

            offset = newPosition.LowPart;
            offsetHigh = newPosition.HighPart;
        }
        
        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
        
        buffer[bytesRead] = '\0';
        return { std::vector<char>{ buffer.get(), buffer.get() + bytesRead }, false};
    }

    //Dispatches the input reading operation based on the type of the input handle.
    std::pair<std::vector<char>, bool> ConsumeStdIn(HANDLE hStdIn, HANDLE hCancelIO, DWORD& overlappedOffset, DWORD& overlappedOffsetHigh) {
        switch (GetFileType(hStdIn)) {
            case FILE_TYPE_CHAR: {
                //For console input, we must consume all input because the console is line-buffered.
                return ConsumeConsoleInput(capturedStdIn, IOCancellationSignal);
            }
            case FILE_TYPE_DISK: {
                //For files and pipes, we are looking for a newline character to determine the end of input.
                //All other input goes into the buffer.
                return ConsumeAsynchronous<FILE_TYPE_DISK>(
                    hStdIn,
                    hCancelIO,
                    overlappedOffset,
                    overlappedOffsetHigh
                );
            }
            case FILE_TYPE_PIPE: {
               return ConsumeAsynchronous<FILE_TYPE_PIPE>(
                    hStdIn,
                    hCancelIO,
                    overlappedOffset,
                    overlappedOffsetHigh
                );
            }
            default: {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Unknown file type for STDIN: {}. " \
                        "Cannot read from stdin. Shutting down the worker.",
                        GetFileType(capturedStdIn)
                    )
                );

                return { {}, true };
            }
        }
    }

    bool PushConsoleOutput(HANDLE hStdOut, module_mediator::memory buffer) {
        auto [output_buffer, buffer_size] = backend::decay_pointer(buffer);

        if (buffer_size == 0) return false;
        if (buffer_size > std::numeric_limits<DWORD>::max()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Buffer size {} exceeds maximum allowed size of {}.",
                    buffer_size,
                    std::numeric_limits<DWORD>::max()
                )
            );
            return true;
        }

        DWORD bytesWritten = 0;
        BOOL writeResult = WriteConsoleA(
            hStdOut,
            output_buffer,
            static_cast<DWORD>(buffer_size),
            &bytesWritten,
            NULL
        );

        if (!writeResult || bytesWritten == 0) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "WriteConsole failed with error code {}. " \
                    "Cannot write to console.",
                    GetLastError()
                )
            );

            return true;
        }

        return false;
    }

    template<auto file_type>
    bool PushAsynchronous(HANDLE hStdOut, HANDLE hCancelIO, module_mediator::memory buffer, DWORD& offset, DWORD& offsetHigh) {
        constexpr bool isPipe = (file_type == FILE_TYPE_PIPE);

        HANDLE waitHandles[]{ hStdOut, hCancelIO };
        auto [output_buffer, buffer_size] = backend::decay_pointer(buffer);

        if (buffer_size == 0) return false;
        if (buffer_size > std::numeric_limits<DWORD>::max()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Buffer size {} exceeds maximum allowed size of {}.",
                    buffer_size,
                    std::numeric_limits<DWORD>::max()
                )
            );

            return true;
        }

        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
        );

        if (overlapped.hEvent == NULL) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "CreateEvent failed with error code {}. " \
                    "Cannot perform asynchronous write operation.",
                    GetLastError()
                )
            );

            return true;
        }

        if constexpr (isPipe) {
            overlapped.Offset = 0;
            overlapped.OffsetHigh = 0;
        }
        else {
            overlapped.Offset = offset;
            overlapped.OffsetHigh = offsetHigh;
        }

        DWORD bytesWritten = 0;
        BOOL writeResult = WriteFile(
            hStdOut,
            output_buffer,
            static_cast<DWORD>(buffer_size),
            &bytesWritten,
            &overlapped
        );

        if (!writeResult) {
            if (GetLastError() == ERROR_IO_PENDING) {
                DWORD waitResult = WaitForMultipleObjects(
                    std::size(waitHandles),
                    waitHandles,
                    FALSE,
                    INFINITE
                );

                if (waitResult == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(hStdOut, &overlapped, &bytesWritten, FALSE)) {
                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "GetOverlappedResult failed with error code {}. " \
                                "Cannot write to file.",
                                GetLastError()
                            )
                        );

                        CloseHandle(overlapped.hEvent);
                        return false;
                    }
                }
                else if (waitResult == WAIT_OBJECT_0 + 1) {
                    BOOL IOCancelResult = CancelIo(hStdOut);
                    if (!IOCancelResult) {
                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "CancelIo failed with error code {}. " \
                                "Cannot cancel I/O operation.",
                                GetLastError()
                            )
                        );
                    }

                    CloseHandle(overlapped.hEvent);
                    return true;
                }
                else {
                    if constexpr (isPipe) {
                        if (GetLastError() == ERROR_BROKEN_PIPE) {
                            LOG_PROGRAM_WARNING(
                                interoperation::get_module_part(),
                                std::format(
                                    "Pipe closed before detaching PRTS from stdio."
                                )
                            );

                            CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                            return true;
                        }
                    }

                    BOOL synchronousWriteResult = WriteFile(
                        hStdOut,
                        output_buffer,
                        static_cast<DWORD>(buffer_size),
                        &bytesWritten,
                        NULL
                    );

                    if (WaitForSingleObject(hCancelIO, 0) == WAIT_OBJECT_0) {
                        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                        return true;
                    }

                    if (!synchronousWriteResult) {
                        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                        LOG_PROGRAM_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "WriteFile failed with error code {}. " \
                                "Cannot write to file both async and sync have failed.",
                                GetLastError()
                            )
                        );

                        return true;
                    }
                }
            }
        }

        //Files must have their offsets updated after a successful write.
        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
        if constexpr (!isPipe) {
            ULARGE_INTEGER newPosition;
            newPosition.LowPart = offset;
            newPosition.HighPart = offsetHigh;
            newPosition.QuadPart += bytesWritten;

            offset = newPosition.LowPart;
            offsetHigh = newPosition.HighPart;
        }

        return false;
    }

    bool PushStdOut(HANDLE hStdOut, HANDLE hCancelIO, module_mediator::memory buffer, DWORD& overlappedOffset, DWORD& overlappedOffsetHigh) {
        switch (GetFileType(hStdOut)) {
            case FILE_TYPE_CHAR: {
                return PushConsoleOutput(hStdOut, buffer);
            }
            case FILE_TYPE_DISK: {
                return PushAsynchronous<FILE_TYPE_DISK>(
                    hStdOut,
                    hCancelIO,
                    buffer,
                    overlappedOffset,
                    overlappedOffsetHigh
                );
            }
            case FILE_TYPE_PIPE: {
                return PushAsynchronous<FILE_TYPE_PIPE>(
                    hStdOut,
                    hCancelIO,
                    buffer,
                    overlappedOffset,
                    overlappedOffsetHigh
                );
            }
            default: {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Unknown file type for STDOUT: {}. " \
                        "Cannot write to stdout.",
                        GetFileType(capturedStdOut)
                    )
                );

                return true;
            }
        }
    }
}

//Define IO queues for asynchronous input/output operations.
namespace {
    struct thread_input_descriptor {
        module_mediator::return_value thread_id;
        void* buffer_return_address;
    };

    struct thread_output_descriptor {
        module_mediator::return_value thread_id;
        module_mediator::memory buffer_address;
    };

    //Both are used by their respective worker threads to ensure that all IO is asynchronous.
    namespace input_queue {
        std::mutex lock;
        std::condition_variable signaling;
        std::queue<thread_input_descriptor> input_queue;
    }

    void clean_input_queue() {
        if (!input_queue::input_queue.empty()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                "Input queue was not empty before detaching. All threads will be woken up."
            );
        }

        while (!input_queue::input_queue.empty()) {
            thread_input_descriptor descriptor = std::move(input_queue::input_queue.front());
            input_queue::input_queue.pop();

            module_mediator::fast_call<module_mediator::return_value>(
                interoperation::get_module_part(),
                interoperation::index_getter::execution_module(),
                interoperation::index_getter::execution_module_make_runnable(),
                descriptor.thread_id
            );
        }
    }

    namespace output_queue {
        std::mutex lock;
        std::condition_variable signaling;
        std::queue<thread_output_descriptor> output_queue;
    }

    void clean_output_queue() {
        if (!output_queue::output_queue.empty()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                "Output queue was not empty before detaching. All threads will be woken up."
            );
        }

        while (!output_queue::output_queue.empty()) {
            thread_output_descriptor descriptor = std::move(output_queue::output_queue.front());
            output_queue::output_queue.pop();
            module_mediator::fast_call<module_mediator::return_value>(
                interoperation::get_module_part(),
                interoperation::index_getter::execution_module(),
                interoperation::index_getter::execution_module_make_runnable(),
                descriptor.thread_id
            );
        }
    }
}

namespace {
    //Readers: input or output operations for stdin/stdout.
    //Writers: functions that attach or detach PRTS from stdio.
    std::shared_mutex io_synchronizer;

    //PRTS expects that it is the only entity that can write to stdout and stdin to work correctly.
    bool is_stdin_attached = false;
    bool check_stdio_attached() {
        std::shared_lock<std::shared_mutex> lock(io_synchronizer);
        return is_stdin_attached;
    }

    std::thread input_worker_thread{};
    void input_worker(HANDLE hStdIn, HANDLE hCancelIO) {
        LOG_INFO(
            interoperation::get_module_part(),
            "PRTS is attached to stdio. Starting input worker."
        );

        DWORD overlappedOffset = 0;
        DWORD overlappedOffsetHigh = 0;

        std::vector<char> global_input_buffer{};
        while (check_stdio_attached()) {
            std::unique_lock<std::mutex> lock(input_queue::lock);
            input_queue::signaling.wait(lock, [] {
                return !input_queue::input_queue.empty() || !check_stdio_attached();
            });

            //Exit prematurely if PRTS is detached from stdio.
            if (!check_stdio_attached()) {
                clean_input_queue();
                break;
            }

            std::queue<thread_input_descriptor> local_input_queue{};
            while (!input_queue::input_queue.empty()) {
                local_input_queue.push(std::move(input_queue::input_queue.front()));
                input_queue::input_queue.pop();
            }

            lock.unlock(); //It is critical that we do not hold the lock while processing input.
            while (!local_input_queue.empty()) {
                thread_input_descriptor descriptor = std::move(local_input_queue.front());

                std::vector<char> input_portion{};
                auto buffered_newline_character = 
                    std::ranges::find(global_input_buffer, '\n');

                if (buffered_newline_character != global_input_buffer.end()) {
                    input_portion.insert(input_portion.begin(), global_input_buffer.begin(), buffered_newline_character + 1);
                    global_input_buffer.erase(global_input_buffer.begin(), buffered_newline_character + 1);
                }
                else {
                    auto [input_data, shutdown] =
                        ConsumeStdIn(hStdIn, hCancelIO, overlappedOffset, overlappedOffsetHigh);

                    //By this point PRTS should be detached from stdio.
                    if (shutdown) {
                        lock.lock();
                        clean_input_queue();

                        break;
                    }

                    //Process everything up until a newline character. Everything else goes into global input buffer.
                    auto newline_position = std::ranges::find(input_data, '\n');
                    if (newline_position == input_data.end()) {
                        global_input_buffer.insert(global_input_buffer.end(), input_data.begin(), input_data.end());
                        continue;
                    }

                    input_portion.insert(input_portion.begin(), global_input_buffer.begin(), global_input_buffer.end()); 
                    input_portion.insert(input_portion.begin(), input_data.begin(), newline_position + 1);

                    global_input_buffer.clear();
                    global_input_buffer.insert(global_input_buffer.begin(), newline_position + 1, input_data.end());
                }

                module_mediator::memory result_data = 
                    backend::allocate_program_memory(input_portion.size());

                if (result_data == nullptr) {
                    LOG_PROGRAM_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "Failed to allocate memory for input result buffer. Thread: {}",
                            descriptor.thread_id
                        )
                    );

                    continue;
                }

                auto [input_result_buffer, buffer_size] = 
                    backend::decay_pointer(result_data);

                std::memcpy(input_result_buffer, input_portion.data(), input_portion.size());
                std::memcpy(descriptor.buffer_return_address, static_cast<void*>(&result_data), sizeof(module_mediator::memory));

                local_input_queue.pop();
                module_mediator::fast_call<module_mediator::return_value>(
                    interoperation::get_module_part(),
                    interoperation::index_getter::execution_module(),
                    interoperation::index_getter::execution_module_make_runnable(),
                    descriptor.thread_id
                );
            }
        }

        LOG_INFO(
            interoperation::get_module_part(),
            "Input worker is shutting down. Detaching from stdio."
        );
    }

    std::thread output_worker_thread{};
    void output_worker(HANDLE hStdOut, HANDLE hCancelIO) {
        LOG_INFO(
            interoperation::get_module_part(),
            "PRTS is attached to stdio. Starting output worker."
        );

        DWORD offset;
        DWORD offsetHigh;

        while (check_stdio_attached()) {
            std::unique_lock<std::mutex> lock(output_queue::lock);
            output_queue::signaling.wait(lock, [] {
                return !output_queue::output_queue.empty() || !check_stdio_attached();
            });

            if (!check_stdio_attached()) {
                clean_output_queue();
                break;
            }

            thread_output_descriptor descriptor = std::move(output_queue::output_queue.front());
            output_queue::output_queue.pop();

            lock.unlock(); //It is critical that we do not hold the lock while processing output.

            if ([[maybe_unused]] bool shutdown = PushStdOut(hStdOut, hCancelIO, descriptor.buffer_address, offset, offsetHigh)) {
                lock.lock();
                clean_output_queue();

                break;
            }

            module_mediator::fast_call<module_mediator::return_value>(
                interoperation::get_module_part(),
                interoperation::index_getter::execution_module(),
                interoperation::index_getter::execution_module_make_runnable(),
                descriptor.thread_id
            );
        }

        LOG_INFO(
            interoperation::get_module_part(),
            "Output worker is shutting down. Detaching from stdio."
        );
    }

    void cleanse_string(module_mediator::memory string, module_mediator::eight_bytes size) {
        assert(string != nullptr && size != 0);
        char* string_data = static_cast<char*>(string);
        string_data[size - 1] = '\0';
    }
}

module_mediator::return_value attach_to_stdio(module_mediator::arguments_string_type) {
    std::lock_guard<std::shared_mutex> lock{ io_synchronizer };
    if (is_stdin_attached) {
        return module_mediator::module_success;
    }

    capturedStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (capturedStdOut == INVALID_HANDLE_VALUE || capturedStdOut == NULL) {
       LOG_PROGRAM_WARNING(
           interoperation::get_module_part(),
            std::format(
                "Failed to get STDOUT handle with error code {}. ",
                GetLastError()
            )
       );

       return module_mediator::module_failure;
    }

    capturedStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (capturedStdIn == INVALID_HANDLE_VALUE || capturedStdIn == NULL) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to get STDIN handle with error code {}. ",
                GetLastError()
            )
        );

        capturedStdOut = NULL;
        return module_mediator::module_failure;
    }

    is_stdin_attached = true;
    IOCancellationSignal = CreateEventA(
        NULL, 
        TRUE,    
        FALSE,   
        NULL 
    );

    if (GetFileType(capturedStdIn) == FILE_TYPE_CHAR && !InitializeConsoleInput(capturedStdIn, savedConsoleState)) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to initialize console input with error code {}. " \
                "This error may be generated if stdio is redirected.",
                GetLastError()
            )
        );
    }

    input_worker_thread = std::thread(input_worker, capturedStdIn, IOCancellationSignal);
    output_worker_thread = std::thread(output_worker, capturedStdOut, IOCancellationSignal);

    return module_mediator::module_success;
}

module_mediator::return_value detach_from_stdio(module_mediator::arguments_string_type) {
    std::unique_lock<std::shared_mutex> lock(io_synchronizer);
    if (!is_stdin_attached) {
        return module_mediator::module_success;
    }

    capturedStdOut = NULL;
    capturedStdIn = NULL;

    is_stdin_attached = false;

    //Now perform shutdown routines for both input and output workers.
    //It tries to cancel all asynchronous I/O operations first, then it tries to cancel synchronous ones.
    //Then it just detaches the worker threads. If they never woke up then we have a resource leak. Otherwise, they will exit gracefully.
    //If they wake up somewhere later after the cancellation, they will just exit without doing anything.
    lock.unlock();

    BOOL signalIOEnd = SetEvent(IOCancellationSignal);
    if (!signalIOEnd) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to set event for I/O cancellation with error code {}. " \
                "This will lead to resource leaks. Detaching worker threads as a last-ditch effort.",
                GetLastError()
            )
        );

        input_worker_thread.detach();
        output_worker_thread.detach();

        return module_mediator::module_failure;
    }

    BOOL cancelSynchronousInput = CancelSynchronousIo(input_worker_thread.native_handle());
    if (!cancelSynchronousInput) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to cancel synchronous I/O for input worker with error code {}. " \
                "This may lead to resource leaks.",
                GetLastError()
            )
        );
    }

    BOOL cancelSynchronousOutput = CancelSynchronousIo(output_worker_thread.native_handle());
    if (!cancelSynchronousOutput) {
       LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to cancel synchronous I/O for output worker with error code {}. " \
                "This may lead to resource leaks.",
                GetLastError()
            )
       );
    }

    if (GetFileType(capturedStdIn) == FILE_TYPE_CHAR && !RestoreConsoleInput(capturedStdIn, savedConsoleState)) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to restore console input with error code {}. " \
                "This error may be generated if stdio is redirected.",
                GetLastError()
            )
        );
    }

    //This will block if the shutdown sequence fails.
    input_worker_thread.join();
    output_worker_thread.join();

    CloseHandleReport(IOCancellationSignal, "IOCancellationSignal");
    IOCancellationSignal = NULL;

    return module_mediator::module_success;
}

module_mediator::return_value callback_register_output(module_mediator::arguments_string_type bundle) {
    auto [thread_id, buffer_address] = 
        module_mediator::respond_callback<module_mediator::return_value, module_mediator::memory>::unpack(bundle);

    std::unique_lock<std::mutex> lock(output_queue::lock);
    output_queue::output_queue.push({
        .thread_id = thread_id,
        .buffer_address = buffer_address
    });

    lock.unlock();
    output_queue::signaling.notify_one();

    return module_mediator::module_success;
}

module_mediator::return_value out(module_mediator::arguments_string_type bundle) {
    std::shared_lock<std::shared_mutex> lock(io_synchronizer);
    if (!is_stdin_attached) {
        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            "PRTS is not attached to stdio. Cannot write to stdout."
        );

        return module_mediator::execution_result_continue;
    }

    module_mediator::fast_call(
    );
}
