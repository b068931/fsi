#include "pch.h"
#include "standard_input_output.h"
#include "backend_functions.h"

#include "../logger_module/logging.h"
#include "../module_mediator/local_crash_handle_setup.h"

//This file describes IO logic for the FSI programs through PRTS (Program RunTime Services) module.
//I use global variables because they are isolated to this cpp file, they cannot be accessed elsewhere.
//You should view PRTS as an assortment of "classes" or "objects" that are isolated in their own cpp files.

//Global synchronization primitives to ensure that IO can always be cancelled or processed correctly.
namespace {
    //Readers: input or output operations for stdin/stdout.
    //Writers: functions that attach or detach PRTS from stdio.
    std::shared_mutex io_synchronizer;

    //Mostly bear a cosmetic effect.
    //3: input_worker, output_worker, whatever thread calls detach or attach.
    std::barrier phase_coordination{ 3 };

    //PRTS expects that it is the only entity that can write to stdout and stdin to work correctly.
    bool is_stdio_attached = false;
    bool check_stdio_attached() {
        std::shared_lock<std::shared_mutex> lock(io_synchronizer);
        return is_stdio_attached;
    }
}

//WinAPI thingies for console input/output management.
namespace {
    //Available only if attached.
    HANDLE hCapturedStdOut;
    HANDLE hCapturedStdIn;

    //The handle to the worker thread that is responsible for reading from stdin.
    //Allows to exit the thread gracefully when PRTS is detached from stdin.
    HANDLE hIOCancellationSignal;

    //The state of the console before PRTS attached to it.
    DWORD dwSavedConsoleState;

