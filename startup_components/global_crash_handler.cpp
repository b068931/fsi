#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <utility>
#include <limits>
#include <bit>

#include "global_crash_handler.h"
#include "dbghelp_functions.h"

// Handle SEH exceptions
namespace {
    LPTOP_LEVEL_EXCEPTION_FILTER g_pPreviousFilter = nullptr;

    // Called safe because becomes noop when buffer is full.
    void SafeAppend(
        char* sBuffer, 
        std::size_t szBufferSize, 
        std::size_t* szBufferPosition, 
        const char* sAppend
    ) {
        if (!sBuffer || !sAppend) {
            return;
        }

        while (*sAppend && *szBufferPosition + 1 < szBufferSize) {
            sBuffer[(*szBufferPosition)++] = *sAppend++;
        }

        if (szBufferSize > 0) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    void SafeAppendWideAsUtf8(
        char* sBuffer, 
        std::size_t szBufferSize, 
        std::size_t* szBufferPosition, 
        const wchar_t* wsSource
    ) {
        if (!sBuffer || !wsSource || !szBufferPosition) {
            return;
        }

        std::size_t szRemaining = szBufferSize > *szBufferPosition ? szBufferSize - *szBufferPosition - 1 : 0;
        if (szRemaining == 0) {
            return;
        }

        int iConverted = WideCharToMultiByte(
            CP_UTF8,
            0,
            wsSource,
            -1,
            sBuffer + *szBufferPosition,
            static_cast<int>(szRemaining),
            nullptr,
            nullptr
        );

        if (iConverted > 0) {
            // iConverted includes null terminator, so subtract 1
            *szBufferPosition += static_cast<std::size_t>(iConverted - 1);
        }

        if (szBufferSize > 0) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    // Writes "0x" + hex digits (uppercase).
    void SafeAppendPointerHex(
        char* sBuffer, 
        std::size_t szBufferSize, 
        std::size_t* szBufferPosition, 
        std::uintptr_t lpValue
    ) {
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "0x");

        // Each byte is two letters + null terminator.
        char sTemporary[sizeof(lpValue) * 2 + 1];
        int iDigitIndex = 0;

        SecureZeroMemory(sTemporary, sizeof(sTemporary));

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

        if (szBufferSize > 0) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    void SafeAppendDecimalU32(
        char* sBuffer, 
        std::size_t szBufferSize, 
        std::size_t* szBufferPosition, 
        std::uint32_t u32Value
    ) {
        // Extra digit, which can't be fully represented in 32 bits + null terminator
        char sTemporary[std::numeric_limits<std::uint32_t>::digits10 + 2];
        int iDigitIndex = 0;

        SecureZeroMemory(sTemporary, sizeof(sTemporary));

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

        if (szBufferSize > 0) {
            sBuffer[*szBufferPosition < szBufferSize ? *szBufferPosition : szBufferSize - 1] = '\0';
        }
    }

    // Best-effort full write to a handle.
    [[nodiscard]] BOOL WriteAll(HANDLE hOut, const char* data, DWORD dwTotalLength) {
        DWORD writtenTotal = 0;
        while (writtenTotal < dwTotalLength) {
            DWORD written = 0;
            if (!WriteFile(hOut, data + writtenTotal, dwTotalLength - writtenTotal, 
                &written, nullptr)) {
                return FALSE;
            }

            if (!written) {
                return FALSE;
            }

            writtenTotal += written;
        }

        return TRUE;
    }

    // Try to write to a handle, if that fails fall back to OutputDebugStringW.
    void WriteOutput(const char* sBuffer, std::size_t szLength) {
        HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
        if (hStdErr != INVALID_HANDLE_VALUE && hStdErr != nullptr) {
            if (WriteAll(hStdErr, sBuffer, static_cast<DWORD>(szLength))) {
                return;
            }
        }

        // Convert UTF-8 to wide characters and use OutputDebugStringW
        // Use a fixed stack buffer of ~8KB wide characters
        constexpr int iStackBufferSize = 8192;
        wchar_t wsBuffer[iStackBufferSize];

        int iConverted = MultiByteToWideChar(
            CP_UTF8,
            0,
            sBuffer,
            static_cast<int>(szLength),
            wsBuffer,
            iStackBufferSize - 1
        );

        if (iConverted > 0) {
            wsBuffer[iConverted] = L'\0';
            OutputDebugStringW(wsBuffer);
        }
    }

    // Callback for EnumerateLoadedModulesW64 to load symbols for each module
    [[nodiscard]] BOOL CALLBACK LoadModuleSymbolsCallback(
        PCWSTR ModuleName,
        DWORD64 ModuleBase,
        ULONG ModuleSize,
        PVOID lpContext
    ) {
        using namespace startup_components::dbghelp;

        // Immediately stop if we don't have the required function loaded.
        if (!g_bSymbolsInitialized || !pfnSymLoadModuleExW) {
            return FALSE;
        }

        struct UserContext {
            HANDLE hProcess{};
            DWORD dwFailuresCount{};

            char* sErrorBuffer{};
            std::size_t szErrorBufferSize{};
            std::size_t* szErrorBufferPosition{};

            DWORD64* pdwLoadedModulesBaseAddresses{};
            std::size_t szLoadedModulesBaseAddressesSize{};
            std::size_t* pszLoadedModulesBaseAddressesPosition{};
        };
        
        UserContext* Context = 
            static_cast<UserContext*>(lpContext);

        DWORD64 result = pfnSymLoadModuleExW(
            Context->hProcess,
            nullptr,        // hFile
            ModuleName,     // ImageName
            nullptr,        // ModuleName (use default)
            ModuleBase,
            ModuleSize,
            nullptr,        // Data
            0               // Flags
        );

        if (result == 0 && GetLastError() != ERROR_SUCCESS) {
            ++Context->dwFailuresCount;
            SafeAppendWideAsUtf8(
                Context->sErrorBuffer, 
                Context->szErrorBufferSize, 
                Context->szErrorBufferPosition, 
                ModuleName
            );
        }

        if (result != 0) {
            if (*Context->pszLoadedModulesBaseAddressesPosition >= Context->szLoadedModulesBaseAddressesSize) {
                SafeAppend(
                    Context->sErrorBuffer, 
                    Context->szErrorBufferSize, 
                    Context->szErrorBufferPosition,
                    "Ran out of space while loading symbols!\n"
                );

                // No more space to store loaded module base addresses, stop loading.
                return FALSE;
            }

            Context->pdwLoadedModulesBaseAddresses[
                (*Context->pszLoadedModulesBaseAddressesPosition)++
            ] = ModuleBase;
        }

        // Continues enumeration.
        return TRUE;
    }

    // Ensure that symbols for all loaded modules are available. Store information about those
    // that we forced to load. Returns TRUE on success, FALSE on failure.
    [[nodiscard]] BOOL LoadAllModuleSymbols(
        HANDLE hProcess,
        char* sErrorBuffer,
        std::size_t szErrorBufferSize,
        std::size_t* szErrorBufferPosition,
        DWORD64* pdwLoadedModulesBaseAddresses,
        std::size_t szLoadedModulesBaseAddressesSize,
        std::size_t* pszLoadedModulesBaseAddressesPosition
    ) {
        using namespace startup_components::dbghelp;

        if (!g_bSymbolsInitialized || !pfnEnumerateLoadedModulesW64 || !pfnSymLoadModuleExW) {
            SafeAppend(
                sErrorBuffer, 
                szErrorBufferSize, 
                szErrorBufferPosition, 
                "Symbol loading is unavailable!\n"
            );

            return false;
        }

        struct UserContext {
            HANDLE hProcess{};
            DWORD dwFailuresCount{};

            char* sErrorBuffer{};
            std::size_t szErrorBufferSize{};
            std::size_t* szErrorBufferPosition{};

            DWORD64* pdwLoadedModulesBaseAddresses{};
            std::size_t szLoadedModulesBaseAddressesSize{};
            std::size_t* pszLoadedModulesBaseAddressesPosition{};
        };
        
        UserContext Context{
            .hProcess = hProcess,
            .dwFailuresCount = 0,
            .sErrorBuffer = sErrorBuffer,
            .szErrorBufferSize = szErrorBufferSize,
            .szErrorBufferPosition = szErrorBufferPosition,
            .pdwLoadedModulesBaseAddresses = pdwLoadedModulesBaseAddresses,
            .szLoadedModulesBaseAddressesSize = szLoadedModulesBaseAddressesSize,
            .pszLoadedModulesBaseAddressesPosition = pszLoadedModulesBaseAddressesPosition
        };

        BOOL success = pfnEnumerateLoadedModulesW64(hProcess, LoadModuleSymbolsCallback, &Context) &&
            Context.dwFailuresCount == 0;

        if (!success) {
            SafeAppend(
                sErrorBuffer, 
                szErrorBufferSize, 
                szErrorBufferPosition, 
                "Failed to load symbols for the module(s) listed above.\n"
            );
        }

        return success;
    }

    // Unload all previously loaded symbols for modules.
    [[nodiscard]] BOOL UnloadAllModuleSymbols(
        HANDLE hProcess,
        DWORD64* pdwLoadedModulesBaseAddresses,
        std::size_t szLoadedModulesCount
    ) {
        using namespace startup_components::dbghelp;

        if (!g_bSymbolsInitialized || !pfnSymUnloadModule64) {
            return FALSE;
        }

        BOOL success = TRUE;
        for (std::size_t szCurrent = 0; szCurrent < szLoadedModulesCount; ++szCurrent) {
            success = success && 
                pfnSymUnloadModule64(hProcess, pdwLoadedModulesBaseAddresses[szCurrent]);
        }

        return success;
    }

    // Custom function table access that falls back to RtlLookupFunctionEntry.
    [[nodiscard]] PVOID CALLBACK CustomFunctionTableAccess64(HANDLE hProcess, DWORD64 dwAddrBase) {
        using namespace startup_components::dbghelp;

        // First try DbgHelp's function.
        if (pfnSymFunctionTableAccess64 && g_bSymbolsInitialized) {
            if (PVOID pResult = pfnSymFunctionTableAccess64(hProcess, dwAddrBase)) {
                return pResult;
            }
        }

        // Fall back to RtlLookupFunctionEntry for dynamic code.
        // Function registered with RtlAddFunctionTable is not necessarily registered with DbgHelp,
        // so this is required to cover some edge cases.
#if defined(_M_AMD64) || defined(_M_ARM64)

        DWORD64 dw64ImageBase = 0;
        return RtlLookupFunctionEntry(dwAddrBase, &dw64ImageBase, nullptr);

#else

        return nullptr;

#endif
    }

    // Custom module base function that falls back to RtlLookupFunctionEntry.
    [[nodiscard]] DWORD64 CALLBACK CustomGetModuleBase64(HANDLE hProcess, DWORD64 dwAddr) {
        using namespace startup_components::dbghelp;

        // First try DbgHelp's function.
        if (pfnSymGetModuleBase64 && g_bSymbolsInitialized) {
            if (DWORD64 dwResult = pfnSymGetModuleBase64(hProcess, dwAddr)) {
                return dwResult;
            }
        }

        // Fall back to RtlLookupFunctionEntry to get the image base for dynamic code.
#if defined(_M_AMD64) || defined(_M_ARM64)

        DWORD64 dw64ImageBase = 0;
        if (RtlLookupFunctionEntry(dwAddr, &dw64ImageBase, nullptr)) {
            return dw64ImageBase;
        }

#endif

        // Last resort: try to get module info from address.
        // This function can be really slow if used on a big region.

        MEMORY_BASIC_INFORMATION mbi;
        SecureZeroMemory(&mbi, sizeof(mbi));

        if (VirtualQueryEx(hProcess, std::bit_cast<LPCVOID>(dwAddr), &mbi, sizeof(mbi))) {
            return reinterpret_cast<DWORD64>(mbi.AllocationBase);
        }

        return 0;
    }

    // Try to get module name from an address.
    // Returns true if the address belongs to a loaded module.
    bool GetModuleNameFromAddress(DWORD64 dwAddr, char* sModuleName, std::size_t szModuleNameSize) {
        if (sModuleName && szModuleNameSize > 0) {
            sModuleName[0] = '\0';
        }

        HMODULE hModule = nullptr;
        BOOL bResult = GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            std::bit_cast<LPCSTR>(dwAddr),
            &hModule
        );


        if (!bResult || !hModule) {
            return false;
        }

        if (sModuleName && szModuleNameSize > 0) {
            wchar_t wsFullPath[MAX_PATH];

            SecureZeroMemory(wsFullPath, sizeof(wsFullPath));
            DWORD dwLength = GetModuleFileNameW(hModule, wsFullPath, MAX_PATH);
            
            // Extract just the filename from the full path.
            if (dwLength > 0 && dwLength < MAX_PATH) {
                const wchar_t* wsFileName = wsFullPath;
                for (const wchar_t* iterator = wsFullPath; *iterator; ++iterator) {
                    if (*iterator == L'\\' || *iterator == L'/') {
                        wsFileName = iterator + 1;
                    }
                }

                // Convert to UTF-8.
                int iConverted = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    wsFileName,
                    -1,
                    sModuleName,
                    static_cast<int>(szModuleNameSize),
                    nullptr,
                    nullptr
                );

                if (iConverted == 0) {
                    sModuleName[0] = '\0';
                }
            }
        }

        return true;
    }

