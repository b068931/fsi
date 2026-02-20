#ifdef __clang__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"

#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>

#include "dbghelp_functions.h"

namespace startup_components::dbghelp {
    PFN_SymInitializeW pfnSymInitializeW = nullptr;
    PFN_SymCleanup pfnSymCleanup = nullptr;
    PFN_SymSetOptions pfnSymSetOptions = nullptr;
    PFN_SymGetOptions pfnSymGetOptions = nullptr;
    PFN_SymFromAddrW pfnSymFromAddrW = nullptr;
    PFN_SymGetLineFromAddrW64 pfnSymGetLineFromAddrW64 = nullptr;
    PFN_SymFunctionTableAccess64 pfnSymFunctionTableAccess64 = nullptr;
    PFN_SymGetModuleBase64 pfnSymGetModuleBase64 = nullptr;
    PFN_StackWalk2 pfnStackWalk2 = nullptr;
    PFN_SymAddrIncludeInlineTrace pfnSymAddrIncludeInlineTrace = nullptr;
    PFN_SymQueryInlineTrace pfnSymQueryInlineTrace = nullptr;
    PFN_SymFromInlineContextW pfnSymFromInlineContextW = nullptr;
    PFN_SymGetLineFromInlineContextW pfnSymGetLineFromInlineContextW = nullptr;
    PFN_SymRefreshModuleList pfnSymRefreshModuleList = nullptr;
    PFN_SymLoadModuleExW pfnSymLoadModuleExW = nullptr;
    PFN_SymUnloadModule64 pfnSymUnloadModule64 = nullptr;
    PFN_EnumerateLoadedModulesW64 pfnEnumerateLoadedModulesW64 = nullptr;

    BOOL g_bDbgHelpLoaded = false;
    BOOL g_bSymbolsInitialized = false;
    HANDLE g_hDbgHelpModule = nullptr;

    namespace {
        template<typename T>
        bool load_function(HMODULE hModule, const char* sFunctionName, T*& pfnOut) {
            pfnOut = reinterpret_cast<T*>(GetProcAddress(hModule, sFunctionName));
            return pfnOut != nullptr;
        }

        void clear_all_function_pointers() {
            pfnSymInitializeW = nullptr;
            pfnSymCleanup = nullptr;
            pfnSymSetOptions = nullptr;
            pfnSymGetOptions = nullptr;
            pfnSymFromAddrW = nullptr;
            pfnSymGetLineFromAddrW64 = nullptr;
            pfnSymFunctionTableAccess64 = nullptr;
            pfnSymGetModuleBase64 = nullptr;
            pfnStackWalk2 = nullptr;
            pfnSymAddrIncludeInlineTrace = nullptr;
            pfnSymQueryInlineTrace = nullptr;
            pfnSymFromInlineContextW = nullptr;
            pfnSymGetLineFromInlineContextW = nullptr;
            pfnSymRefreshModuleList = nullptr;
            pfnSymLoadModuleExW = nullptr;
            pfnSymUnloadModule64 = nullptr;
            pfnEnumerateLoadedModulesW64 = nullptr;
        }
    }