    bool InitializeConsoleInput(HANDLE hStdin, DWORD& dwOutOriginalMode) {
        if (!GetConsoleMode(hStdin, &dwOutOriginalMode))
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

    BOOL RestoreConsoleInput(HANDLE hStdin, DWORD dwOriginalMode) {
        return SetConsoleMode(hStdin, dwOriginalMode);
    }

    void CloseHandleReport(HANDLE& handle, const char* lpsHandleName) {
        if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
            if (!CloseHandle(handle)) {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Failed to close {} handle with error code {}.",
                        lpsHandleName,
                        GetLastError()
                    )
                );
            }
            else {
                handle = NULL;
            }
        }
    }

    std::pair<std::vector<char>, bool> ConsumeConsoleInput(HANDLE hStdIn, HANDLE hCancelIO) {
        HANDLE haWaitingHandles[]{ hStdIn, hCancelIO };
        std::vector<char> result{};

        // Ensure that PRTS knows that we are waiting on console input, so that it can try and cancel it if needed.
        std::shared_lock synchronous_io_lock{ io_synchronizer };
        while (std::ranges::find(result, '\n') == result.end()) {
            DWORD dwConsoleWaitResult = WaitForMultipleObjects(std::size(haWaitingHandles), haWaitingHandles, FALSE, INFINITE);
            if (dwConsoleWaitResult == WAIT_OBJECT_0 + 1) {
                return { {}, true }; //We received shutdown signal while waiting.
            }

            if (dwConsoleWaitResult == WAIT_OBJECT_0) {
                DWORD dwAvailableInput = 0;
                BOOL bEventsResult = GetNumberOfConsoleInputEvents(hStdIn, &dwAvailableInput);
                if (!bEventsResult) {
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "GetNumberOfConsoleInputEvents failed with error code {}. " \
                            "Cannot read from console input.",
                            GetLastError()
                        )
                    );

                    return { {}, true };
                }

                constexpr DWORD dwBufferSize = 128;

                INPUT_RECORD buffer[dwBufferSize];
                DWORD dwReadEvents = 0;

                DWORD dwAcquiredInput = std::min(dwAvailableInput, dwBufferSize);
                BOOL bEventPeekedResult = PeekConsoleInputA(
                    hStdIn,
                    buffer,
                    dwAcquiredInput,
                    &dwReadEvents
                );

                if (!bEventPeekedResult) {
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "ReadConsoleInput failed with error code {}. " \
                            "Cannot read from console input.",
                            GetLastError()
                        )
                    );

                    return { {}, true };
                }

                WORD dwTotalTextSize = 0;
                for (DWORD dwInputBufferIndex = 0; dwInputBufferIndex < dwReadEvents; ++dwInputBufferIndex) {
                    if (buffer[dwInputBufferIndex].EventType == KEY_EVENT) {
                        const KEY_EVENT_RECORD& keyEvent = buffer[dwInputBufferIndex].Event.KeyEvent;
                        if (keyEvent.bKeyDown) {
                            if (keyEvent.uChar.AsciiChar >= 0 && keyEvent.uChar.AsciiChar < 128) {  // NOLINT(clang-diagnostic-tautological-constant-out-of-range-compare)
                                dwTotalTextSize += keyEvent.wRepeatCount;
                            }
                        }
                        else if (keyEvent.wVirtualKeyCode == VK_RETURN) {
                            dwTotalTextSize += 1;
                        }
                    }
                }

                if (dwTotalTextSize > 0) {
                    DWORD dwTextRead = 0;
                    std::unique_ptr<CHAR[]> lpTextBuffer{ new CHAR[dwTotalTextSize] };

                    BOOL bConsoleReadResult = ReadConsoleA(
                        hStdIn,
                        lpTextBuffer.get(),
                        dwTotalTextSize,
                        &dwTextRead,
                        NULL
                    );

                    if (!bConsoleReadResult) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "ReadConsole failed with error code {}. " \
                                "Cannot read from console input.",
                                GetLastError()
                            )
                        );

                        return { {}, true };
                    }

                    for (DWORD dwTextIndex = 0; dwTextIndex < dwTextRead; ++dwTextIndex) {
                        if (lpTextBuffer[dwTextIndex] == '\x1a') {
                            // Ctrl+Z is pressed, EOF.
                            return { result, true };
                        }
                        if (lpTextBuffer[dwTextIndex] != '\r') {
                            result.push_back(lpTextBuffer[dwTextIndex]);
                        }
                    }
                }
            }
            else {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Unexpected wait result {}. Received error: {}.",
                        dwConsoleWaitResult,
                        GetLastError()
                    )
                );

                return { {}, true };
            }
        }

        return { result, false };
    }

    //This function is unified for both file and pipe input.
    //The absolute monster that handles both files and pipes. (Refactoring likely needed)
    //With all possible edge cases and fallback to synchronous IO if overlapped IO fails.
    template<auto file_type>
    std::pair<std::vector<char>, bool> ConsumeAsynchronous(
        HANDLE hStdIn, 
        HANDLE hCancelIO, 
        DWORD& dwOffset, 
        DWORD& dwOffsetHigh
    ) {
        constexpr bool is_pipe = file_type == FILE_TYPE_PIPE;
        HANDLE haWaitHandles[]{ hStdIn, hCancelIO };

        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEvent(
            NULL, 
            TRUE,
            FALSE,
            NULL
        );
        
        if (overlapped.hEvent == NULL) {
            LOG_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "CreateEvent failed with error code {}. " \
                    "Cannot perform asynchronous read operation for file.",
                    GetLastError()
                )
            );

            return { {}, true };
        }
        
        if constexpr (is_pipe) {
            overlapped.Offset = 0;
            overlapped.OffsetHigh = 0;
        }
        else {
            overlapped.Offset = dwOffset;
            overlapped.OffsetHigh = dwOffsetHigh;
        }

        constexpr DWORD dwDefaultBufferSize = 4096;
        DWORD dwBufferSize = 0;

        if constexpr (is_pipe) {
            DWORD dwMessageLeft = 0;
            PeekNamedPipe(hStdIn, NULL, 0, NULL, &dwBufferSize, &dwMessageLeft);

            dwBufferSize = std::max(dwBufferSize, dwMessageLeft);
            if (dwBufferSize == 0) { //If we still don't have any idea about what the buffer size should be.
                dwBufferSize = dwDefaultBufferSize;
            }
        }
        else {
            dwBufferSize = dwDefaultBufferSize;
        }

        std::unique_ptr<CHAR[]> buffer{ new CHAR[dwBufferSize] {} };
        DWORD dwBytesRead = 0;
        
        BOOL bReadResult = ReadFile(
            hStdIn,
            buffer.get(),
            dwBufferSize,
            &dwBytesRead,
            &overlapped
        );
        
        if (!bReadResult) { //This is just how ridiculously complicated overlapped I/O is in Windows.
            if (GetLastError() == ERROR_IO_PENDING) {
                DWORD waitResult = WaitForMultipleObjects(
                    std::size(haWaitHandles),
                    haWaitHandles,
                    FALSE,
                    INFINITE
                );

                if (waitResult == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(hStdIn, &overlapped, &dwBytesRead, FALSE)) {
                        CloseHandleReport(overlapped.hEvent, "overlapped.hEvent");
                        if (GetLastError() == ERROR_HANDLE_EOF) {
                            return { { buffer.get(), buffer.get() + dwBytesRead }, false };
                        }

                        if constexpr (is_pipe) {
                            if (GetLastError() == ERROR_BROKEN_PIPE) {
                                LOG_WARNING(
                                    interoperation::get_module_part(),
                                    std::format(
                                        "Pipe closed before detaching PRTS from stdio."
                                    )
                                );

                                return { { buffer.get(), buffer.get() + dwBytesRead }, true};
                            }

                            if (GetLastError() == ERROR_MORE_DATA) {
                                LOG_WARNING(
                                    interoperation::get_module_part(),
                                    std::format(
                                        "Unexpected more data is available in a pipe. A second call will be required."
                                    )
                                );

                                auto [result, shutdown] = 
                                    ConsumeAsynchronous<file_type>(hStdIn, hCancelIO, dwOffset, dwOffsetHigh);

                                result.insert(result.begin(), buffer.get(),
                                    buffer.get() + dwBytesRead);

                                return { result, shutdown };
                            }
                        }

                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "GetOverlappedResult failed with error code {}. " \
                                "Cannot read from file.",
                                GetLastError()
                            )
                        );

                        return { {}, true };
                    }
                }
                else if (waitResult == WAIT_OBJECT_0 + 1) {
                    BOOL bIOCancelResult = CancelIo(hStdIn);
                    if (!bIOCancelResult) {
                        LOG_WARNING(
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
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "WaitForMultipleObjects failed with error code {}. " \
                            "Cannot read from file.",
                            GetLastError()
                        )
                    );

                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return { {}, true };
                }
            }
            else if (GetLastError() == ERROR_HANDLE_EOF) {
                CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                return { {}, false };
            }
            else {
                if constexpr (is_pipe) {
                    if (GetLastError() == ERROR_BROKEN_PIPE) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Pipe closed before detaching PRTS from stdio."
                            )
                        );

                        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                        return { { buffer.get(), buffer.get() + dwBytesRead }, true};
                    }

                    if (GetLastError() == ERROR_MORE_DATA) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Unexpected more data is available in a pipe. A second call will be required."
                            )
                        );

                        auto [result, shutdown] = 
                            ConsumeAsynchronous<file_type>(hStdIn, hCancelIO, dwOffset, dwOffsetHigh);

                        result.insert(result.begin(), buffer.get(),
                            buffer.get() + dwBytesRead);

                        return { result, shutdown };
                    }
                }

                std::shared_lock synchronous_io_lock{ io_synchronizer };
                BOOL bSynchronousReadResult = ReadFile(
                    hStdIn,
                    buffer.get(),
                    dwBufferSize - 1,
                    &dwBytesRead,
                    NULL
                );

                synchronous_io_lock.unlock();
                if (WaitForSingleObject(hCancelIO, 0) == WAIT_OBJECT_0 || GetLastError() == ERROR_OPERATION_ABORTED) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return { {}, true };
                }

                //This also means that EOF was reached. Applies to files only.
                if (bSynchronousReadResult && dwBytesRead == 0) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return { {}, false };
                }

                if (!bSynchronousReadResult) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    if constexpr (is_pipe) {
                        if (GetLastError() == ERROR_BROKEN_PIPE) {
                            LOG_WARNING(
                                interoperation::get_module_part(),
                                std::format(
                                    "Pipe closed before detaching PRTS from stdio."
                                )
                            );

                            return { { buffer.get(), buffer.get() + dwBytesRead }, true };
                        }

                        if (GetLastError() == ERROR_MORE_DATA) {
                            LOG_WARNING(
                                interoperation::get_module_part(),
                                std::format(
                                    "Unexpected more data is available in a pipe. A second call will be required."
                                )
                            );

                            auto [result, shutdown] = 
                                ConsumeAsynchronous<file_type>(hStdIn, hCancelIO, dwOffset, dwOffsetHigh);

                            result.insert(result.begin(), buffer.get(),
                                buffer.get() + dwBytesRead);

                            return { result, shutdown };
                        }
                    }

                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "ReadFile failed with error code {}. " \
                            "Cannot read from file both async and sync have failed.",
                            GetLastError()
                        )
                    );

                    return { {}, true };
                }
            }
        }

        //Files must have their offsets updated after a successful read.
        if constexpr (!is_pipe) {
            ULARGE_INTEGER newPosition;
            newPosition.LowPart = dwOffset;
            newPosition.HighPart = dwOffsetHigh;
            newPosition.QuadPart += dwBytesRead;

            dwOffset = newPosition.LowPart;
            dwOffsetHigh = newPosition.HighPart;
        }
        
        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
        return { { buffer.get(), buffer.get() + dwBytesRead }, false};
    }

    //Dispatches the input reading operation based on the type of the input handle.
    std::pair<std::vector<char>, bool> ConsumeStdIn(
        HANDLE hStdIn, 
        HANDLE hCancelIO, 
        DWORD& dwOverlappedOffset, 
        DWORD& dwOverlappedOffsetHigh
    ) {
        switch (GetFileType(hStdIn)) {
            case FILE_TYPE_CHAR: {
                return ConsumeConsoleInput(hStdIn, hCancelIO);
            }
            case FILE_TYPE_DISK: {
                //For files and pipes, we are looking for a newline character to determine the end of input.
                //All other input goes into the buffer.
                return ConsumeAsynchronous<FILE_TYPE_DISK>(
                    hStdIn,
                    hCancelIO,
                    dwOverlappedOffset,
                    dwOverlappedOffsetHigh
                );
            }
            case FILE_TYPE_PIPE: {
               return ConsumeAsynchronous<FILE_TYPE_PIPE>(
                    hStdIn,
                    hCancelIO,
                    dwOverlappedOffset,
                    dwOverlappedOffsetHigh
                );
            }
            default: {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Unknown file type for STDIN: {}. " \
                        "Cannot read from stdin. Shutting down the worker.",
                        GetFileType(hCapturedStdIn)
                    )
                );

                return { {}, true };
            }
        }
    }

    bool PushConsoleOutput(
        HANDLE hStdOut, 
        module_mediator::memory output_buffer,
        module_mediator::eight_bytes buffer_size
    ) {
        if (buffer_size == 0) return false;
        if (buffer_size > std::numeric_limits<DWORD>::max()) {
            LOG_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Buffer size {} exceeds maximum allowed size of {}.",
                    buffer_size,
                    std::numeric_limits<DWORD>::max()
                )
            );

            return true;
        }

        DWORD dwBytesWritten = 0;
        std::shared_lock synchronous_io_lock{ io_synchronizer };

        BOOL writeResult = WriteConsoleA(
            hStdOut,
            output_buffer,
            static_cast<DWORD>(buffer_size),
            &dwBytesWritten,
            NULL
        );

        synchronous_io_lock.unlock();
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            return true;
        }

        if (!writeResult || dwBytesWritten == 0) {
            LOG_WARNING(
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

    //Similarly to ConsumeAsynchronous, this function handles both files and pipes.
    //It uses overlapped IO for files and pipes, and falls back to synchronous IO if necessary.
    template<auto file_type>
    bool PushAsynchronous(
        HANDLE hStdOut, 
        HANDLE hCancelIO, 
        module_mediator::memory output_buffer,
        module_mediator::eight_bytes buffer_size,
        DWORD& dwOffset, 
        DWORD& dwOffsetHigh
    ) {
        constexpr bool is_pipe = file_type == FILE_TYPE_PIPE;
        HANDLE waitHandles[]{ hStdOut, hCancelIO };

        if (buffer_size == 0) return false;
        if (buffer_size > std::numeric_limits<DWORD>::max()) {
            LOG_WARNING(
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
            LOG_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "CreateEvent failed with error code {}. " \
                    "Cannot perform asynchronous write operation.",
                    GetLastError()
                )
            );

            return true;
        }

        if constexpr (is_pipe) {
            overlapped.Offset = 0;
            overlapped.OffsetHigh = 0;
        }
        else {
            overlapped.Offset = dwOffset;
            overlapped.OffsetHigh = dwOffsetHigh;
        }

        DWORD dwBytesWritten = 0;
        BOOL bWriteResult = WriteFile(
            hStdOut,
            output_buffer,
            static_cast<DWORD>(buffer_size),
            &dwBytesWritten,
            &overlapped
        );

        if (!bWriteResult) {
            if (GetLastError() == ERROR_IO_PENDING) {
                DWORD dwWaitResult = WaitForMultipleObjects(
                    std::size(waitHandles),
                    waitHandles,
                    FALSE,
                    INFINITE
                );

                if (dwWaitResult == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(hStdOut, &overlapped, &dwBytesWritten, FALSE)) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "GetOverlappedResult failed with error code {}. " \
                                "Cannot write to file. Is pipe: {}.",
                                GetLastError(),
                                is_pipe
                            )
                        );

                        CloseHandle(overlapped.hEvent);
                        return true;
                    }
                }
                else if (dwWaitResult == WAIT_OBJECT_0 + 1) {
                    BOOL bIOCancelResult = CancelIo(hStdOut);
                    if (!bIOCancelResult) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "CancelIo failed with error code {}. " \
                                "Cannot cancel I/O operation. Is pipe: {}.",
                                GetLastError(),
                                is_pipe
                            )
                        );
                    }

                    CloseHandle(overlapped.hEvent);
                    return true;
                }
                else {
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "WaitForSingleObject failed with error code {}. " \
                            "Cannot write to file/pipe. Is pipe: {}.",
                            GetLastError(),
                            is_pipe
                        )
                    );

                    return true;
                }
            }
            else {
                if constexpr (is_pipe) {
                    if (GetLastError() == ERROR_BROKEN_PIPE) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Pipe closed before detaching PRTS from stdio."
                            )
                        );

                        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                        return true;
                    }
                }

                std::shared_lock synchronous_io_lock{ io_synchronizer };
                BOOL bSynchronousWriteResult = WriteFile(
                    hStdOut,
                    output_buffer,
                    static_cast<DWORD>(buffer_size),
                    &dwBytesWritten,
                    NULL
                );

                synchronous_io_lock.unlock();
                if (WaitForSingleObject(hCancelIO, 0) == WAIT_OBJECT_0 || GetLastError() == ERROR_OPERATION_ABORTED) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    return true;
                }

                if (!bSynchronousWriteResult) {
                    CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "WriteFile failed with error code {}. " \
                            "Cannot write to file both async and sync have failed. Is pipe: {}.",
                            GetLastError(),
                            is_pipe
                        )
                    );

                    return true;
                }
            }
        }

        if constexpr (!is_pipe) {
            BOOL bResult = FlushFileBuffers(hStdOut);
            if (!bResult) {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "FlushFileBuffers failed with error code {}. " \
                        "Cannot flush file buffers.",
                        GetLastError()
                    )
                );

                CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
                return true;
            }
        }

        //Files must have their offsets updated after a successful write.
        CloseHandleReport(overlapped.hEvent, "overlapped.HEvent");
        if constexpr (!is_pipe) {
            ULARGE_INTEGER newPosition;
            newPosition.LowPart = dwOffset;
            newPosition.HighPart = dwOffsetHigh;
            newPosition.QuadPart += dwBytesWritten;

            dwOffset = newPosition.LowPart;
            dwOffsetHigh = newPosition.HighPart;
        }

        return false;
    }

    bool PushStdOut(
        HANDLE hStdOut, 
        HANDLE hCancelIO, 
        module_mediator::memory output_buffer, 
        module_mediator::eight_bytes buffer_size, 
        DWORD& dwOverlappedOffset, 
        DWORD& dwOverlappedOffsetHigh
    ) {
        switch (GetFileType(hStdOut)) {
            case FILE_TYPE_CHAR: {
                return PushConsoleOutput(hStdOut, output_buffer, buffer_size);
            }
            case FILE_TYPE_DISK: {
                return PushAsynchronous<FILE_TYPE_DISK>(
                    hStdOut,
                    hCancelIO,
                    output_buffer,
                    buffer_size,
                    dwOverlappedOffset,
                    dwOverlappedOffsetHigh
                );
            }
            case FILE_TYPE_PIPE: {
                return PushAsynchronous<FILE_TYPE_PIPE>(
                    hStdOut,
                    hCancelIO,
                    output_buffer,
                    buffer_size,
                    dwOverlappedOffset,
                    dwOverlappedOffsetHigh
                );
            }
            default: {
                LOG_WARNING(
                    interoperation::get_module_part(),
                    std::format(
                        "Unknown file type for STDOUT: {}. " \
                        "Cannot write to stdout.",
                        GetFileType(hCapturedStdOut)
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
        module_mediator::memory return_address;

        module_mediator::memory input_buffer;
        module_mediator::eight_bytes buffer_size;
    };

    struct thread_output_descriptor {
        module_mediator::return_value thread_id;

        module_mediator::memory buffer_address;
        module_mediator::eight_bytes buffer_size;
    };

    //Both are used by their respective worker threads to ensure that all IO is asynchronous.
    namespace input_queue {
        std::mutex lock;
        std::condition_variable signaling;
        std::queue<thread_input_descriptor> input_queue;
    }

    void clean_input_queue() {
        if (!input_queue::input_queue.empty()) {
            LOG_WARNING(
                interoperation::get_module_part(),
                "Input queue was not empty before detaching. All threads will be woken up."
            );
        }

        while (!input_queue::input_queue.empty()) {
            thread_input_descriptor descriptor = std::move(input_queue::input_queue.front());
            input_queue::input_queue.pop();

            //So as not to trick the thread into reading or writing to a buffer that is not initialized.
            module_mediator::eight_bytes input_size{ 0 };
            std::memcpy(descriptor.input_buffer, &input_size, sizeof(module_mediator::eight_bytes));

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
            LOG_WARNING(
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

//IO worker threads.
namespace {
    std::thread input_worker_thread{};
    void input_worker(HANDLE hStdIn, HANDLE hCancelIO) {
        // We can't call this after LOG_* function, because it might fail.
        module_mediator::crash_handling::install_crash_handlers();

        LOG_INFO(
            interoperation::get_module_part(),
            std::format(
                "PRTS is attached to stdio. Starting input worker. System thread is: {}.",
                GetCurrentThreadId()
            )
        );

        ULONG ulDesiredStackSize = 8192;
        BOOL bResult = SetThreadStackGuarantee(&ulDesiredStackSize);
        if (!bResult) {
            LOG_WARNING(
                interoperation::get_module_part(),
                "Failed to set thread stack guarantee for input worker. " \
                "This may impede fatal error reporting."
            );
        }

        phase_coordination.arrive_and_wait();

        DWORD dwOverlappedOffset = 0;
        DWORD dwOverlappedOffsetHigh = 0;

        std::vector<char> global_input_buffer{};
        bool input_shutdown = false;

        while (check_stdio_attached()) {
            std::queue<thread_input_descriptor> local_input_queue{};
            std::unique_lock<std::mutex> lock(input_queue::lock);

            input_queue::signaling.wait(lock, [] {
                return !check_stdio_attached() || !input_queue::input_queue.empty();
            });

            //Exit prematurely if PRTS is detached from stdio.
            if (!check_stdio_attached()) {
                break;
            }

            while (!input_queue::input_queue.empty()) {
                local_input_queue.push(std::move(input_queue::input_queue.front()));
                input_queue::input_queue.pop();
            }

            lock.unlock(); //It is critical that we do not hold the lock while processing input.
            while (!local_input_queue.empty()) {
                thread_input_descriptor descriptor = std::move(local_input_queue.front());
                std::vector<char> input_portion{};

                if (!global_input_buffer.empty() && global_input_buffer.size() <= descriptor.buffer_size) {
                    input_portion = std::move(global_input_buffer);
                    global_input_buffer = {};
                }
                else if (descriptor.buffer_size < global_input_buffer.size()) {
                    input_portion.insert(
                        input_portion.end(),
                        global_input_buffer.begin(),
                        global_input_buffer.begin() + static_cast<std::ptrdiff_t>(descriptor.buffer_size)
                    );

                    global_input_buffer.erase(
                        global_input_buffer.begin(),
                        global_input_buffer.begin() + static_cast<std::ptrdiff_t>(descriptor.buffer_size)
                    );
                }
                else if (!input_shutdown) {
                    auto [input_data, shutdown_requested] =
                        ConsumeStdIn(hStdIn, hCancelIO, dwOverlappedOffset, dwOverlappedOffsetHigh);

                    input_shutdown = shutdown_requested;
                    std::size_t additional_input = descriptor.buffer_size - input_portion.size();

                    std::ptrdiff_t consume_end =
                        static_cast<std::ptrdiff_t>(std::min(additional_input, input_data.size()));

                    input_portion.insert(
                        input_portion.end(),
                        input_data.begin(),
                        input_data.begin() + consume_end
                    );

                    global_input_buffer.insert(
                        global_input_buffer.end(),
                        input_data.begin() + consume_end,
                        input_data.end()
                    );
                }

                module_mediator::eight_bytes input_size{ input_portion.size() };

                std::memcpy(descriptor.input_buffer, input_portion.data(), input_portion.size());
                std::memcpy(descriptor.return_address, &input_size, sizeof(module_mediator::eight_bytes));

                local_input_queue.pop();
                module_mediator::fast_call<module_mediator::return_value>(
                    interoperation::get_module_part(),
                    interoperation::index_getter::execution_module(),
                    interoperation::index_getter::execution_module_make_runnable(),
                    descriptor.thread_id
                );
            }
        }

        phase_coordination.arrive_and_wait();
        LOG_INFO(
            interoperation::get_module_part(),
            "Input worker is shutting down. Detaching from stdio."
        );
    }

    std::thread output_worker_thread{};
    void output_worker(HANDLE hStdOut, HANDLE hCancelIO) {
        // We can't call this after LOG_* function, because it might fail.
        module_mediator::crash_handling::install_crash_handlers();

        LOG_INFO(
            interoperation::get_module_part(),
            std::format(
                "PRTS is attached to stdio. Starting output worker. System thread is: {}.",
                GetCurrentThreadId()
            )
        );

        ULONG ulDesiredStackSize = 8192;
        BOOL bResult = SetThreadStackGuarantee(&ulDesiredStackSize);
        if (!bResult) {
            LOG_WARNING(
                interoperation::get_module_part(),
                "Failed to set thread stack guarantee for output worker. " \
                "This may impede fatal error reporting."
            );
        }

        phase_coordination.arrive_and_wait();

        DWORD dwOverlappedOffset = 0;
        DWORD dwOverlappedOffsetHigh = 0;

        bool output_shutdown = false;
        while (check_stdio_attached()) {
            std::unique_lock<std::mutex> lock(output_queue::lock);
            output_queue::signaling.wait(lock, [] {
                return  !check_stdio_attached() || !output_queue::output_queue.empty();
            });

            if (!check_stdio_attached()) {
                break;
            }

            thread_output_descriptor descriptor = std::move(output_queue::output_queue.front());
            output_queue::output_queue.pop();

            lock.unlock(); //It is critical that we do not hold the lock while processing output.

            if (!output_shutdown) {
                output_shutdown = PushStdOut(
                    hStdOut,
                    hCancelIO,
                    descriptor.buffer_address,
                    descriptor.buffer_size,
                    dwOverlappedOffset,
                    dwOverlappedOffsetHigh
                );
            }

            module_mediator::fast_call<module_mediator::return_value>(
                interoperation::get_module_part(),
                interoperation::index_getter::execution_module(),
                interoperation::index_getter::execution_module_make_runnable(),
                descriptor.thread_id
            );
        }

        phase_coordination.arrive_and_wait();
        LOG_INFO(
            interoperation::get_module_part(),
            "Output worker is shutting down. Detaching from stdio."
        );
    }
}

module_mediator::return_value attach_to_stdio(module_mediator::arguments_string_type) {
    std::lock_guard<std::shared_mutex> lock{ io_synchronizer };
    if (is_stdio_attached) {
        return module_mediator::module_success;
    }

    hCapturedStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hCapturedStdOut == INVALID_HANDLE_VALUE || hCapturedStdOut == NULL) {
       LOG_WARNING(
           interoperation::get_module_part(),
            std::format(
                "Failed to get STDOUT handle with error code {}. ",
                GetLastError()
            )
       );

       return module_mediator::module_failure;
    }

    hCapturedStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hCapturedStdIn == INVALID_HANDLE_VALUE || hCapturedStdIn == NULL) {
        LOG_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to get STDIN handle with error code {}. ",
                GetLastError()
            )
        );

        hCapturedStdOut = NULL;
        return module_mediator::module_failure;
    }

    if (GetFileType(hCapturedStdIn) == FILE_TYPE_CHAR && !InitializeConsoleInput(hCapturedStdIn, dwSavedConsoleState)) {
        LOG_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to initialize console input with error code {}." \
                "Unexpected console handle.",
                GetLastError()
            )
        );

        hCapturedStdIn = NULL;
        hCapturedStdOut = NULL;

        return module_mediator::module_failure;
    }

    is_stdio_attached = true;
    hIOCancellationSignal = CreateEventA(
        NULL, 
        TRUE,    
        FALSE,   
        NULL 
    );

    input_worker_thread = std::thread(input_worker, hCapturedStdIn, hIOCancellationSignal);
    output_worker_thread = std::thread(output_worker, hCapturedStdOut, hIOCancellationSignal);

    phase_coordination.arrive_and_wait();
    return module_mediator::module_success;
}

module_mediator::return_value detach_from_stdio(module_mediator::arguments_string_type) {
    // Start by setting IO cancellation event so that worker threads can exit overlapped IO, and console IO operations.
    BOOL signalIOEnd = SetEvent(hIOCancellationSignal);
    if (!signalIOEnd) {
        LOG_WARNING(
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

    {
        std::unique_lock<std::shared_mutex> io_lock(io_synchronizer, std::defer_lock);
        std::unique_lock<std::mutex> input_lock(input_queue::lock, std::defer_lock);
        std::unique_lock<std::mutex> output_lock(output_queue::lock, std::defer_lock);

        //We must ensure that workers are either already waiting on their respective condition variables or
        //are yet to acquire their locks. This avoids a potential deadlock when they check whether we are
        //attached to stdio, and we instantly detach from it.
        //Notice that we must use custom backoff algorithm here to avoid another possible deadlock. Simply acquiring mutexes
        //in some specific order won't cut it here.
        while (true) {
            std::this_thread::yield();
            if (!io_lock.try_lock()) {
                //Error not found is documented to be returned if the thread is not waiting on synchronous IO.
                BOOL cancelSynchronousInput = CancelSynchronousIo(input_worker_thread.native_handle());
                if (!cancelSynchronousInput && GetLastError() != ERROR_NOT_FOUND) {
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "Failed to cancel synchronous I/O for input worker with error code {}. " \
                            "This may lead to resource leaks.",
                            GetLastError()
                        )
                    );

                    return module_mediator::module_failure;
                }

                BOOL cancelSynchronousOutput = CancelSynchronousIo(output_worker_thread.native_handle());
                if (!cancelSynchronousOutput && GetLastError() != ERROR_NOT_FOUND) {
                    LOG_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "Failed to cancel synchronous I/O for output worker with error code {}. " \
                            "This may lead to resource leaks.",
                            GetLastError()
                        )
                    );

                    return module_mediator::module_failure;
                }
            }

            is_stdio_attached = false;
            if (!input_lock.try_lock()) {
                io_lock.unlock();
                continue;
            }

            if (!output_lock.try_lock()) {
                io_lock.unlock();
                input_lock.unlock();
                continue;
            }

            break;
        }

        if (hCapturedStdIn == NULL || hCapturedStdOut == NULL || hIOCancellationSignal == NULL) {
            assert(hCapturedStdIn == NULL && hCapturedStdOut == NULL && hIOCancellationSignal == NULL);
            return module_mediator::module_success;
        }
    }

    //We already know that both (or at least one) worker threads are either
    //waiting on their condition variables or are yet to acquire their locks.
    //In the last case they will read is_stdio_attached false and exit.
    input_queue::signaling.notify_one();
    output_queue::signaling.notify_one();

    //This will block if the shutdown sequence fails.
    phase_coordination.arrive_and_wait();

    input_worker_thread.join();
    output_worker_thread.join();

    clean_input_queue();
    clean_output_queue();

    CloseHandleReport(hIOCancellationSignal, "hIOCancellationSignal");
    hIOCancellationSignal = NULL;

    if (GetFileType(hCapturedStdIn) == FILE_TYPE_CHAR && !RestoreConsoleInput(hCapturedStdIn, dwSavedConsoleState)) {
        LOG_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Failed to restore console input with error code {}. " \
                "This error may be generated if stdio is redirected.",
                GetLastError()
            )
        );

        hCapturedStdOut = NULL;
        hCapturedStdIn = NULL;

        return module_mediator::module_failure;
    }

    hCapturedStdOut = NULL;
    hCapturedStdIn = NULL;

    return module_mediator::module_success;
}