    // Check if an address is a dynamic function (JIT compiled, etc.)
    bool IsDynamicFunction(DWORD64 dwAddr) {
#if defined(_M_AMD64) || defined(_M_ARM64)

        DWORD64 dw64ImageBase = 0;
        if (!RtlLookupFunctionEntry(dwAddr, &dw64ImageBase, nullptr)) {
            return false;
        }

        // Check if the address belongs to a loaded module.
        // If it does, it's not a dynamic function (just missing symbols).
        if (GetModuleNameFromAddress(dwAddr, nullptr, 0)) {
            return false;
        }

        // Has runtime function but no module - dynamic function.
        return true;

#else

        (void)dwAddr;
        return false;

#endif
    }

    // Get displacement from function start for dynamic functions.
    DWORD64 GetDynamicFunctionDisplacement(DWORD64 dwAddr) {
#if defined(_M_AMD64) || defined(_M_ARM64)

        DWORD64 dw64ImageBase = 0;
        if (PRUNTIME_FUNCTION pRuntimeFunction = RtlLookupFunctionEntry(dwAddr, &dw64ImageBase, nullptr)) {
            DWORD64 dwFunctionStart = dw64ImageBase + pRuntimeFunction->BeginAddress;
            return dwAddr - dwFunctionStart;
        }

#else

        (void)dwAddr;

#endif
        return 0;
    }