    bool load_dbghelp() {
        if (g_bDbgHelpLoaded) {
            return true;
        }

        // Try to load DbgHelp.dll from system directory first.
        HMODULE hDbgHelp = LoadLibraryExW(L"DbgHelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!hDbgHelp) {
            // Fall back to default search path.
            hDbgHelp = LoadLibraryW(L"DbgHelp.dll");
        }

        if (!hDbgHelp) {
            return false;
        }

        g_hDbgHelpModule = hDbgHelp;

        bool bAllLoaded = true;

        bAllLoaded &= load_function(hDbgHelp, "SymInitializeW", pfnSymInitializeW);
        bAllLoaded &= load_function(hDbgHelp, "SymCleanup", pfnSymCleanup);
        bAllLoaded &= load_function(hDbgHelp, "SymSetOptions", pfnSymSetOptions);
        bAllLoaded &= load_function(hDbgHelp, "SymGetOptions", pfnSymGetOptions);
        bAllLoaded &= load_function(hDbgHelp, "SymFromAddrW", pfnSymFromAddrW);
        bAllLoaded &= load_function(hDbgHelp, "SymGetLineFromAddrW64", pfnSymGetLineFromAddrW64);
        bAllLoaded &= load_function(hDbgHelp, "SymFunctionTableAccess64", pfnSymFunctionTableAccess64);
        bAllLoaded &= load_function(hDbgHelp, "SymGetModuleBase64", pfnSymGetModuleBase64);
        bAllLoaded &= load_function(hDbgHelp, "StackWalk2", pfnStackWalk2);
        bAllLoaded &= load_function(hDbgHelp, "SymRefreshModuleList", pfnSymRefreshModuleList);
        bAllLoaded &= load_function(hDbgHelp, "SymLoadModuleExW", pfnSymLoadModuleExW);
        bAllLoaded &= load_function(hDbgHelp, "SymUnloadModule64", pfnSymUnloadModule64);
        bAllLoaded &= load_function(hDbgHelp, "EnumerateLoadedModulesW64", pfnEnumerateLoadedModulesW64);

        // These are optional - available in newer versions of DbgHelp
        load_function(hDbgHelp, "SymAddrIncludeInlineTrace", pfnSymAddrIncludeInlineTrace);
        load_function(hDbgHelp, "SymQueryInlineTrace", pfnSymQueryInlineTrace);
        load_function(hDbgHelp, "SymFromInlineContextW", pfnSymFromInlineContextW);
        load_function(hDbgHelp, "SymGetLineFromInlineContextW", pfnSymGetLineFromInlineContextW);

        if (!bAllLoaded) {
            FreeLibrary(hDbgHelp);
            g_hDbgHelpModule = nullptr;

            clear_all_function_pointers();
            return false;
        }

        g_bDbgHelpLoaded = true;
        return true;
    }

    bool initialize_symbols(HANDLE hProcess) {
        if (!g_bDbgHelpLoaded || !pfnSymInitializeW || !pfnSymSetOptions || !pfnSymGetOptions || !pfnSymCleanup) {
            return false;
        }

        // Refresh module list if already initialized.
        if (g_bSymbolsInitialized) {
            if (pfnSymRefreshModuleList) {
                pfnSymRefreshModuleList(hProcess);
            }

            return true;
        }

        DWORD dwOptions = pfnSymGetOptions();
        dwOptions |= SYMOPT_LOAD_LINES;                          // Load line number information.
        dwOptions |= SYMOPT_UNDNAME;                             // Undecorate symbol names.
        dwOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;                // Don't show error dialogs.
        dwOptions &= ~static_cast<DWORD>(SYMOPT_DEFERRED_LOADS); // Do not defer symbol loading.
        dwOptions &= ~static_cast<DWORD>(SYMOPT_NO_PROMPTS);     // Allow prompts if needed.

        pfnSymSetOptions(dwOptions);

        // Pass nullptr for search path to use default (_NT_SYMBOL_PATH, etc.)
        // Pass FALSE for fInvadeProcess - we'll load modules manually.
        if (!pfnSymInitializeW(hProcess, nullptr, false)) {
            return false;
        }

        g_bSymbolsInitialized = true;
        return true;
    }

    void cleanup_symbols(HANDLE hProcess) {
        if (g_bSymbolsInitialized) {
            assert(pfnSymCleanup && "Symbols initialized without cleanup function.");
            if (pfnSymCleanup) {
                pfnSymCleanup(hProcess);
            }

            g_bSymbolsInitialized = false;
        }
    }

    void unload_dbghelp() {
        if (g_bSymbolsInitialized) {
            cleanup_symbols(GetCurrentProcess());
        }

        if (g_hDbgHelpModule) {
            assert(g_bDbgHelpLoaded && "Module loaded without flag set.");

            FreeLibrary(static_cast<HMODULE>(g_hDbgHelpModule));
            g_hDbgHelpModule = nullptr;
        }

        clear_all_function_pointers();
        g_bDbgHelpLoaded = false;
    }
}

#ifdef __clang__

#pragma clang diagnostic pop

#endif