module_mediator::return_value callback_register_output(module_mediator::arguments_string_type bundle) {
    auto [thread_id, buffer_address, buffer_size] = 
        module_mediator::respond_callback<module_mediator::return_value, module_mediator::memory, module_mediator::eight_bytes>::unpack(bundle);

    std::unique_lock<std::mutex> lock(output_queue::lock);
    if (!check_stdio_attached()) { //This should not ever lead to a deadlock because we are using a shared lock.
        LOG_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Concurrency error: PRTS was detached from stdio before the thread was added to the output queue." \
                "Waking up the thread {}.",
                thread_id
            ) 
        );

        module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::execution_module(),
            interoperation::index_getter::execution_module_make_runnable(),
            thread_id
        );

        return module_mediator::module_failure;
    }

    output_queue::output_queue.push({
        .thread_id = thread_id,
        .buffer_address = buffer_address,
        .buffer_size = buffer_size
    });

    lock.unlock();
    output_queue::signaling.notify_one();

    return module_mediator::module_success;
}

module_mediator::return_value out(module_mediator::arguments_string_type bundle) {
    auto [output_buffer, output_size] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::eight_bytes>(bundle);

    if (output_size == 0) {
        return module_mediator::execution_result_continue;
    }

    auto [memory, memory_size] = 
        backend::decay_pointer(output_buffer);

    if (memory_size < output_size) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            std::format(
                "Requested output size {} exceeds memory block size of {}.",
                output_size,
                memory_size
            )
        );

        return module_mediator::execution_result_terminate;
    }

    std::shared_lock<std::shared_mutex> lock(io_synchronizer);
    if (!is_stdio_attached) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "PRTS is not attached to stdio. Cannot write to stdout."
        );

        return module_mediator::execution_result_terminate;
    }

    module_mediator::callback_bundle* callback_structure =  
        module_mediator::create_callback<module_mediator::return_value, module_mediator::memory>(
            "prts",
            "callback_register_output",
            interoperation::get_current_thread_id(),
            memory,
            output_size
        );

    module_mediator::fast_call<module_mediator::memory>(
        interoperation::get_module_part(),
        interoperation::index_getter::execution_module(),
        interoperation::index_getter::execution_module_register_deferred_callback(),
        callback_structure
    );

    return module_mediator::execution_result_block;
}

