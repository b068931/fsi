#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <utility>
#include <limits>

#include "global_crash_handle_setup.h"

// Handle SEH exceptions
namespace {
    LPTOP_LEVEL_EXCEPTION_FILTER g_pPreviousFilter = nullptr;

    void SafeAppend(char* sBuffer, std::size_t szBufferSize, std::size_t* szBufferPosition, const char* sAppend) {
        if (!sBuffer || !sAppend) {
            return;
        }
        while (*sAppend && *szBufferPosition + 1 < szBufferSize) {
            sBuffer[(*szBufferPosition)++] = *sAppend++;
        }

        if (szBufferSize) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    // write "0x" + hex digits (uppercase)
    void AppendPointerHex(char* sBuffer, std::size_t szBufferSize, std::size_t* szBufferPosition, std::uintptr_t lpValue) {
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "0x");

        // each byte is two letters + null terminator
        char sTemporary[sizeof(lpValue) * 2 + 1]{};
        int iDigitIndex = 0;
        if (lpValue == 0) {
            sTemporary[iDigitIndex++] = '0';
        }
        else {
            while (lpValue && std::cmp_less(iDigitIndex, std::size(sTemporary) - 1)) {
                int iDigit = static_cast<int>(lpValue & 0xF);
                sTemporary[iDigitIndex++] = static_cast<char>(
                    iDigit < 10 ? '0' + iDigit : 'A' + (iDigit - 10)
                    );

                lpValue >>= 4;
            }
        }

        for (int iIndex = iDigitIndex - 1; iIndex >= 0; --iIndex) {
            if (*szBufferPosition + 1 < szBufferSize) {
                sBuffer[(*szBufferPosition)++] = sTemporary[iIndex];
            }
            else {
                break;
            }
        }

        if (szBufferSize) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    void AppendDecU32(char* sBuffer, std::size_t szBufferSize, std::size_t* szBufferPosition, std::uint32_t u32Value) {
        // extra digit, which can't be fully represented in 32 bits + null terminator
        char sTemporary[std::numeric_limits<std::uint32_t>::digits10 + 2]{};
        int iDigitIndex = 0;

        if (u32Value == 0) {
            sTemporary[iDigitIndex++] = '0';
        }
        else {
            while (u32Value && std::cmp_less(iDigitIndex, std::size(sTemporary) - 1)) {
                sTemporary[iDigitIndex++] = static_cast<char>('0' + u32Value % 10);
                u32Value /= 10;
            }
        }

        for (int iIndex = iDigitIndex - 1; iIndex >= 0; --iIndex) {
            if (*szBufferPosition + 1 < szBufferSize) {
                sBuffer[(*szBufferPosition)++] = sTemporary[iIndex];
            }
            else {
                break;
            }
        }

        if (szBufferSize) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    // best-effort full write to a handle
    BOOL WriteAll(HANDLE hOut, const char* data, DWORD dwTotalLength) {
        DWORD writtenTotal = 0;
        while (writtenTotal < dwTotalLength) {
            DWORD written = 0;
            if (!WriteFile(hOut, data + writtenTotal, dwTotalLength - writtenTotal, &written, nullptr)) {
                return FALSE;
            }
            if (!written) {
                return FALSE;
            }

            writtenTotal += written;
        }

        return TRUE;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    LONG WINAPI NotifyFatalUnhandledSEH(EXCEPTION_POINTERS* pExceptionInfo) {
        char sExceptionInfoBuffer[2048]{};
        size_t szBufferPosition = 0;

        SafeAppend(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            "*** FATAL ERROR: UNHANDLED SEH EXCEPTION\n"
        );

        SafeAppend(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            "--> EXCEPTION CODE: "
        );

        AppendPointerHex(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            pExceptionInfo->ExceptionRecord->ExceptionCode
        );

        SafeAppend(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            "\n--> EXCEPTION ADDRESS: "
        );

        AppendPointerHex(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            reinterpret_cast<std::uintptr_t>(pExceptionInfo->ExceptionRecord->ExceptionAddress)
        );

        SafeAppend(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            "\n--> SYSTEM THREAD: "
        );

        AppendDecU32(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            GetCurrentThreadId()
        );

        SafeAppend(
            sExceptionInfoBuffer,
            std::size(sExceptionInfoBuffer),
            &szBufferPosition,
            "\n"
        );

        HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
        if (hStdErr != INVALID_HANDLE_VALUE && hStdErr != nullptr) {
            if (!WriteAll(hStdErr, sExceptionInfoBuffer, static_cast<DWORD>(szBufferPosition))) {
                OutputDebugStringA(sExceptionInfoBuffer);
            }
        }
        else {
            // as last resort, send to debugger
            OutputDebugStringA(sExceptionInfoBuffer);
        }

        if (g_pPreviousFilter != nullptr) {
            return g_pPreviousFilter(pExceptionInfo);
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }
}

BOOL InstallGlobalCrashHandler() {
    ULONG ulDesiredStackSize = 8192;
    BOOL bResult = SetThreadStackGuarantee(&ulDesiredStackSize);
    if (!bResult) {
        return FALSE;
    }

    g_pPreviousFilter = SetUnhandledExceptionFilter(NotifyFatalUnhandledSEH);
    return TRUE;
}