    // Append module name to buffer if available
    void AppendModuleName(
        char* sBuffer,
        std::size_t szBufferSize,
        std::size_t* szBufferPosition,
        DWORD64 dwAddr
    ) {
        char sModuleName[MAX_PATH];
        SecureZeroMemory(sModuleName, sizeof(sModuleName));

        if (GetModuleNameFromAddress(dwAddr, sModuleName, sizeof(sModuleName)) && sModuleName[0] != '\0') {
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, sModuleName);
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "!");
        }
    }

    void AppendSymbolInfo(
        char* sBuffer, 
        std::size_t szBufferSize, 
        std::size_t* szBufferPosition, 
        DWORD64 dwAddr,
        HANDLE hProcess
    ) {
        using namespace startup_components::dbghelp;

        // Always try to append module name first.
        AppendModuleName(sBuffer, szBufferSize, szBufferPosition, dwAddr);

        // No symbol info available, check for dynamic function.
        if (!g_bSymbolsInitialized || !pfnSymFromAddrW) {
            if (IsDynamicFunction(dwAddr)) {
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "[DYNAMIC_FUNCTION]+");
                DWORD64 dwDisplacement = GetDynamicFunctionDisplacement(dwAddr);
                SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, dwDisplacement);
            }
            else {
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "[UNKNOWN]");
            }

            return;
        }

        char symbolBuffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)];
        PSYMBOL_INFOW pSymbol = new(symbolBuffer) SYMBOL_INFOW;

        SecureZeroMemory(pSymbol, sizeof(symbolBuffer));

        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 dw64SymbolDisplacement = 0;
        if (pfnSymFromAddrW(hProcess, dwAddr, &dw64SymbolDisplacement, pSymbol)) {
            SafeAppendWideAsUtf8(sBuffer, szBufferSize, szBufferPosition, pSymbol->Name);
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "+");
            SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, dw64SymbolDisplacement);

            if (pfnSymGetLineFromAddrW64) {
                IMAGEHLP_LINEW64 lineInfo;

                SecureZeroMemory(&lineInfo, sizeof(lineInfo));
                lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

                DWORD dwLineDisplacement = 0;
                if (pfnSymGetLineFromAddrW64(hProcess, dwAddr, &dwLineDisplacement, &lineInfo)) {
                    SafeAppend(sBuffer, szBufferSize, szBufferPosition, " (");
                    SafeAppendWideAsUtf8(sBuffer, szBufferSize, szBufferPosition, lineInfo.FileName);
                    SafeAppend(sBuffer, szBufferSize, szBufferPosition, ":");
                    SafeAppendDecimalU32(sBuffer, szBufferSize, szBufferPosition, lineInfo.LineNumber);
                    SafeAppend(sBuffer, szBufferSize, szBufferPosition, ")");
                }
            }
        }
        else if (IsDynamicFunction(dwAddr)) {
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "[DYNAMIC_FUNCTION]+");
            SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, 
                GetDynamicFunctionDisplacement(dwAddr));
        }
        else {
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "[UNKNOWN]");
        }
    }

    void AppendInlineFrames(
        char* sBuffer,
        std::size_t szBufferSize,
        std::size_t* szBufferPosition,
        DWORD64 dwAddr,
        DWORD64 dwReturnAddr,
        HANDLE hProcess,
        int* piFrameNumber
    ) {
        using namespace startup_components::dbghelp;

        if (!g_bSymbolsInitialized || !pfnSymQueryInlineTrace || 
            !pfnSymFromInlineContextW || !pfnSymGetLineFromInlineContextW ||
            !pfnSymAddrIncludeInlineTrace) {
            return;
        }

        // Get the number of inlined frames.
        DWORD dwInlineTrace = pfnSymAddrIncludeInlineTrace(hProcess, dwAddr);
        if (dwInlineTrace == 0) {
            // The symbol is not within inline frame.
            return;
        }

        DWORD dwInlineContext = 0;
        DWORD dwFrameIndex = 0;

        if (!pfnSymQueryInlineTrace(hProcess, dwAddr, 0, dwReturnAddr, 
            dwAddr, &dwInlineContext, &dwFrameIndex)) {
            return;
        }

        for (DWORD dwCurrent = 0; dwCurrent < dwInlineTrace; ++dwCurrent) {
            char symbolBuffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)];
            PSYMBOL_INFOW pSymbol = new(symbolBuffer) SYMBOL_INFOW;

            SecureZeroMemory(pSymbol, sizeof(symbolBuffer));

            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 dwDisplacement = 0;
            if (pfnSymFromInlineContextW(hProcess, dwAddr, dwInlineContext + dwCurrent, &dwDisplacement, pSymbol)) {
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "    [");
                SafeAppendDecimalU32(sBuffer, szBufferSize, szBufferPosition, static_cast<std::uint32_t>((*piFrameNumber)++));
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "] (inline) ");
                
                // Append module name for inline frame
                AppendModuleName(sBuffer, szBufferSize, szBufferPosition, dwAddr);
                SafeAppendWideAsUtf8(sBuffer, szBufferSize, szBufferPosition, pSymbol->Name);

                // Try to get inline line info (wide character version)
                IMAGEHLP_LINEW64 lineInfo;

                SecureZeroMemory(&lineInfo, sizeof(lineInfo));
                lineInfo.SizeOfStruct = sizeof(lineInfo);

                DWORD dwLineDisplacement = 0;
                DWORD64 dwModuleBase = CustomGetModuleBase64(hProcess, dwAddr);

                if (pfnSymGetLineFromInlineContextW(hProcess, dwAddr, dwInlineContext + dwCurrent, 
                    dwModuleBase, &dwLineDisplacement, &lineInfo)) {
                    SafeAppend(sBuffer, szBufferSize, szBufferPosition, " (");
                    SafeAppendWideAsUtf8(sBuffer, szBufferSize, szBufferPosition, lineInfo.FileName);
                    SafeAppend(sBuffer, szBufferSize, szBufferPosition, ":");
                    SafeAppendDecimalU32(sBuffer, szBufferSize, szBufferPosition, lineInfo.LineNumber);
                    SafeAppend(sBuffer, szBufferSize, szBufferPosition, ")");
                }

                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n");
            }
        }
    }

    void GenerateStackTrace(
        char* sBuffer,
        std::size_t szBufferSize,
        std::size_t* szBufferPosition,
        CONTEXT* pContext
    ) {
        using namespace startup_components::dbghelp;

        if (!g_bDbgHelpLoaded || !pfnStackWalk64) {
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "--> STACK TRACE: [DbgHelp not available]\n");
            return;
        }

        HANDLE hProcess = GetCurrentProcess();
        HANDLE hThread = GetCurrentThread();

        // Make a copy of the context to avoid modifying the original
        CONTEXT contextCopy = *pContext;

        STACKFRAME64 stackFrame;
        DWORD dwMachineType = 0;

        SecureZeroMemory(&stackFrame, sizeof(stackFrame));