module_mediator::return_value callback_register_input(module_mediator::arguments_string_type bundle) {
    auto [thread_id, return_address, input_buffer, buffer_size] = 
        module_mediator::respond_callback<
            module_mediator::return_value,
            module_mediator::memory,
            module_mediator::memory,
            module_mediator::eight_bytes
        >::unpack(bundle);

    std::unique_lock<std::mutex> lock(input_queue::lock);
    if (!check_stdio_attached()) { //This should not ever lead to a deadlock because we are using a shared lock.
        LOG_WARNING(
            interoperation::get_module_part(),
            std::format(
                "Concurrency error: PRTS was detached from stdio before the thread was added to the input queue." \
                "Waking up the thread {}.",
                thread_id
            )
        );

        module_mediator::eight_bytes input_size{ 0 };
        std::memcpy(input_buffer, &input_size, sizeof(module_mediator::eight_bytes));

        module_mediator::fast_call<module_mediator::return_value>(
            interoperation::get_module_part(),
            interoperation::index_getter::execution_module(),
            interoperation::index_getter::execution_module_make_runnable(),
            thread_id
        );

        return module_mediator::module_failure;
    }

    input_queue::input_queue.push({
        .thread_id = thread_id,
        .return_address = return_address,
        .input_buffer = input_buffer,
        .buffer_size = buffer_size
    });

    lock.unlock();
    input_queue::signaling.notify_one();

    return module_mediator::module_success;
}

