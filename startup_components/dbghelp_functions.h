#ifndef STARTUP_COMPONENTS_DBGHELP_FUNCTIONS_H
#define STARTUP_COMPONENTS_DBGHELP_FUNCTIONS_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <DbgHelp.h>

namespace startup_components::dbghelp {
    // Function pointer types for DbgHelp functions.
    using PFN_SymInitializeW = 
        BOOL (WINAPI*) (HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);

    using PFN_SymCleanup = 
        BOOL (WINAPI*) (HANDLE hProcess);

    using PFN_SymSetOptions = 
        DWORD (WINAPI*) (DWORD SymOptions);

    using PFN_SymGetOptions = 
        DWORD (WINAPI*) ();

    using PFN_SymFromAddrW = 
        BOOL (WINAPI*) (HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFOW Symbol);

    using PFN_SymGetLineFromAddrW64 = 
        BOOL (WINAPI*) (HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line);

    using PFN_SymFunctionTableAccess64 = 
        PVOID (WINAPI*) (HANDLE hProcess, DWORD64 AddrBase);

    using PFN_SymGetModuleBase64 = 
        DWORD64 (WINAPI*) (HANDLE hProcess, DWORD64 dwAddr);

    using PFN_StackWalk2 = BOOL (WINAPI*) (
        DWORD MachineType,
        HANDLE hProcess,
        HANDLE hThread,
        LPSTACKFRAME_EX StackFrame,
        PVOID ContextRecord,
        PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
        PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
        PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
        PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress,
        DWORD Flags
    );

    using PFN_SymAddrIncludeInlineTrace = DWORD (WINAPI*) (
        HANDLE hProcess,
        DWORD64 Address
    );

    using PFN_SymQueryInlineTrace = BOOL (WINAPI*) (
        HANDLE hProcess,
        DWORD64 StartAddress,
        DWORD StartContext,
        DWORD64 StartRetAddress,
        DWORD64 CurAddress,
        LPDWORD CurContext,
        LPDWORD CurFrameIndex
    );

    using PFN_SymFromInlineContextW = BOOL (WINAPI*) (
        HANDLE hProcess,
        DWORD64 Address,
        ULONG InlineContext,
        PDWORD64 Displacement,
        PSYMBOL_INFOW Symbol
    );

    using PFN_SymGetLineFromInlineContextW = BOOL (WINAPI*) (
        HANDLE hProcess,
        DWORD64 dwAddr,
        ULONG InlineContext,
        DWORD64 qwModuleBaseAddress,
        PDWORD pdwDisplacement,
        PIMAGEHLP_LINEW64 Line
    );

    using PFN_SymRefreshModuleList = 
        BOOL (WINAPI*) (HANDLE hProcess);

    using PFN_SymLoadModuleExW = DWORD64 (WINAPI*) (
        HANDLE hProcess,
        HANDLE hFile,
        PCWSTR ImageName,
        PCWSTR ModuleName,
        DWORD64 BaseOfDll,
        DWORD DllSize,
        PMODLOAD_DATA Data,
        DWORD Flags
    );

    using PFN_SymUnloadModule64 = 
        BOOL (WINAPI*) (HANDLE hProcess, DWORD64 BaseOfDll);

    using PFN_EnumerateLoadedModulesW64 = BOOL (WINAPI*) (
        HANDLE hProcess,
        PENUMLOADED_MODULES_CALLBACKW64 EnumLoadedModulesCallback,
        PVOID UserContext
    );

    // Extern function pointers - populated by configure_dbghelp.cpp
    extern PFN_SymInitializeW pfnSymInitializeW;
    extern PFN_SymCleanup pfnSymCleanup;
    extern PFN_SymSetOptions pfnSymSetOptions;
    extern PFN_SymGetOptions pfnSymGetOptions;
    extern PFN_SymFromAddrW pfnSymFromAddrW;
    extern PFN_SymGetLineFromAddrW64 pfnSymGetLineFromAddrW64;
    extern PFN_SymFunctionTableAccess64 pfnSymFunctionTableAccess64;
    extern PFN_SymGetModuleBase64 pfnSymGetModuleBase64;
    extern PFN_StackWalk2 pfnStackWalk2;
    extern PFN_SymAddrIncludeInlineTrace pfnSymAddrIncludeInlineTrace;
    extern PFN_SymQueryInlineTrace pfnSymQueryInlineTrace;
    extern PFN_SymFromInlineContextW pfnSymFromInlineContextW;
    extern PFN_SymGetLineFromInlineContextW pfnSymGetLineFromInlineContextW;
    extern PFN_SymRefreshModuleList pfnSymRefreshModuleList;
    extern PFN_SymLoadModuleExW pfnSymLoadModuleExW;
    extern PFN_SymUnloadModule64 pfnSymUnloadModule64;
    extern PFN_EnumerateLoadedModulesW64 pfnEnumerateLoadedModulesW64;

    // Status of DbgHelp initialization
    extern BOOL g_bDbgHelpLoaded;
    extern BOOL g_bSymbolsInitialized;
    extern HANDLE g_hDbgHelpModule;

    // Functions to load and configure DbgHelp
    bool load_dbghelp();
    bool initialize_symbols(HANDLE hProcess);
    void cleanup_symbols(HANDLE hProcess);
    void unload_dbghelp();
}

#endif // STARTUP_COMPONENTS_DBGHELP_FUNCTIONS_H