#if defined(_M_AMD64)

        dwMachineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = contextCopy.Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = contextCopy.Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = contextCopy.Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

#elif defined(_M_ARM64)

        dwMachineType = IMAGE_FILE_MACHINE_ARM64;
        stackFrame.AddrPC.Offset = contextCopy.Pc;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = contextCopy.Fp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = contextCopy.Sp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

#elif defined(_M_IX86)

        dwMachineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = contextCopy.Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = contextCopy.Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = contextCopy.Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

#elif _M_IA64

        image = IMAGE_FILE_MACHINE_IA64;
        frame.AddrPC.Offset = context.StIIP;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.IntSp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrBStore.Offset = context.RsBSP;
        frame.AddrBStore.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.IntSp;
        frame.AddrStack.Mode = AddrModeFlat;

#else

        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "--> STACK TRACE: [Unsupported architecture]\n");
        return;

#endif

        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "--> EXCEPTION STACK CONTEXT:\n");
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "    PC: ");
        SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, stackFrame.AddrPC.Offset);
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n    BP: ");
        SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, stackFrame.AddrFrame.Offset);
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n    SP: ");
        SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, stackFrame.AddrStack.Offset);
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n");

        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "--> STACK TRACE:\n");

        constexpr int iMaxFrames = 64;
        int iFrameNumber = 0;
        DWORD64 dwPrevReturnAddr = 0;

        for (int iCurrent = 0; iCurrent < iMaxFrames; ++iCurrent) {
            BOOL bResult = pfnStackWalk64(
                dwMachineType,
                hProcess,
                hThread,
                &stackFrame,
                &contextCopy,
                nullptr,
                CustomFunctionTableAccess64,
                CustomGetModuleBase64,
                nullptr
            );

            if (!bResult || stackFrame.AddrPC.Offset == 0) {
                break;
            }

            // Avoid infinite loops
            if (stackFrame.AddrReturn.Offset == dwPrevReturnAddr && dwPrevReturnAddr != 0) {
                break;
            }

            dwPrevReturnAddr = stackFrame.AddrReturn.Offset;

            // Conditionally subtract one from the PC to make sure location of the function is
            // reported, not address of the next statement.
            // https://stackoverflow.com/questions/22487887/why-doesnt-stack-walking-work-properly-when-using-setunhandledexceptionfilter
            AppendInlineFrames(
                sBuffer,
                szBufferSize,
                szBufferPosition,
                stackFrame.AddrPC.Offset - 
                    ((iCurrent > 0 && stackFrame.AddrPC.Offset != 0) ? 1 : 0),
                stackFrame.AddrReturn.Offset,
                hProcess,
                &iFrameNumber
            );

            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "    [");
            SafeAppendDecimalU32(sBuffer, szBufferSize, szBufferPosition, static_cast<std::uint32_t>(iFrameNumber++));
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "] ");
            SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, stackFrame.AddrPC.Offset);
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, " ");

            AppendSymbolInfo(
                sBuffer,
                szBufferSize,
                szBufferPosition,
                stackFrame.AddrPC.Offset,
                hProcess
            );

            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n");
        }
    }

    void AppendExceptionRecordInfo(
        char* sBuffer,
        std::size_t szBufferSize,
        std::size_t* szBufferPosition,
        const EXCEPTION_RECORD* pExceptionRecord
    ) {
        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "--> EXCEPTION CODE: ");
        SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, pExceptionRecord->ExceptionCode);

        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n--> EXCEPTION ADDRESS: ");
        SafeAppendPointerHex(
            sBuffer,
            szBufferSize,
            szBufferPosition,
            reinterpret_cast<std::uintptr_t>(pExceptionRecord->ExceptionAddress)
        );

        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n--> EXCEPTION FLAGS: ");
        SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, pExceptionRecord->ExceptionFlags);

        if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
            pExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
            constexpr DWORD ACCESS_VIOLATION_ARGUMENTS = 2;
            constexpr DWORD IN_PAGE_ERROR_ARGUMENTS = 3;

            if (pExceptionRecord->NumberParameters >= ACCESS_VIOLATION_ARGUMENTS) {
                constexpr ULONG_PTR READ_ACCESS_FLAG = 0;
                constexpr ULONG_PTR WRITE_ACCESS_FLAG = 1;
                constexpr ULONG_PTR EXECUTE_ACCESS_FLAG = 8;

                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n--> ACCESS TYPE: ");

                const char* sAccessTypeString;
                switch (pExceptionRecord->ExceptionInformation[0]) {
                case READ_ACCESS_FLAG:
                    sAccessTypeString = "READ";
                    break;

                case WRITE_ACCESS_FLAG:
                    sAccessTypeString = "WRITE";
                    break;

                case EXECUTE_ACCESS_FLAG:
                    sAccessTypeString = "EXECUTE (DEP VIOLATION)";
                    break;

                default:
                    sAccessTypeString = "UNKNOWN";
                    break;
                }

                SafeAppend(sBuffer, szBufferSize, szBufferPosition, sAccessTypeString);
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n--> ACCESS VIRTUAL ADDRESS: ");
                SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, pExceptionRecord->ExceptionInformation[1]);
            }

            if (pExceptionRecord->NumberParameters == IN_PAGE_ERROR_ARGUMENTS) {
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n--> NT STATUS: ");
                SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, pExceptionRecord->ExceptionInformation[2]);
            }
        }
        else if (pExceptionRecord->NumberParameters > 0) {
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n--> EXCEPTION PARAMETERS (");
            SafeAppendDecimalU32(sBuffer, szBufferSize, szBufferPosition, pExceptionRecord->NumberParameters);
            SafeAppend(sBuffer, szBufferSize, szBufferPosition, "):");

            for (DWORD dwIndex = 0; dwIndex < pExceptionRecord->NumberParameters; ++dwIndex) {
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n    [");
                SafeAppendDecimalU32(sBuffer, szBufferSize, szBufferPosition, dwIndex);
                SafeAppend(sBuffer, szBufferSize, szBufferPosition, "] ");
                SafeAppendPointerHex(sBuffer, szBufferSize, szBufferPosition, pExceptionRecord->ExceptionInformation[dwIndex]);
            }
        }

        SafeAppend(sBuffer, szBufferSize, szBufferPosition, "\n");
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    LONG WINAPI NotifyFatalUnhandledSEH(PEXCEPTION_POINTERS pExceptionInfo) {
        HANDLE hProcess = GetCurrentProcess();

        constexpr std::size_t szExceptionInfoBufferSize = 32768;
        char sExceptionInfoBuffer[szExceptionInfoBufferSize];
        std::size_t szExceptionInfoBufferPosition = 0;

        SecureZeroMemory(sExceptionInfoBuffer, sizeof(sExceptionInfoBuffer));

        constexpr std::size_t szLoadedSymbolsSize = 1024;
        DWORD64 pdwLoadedModulesBaseAddresses[szLoadedSymbolsSize];
        std::size_t szLoadedModulesBaseAddressesPosition = 0;

        SecureZeroMemory(pdwLoadedModulesBaseAddresses, sizeof(pdwLoadedModulesBaseAddresses));

        // Attempt to load symbols for all modules in the application.
        // This aids stack trace generation utilities.

        {
            BOOL result = LoadAllModuleSymbols(
                hProcess,
                sExceptionInfoBuffer,
                szExceptionInfoBufferSize,
                &szExceptionInfoBufferPosition,
                pdwLoadedModulesBaseAddresses,
                szLoadedSymbolsSize,
                &szLoadedModulesBaseAddressesPosition
            );

            if (!result) {
                SafeAppend(
                    sExceptionInfoBuffer,
                    szExceptionInfoBufferSize,
                    &szExceptionInfoBufferPosition,
                    "Not all symbols are available, expect [UNKNOWN] functions.\n"
                );
            }
        }

        SafeAppend(
            sExceptionInfoBuffer,
            szExceptionInfoBufferSize,
            &szExceptionInfoBufferPosition,
            "*** FATAL ERROR: UNHANDLED SEH EXCEPTION\n"
        );

        SafeAppend(
            sExceptionInfoBuffer,
            szExceptionInfoBufferSize,
            &szExceptionInfoBufferPosition,
            "--> SYSTEM THREAD: "
        );

        SafeAppendDecimalU32(
            sExceptionInfoBuffer,
            szExceptionInfoBufferSize,
            &szExceptionInfoBufferPosition,
            GetCurrentThreadId()
        );

        SafeAppend(
            sExceptionInfoBuffer,
            szExceptionInfoBufferSize,
            &szExceptionInfoBufferPosition,
            "\n"
        );

        // Iterate through the chain of exception records.
        constexpr int iMaxChainDepth = 16;
        int iCurrentChainIndex = 0;
        const EXCEPTION_RECORD* pCurrentRecord = pExceptionInfo->ExceptionRecord;

        while (pCurrentRecord && iCurrentChainIndex < iMaxChainDepth) {
            if (iCurrentChainIndex == 0) {
                SafeAppend(
                    sExceptionInfoBuffer,
                    szExceptionInfoBufferSize,
                    &szExceptionInfoBufferPosition,
                    "--> PRIMARY EXCEPTION RECORD:\n"
                );
            }
            else {
                SafeAppend(
                    sExceptionInfoBuffer,
                    szExceptionInfoBufferSize,
                    &szExceptionInfoBufferPosition,
                    "--> CHAINED EXCEPTION RECORD ["
                );

                SafeAppendDecimalU32(
                    sExceptionInfoBuffer,
                    szExceptionInfoBufferSize,
                    &szExceptionInfoBufferPosition,
                    static_cast<std::uint32_t>(iCurrentChainIndex)
                );

                SafeAppend(
                    sExceptionInfoBuffer,
                    szExceptionInfoBufferSize,
                    &szExceptionInfoBufferPosition,
                    "]:\n"
                );
            }

            AppendExceptionRecordInfo(
                sExceptionInfoBuffer,
                szExceptionInfoBufferSize,
                &szExceptionInfoBufferPosition,
                pCurrentRecord
            );

            pCurrentRecord = pCurrentRecord->ExceptionRecord;
            ++iCurrentChainIndex;
        }

        if (pCurrentRecord && iCurrentChainIndex >= iMaxChainDepth) {
            SafeAppend(
                sExceptionInfoBuffer,
                szExceptionInfoBufferSize,
                &szExceptionInfoBufferPosition,
                "--> WARNING: Exception chain exceeded maximum depth, truncated.\n"
            );
        }

        // Generate stack trace into the same buffer
        if (pExceptionInfo->ContextRecord) {
            GenerateStackTrace(
                sExceptionInfoBuffer,
                szExceptionInfoBufferSize,
                &szExceptionInfoBufferPosition,
                pExceptionInfo->ContextRecord
            );
        }

        // Done with crash dump generation, time unload symbols we loaded.
        // Notice that this may and leave some resources behind.

        {
            BOOL success = UnloadAllModuleSymbols(
                hProcess,
                pdwLoadedModulesBaseAddresses,
                szLoadedModulesBaseAddressesPosition
            );

            if (!success) {
                SafeAppend(
                    sExceptionInfoBuffer,
                    szExceptionInfoBufferSize,
                    &szExceptionInfoBufferPosition,
                    "Failed to unload some of the symbols!\n"
                );
            }
        }

        // Output everything at once
        WriteOutput(sExceptionInfoBuffer, szExceptionInfoBufferPosition);

        if (g_pPreviousFilter != nullptr) {
            return g_pPreviousFilter(pExceptionInfo);
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }
}

namespace startup_components::crash_handling {
    bool install_global_crash_handler() {
        g_pPreviousFilter = SetUnhandledExceptionFilter(NotifyFatalUnhandledSEH);
        if (g_pPreviousFilter == &NotifyFatalUnhandledSEH) {
            g_pPreviousFilter = nullptr;
        }

        return TRUE;
    }

    void remove_global_crash_handler() {
        SetUnhandledExceptionFilter(g_pPreviousFilter);
        g_pPreviousFilter = nullptr;
    }
}