module_mediator::return_value in(module_mediator::arguments_string_type bundle) {
    auto [return_address, type, input_buffer, buffer_size] = 
        module_mediator::arguments_string_builder::unpack<
            module_mediator::memory,
            module_mediator::one_byte,
            module_mediator::memory,
            module_mediator::eight_bytes
        >(bundle);

    if (buffer_size == 0) {
        return module_mediator::execution_result_continue;
    }

    auto [memory, memory_size] = 
        backend::decay_pointer(input_buffer);

    if (memory_size < buffer_size) {
       LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            std::format(
                "Requested input size {} exceeds memory block size of {}.",
                buffer_size,
                memory_size
            )
        );

       return module_mediator::execution_result_terminate;
    }

    if (type != module_mediator::eight_bytes_return_value) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "Invalid type for stdio input operation. Expected eight bytes return value."
        );

        return module_mediator::execution_result_terminate;
    }

    std::shared_lock<std::shared_mutex> lock(io_synchronizer);
    if (!is_stdio_attached) {
        LOG_PROGRAM_ERROR(
            interoperation::get_module_part(),
            "PRTS is not attached to stdio. Cannot read from stdin."
        );

        return module_mediator::execution_result_terminate;
    }

    module_mediator::callback_bundle* callback_structure = 
        module_mediator::create_callback<
            module_mediator::return_value,
            module_mediator::memory,
            module_mediator::memory,
            module_mediator::eight_bytes
        >(
            "prts",
            "callback_register_input",
            interoperation::get_current_thread_id(),
            return_address,
            memory,
            buffer_size
        );

    module_mediator::fast_call<module_mediator::memory>(
        interoperation::get_module_part(),
        interoperation::index_getter::execution_module(),
        interoperation::index_getter::execution_module_register_deferred_callback(),
        callback_structure
    );

    return module_mediator::execution_result_block;
}
