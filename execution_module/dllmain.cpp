#include "pch.h"
#include "thread_local_structure.h"

namespace {
    DWORD tls_index;
}

namespace backend {
    // Add this forward declaration to avoid including "execution_backend_functions.h",
    // as that one contains a lot of other stuff that is not needed here and can't be used yet.
    extern thread_local_structure* get_thread_local_structure();

    thread_local_structure* get_thread_local_structure() {
        return std::launder(static_cast<thread_local_structure*>(TlsGetValue(tls_index)));
    }
}

BOOL APIENTRY DllMain(HMODULE,  // NOLINT(misc-use-internal-linkage)
                      DWORD ul_reason_for_call,
                      LPVOID)
{
    LPVOID allocated_memory;
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            if ((tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
                return FALSE;
            }

            // No break: initialize the index for first thread.
            [[fallthrough]];

        case DLL_THREAD_ATTACH:
            allocated_memory = 
                LocalAlloc(LPTR, sizeof(thread_local_structure));

            if (allocated_memory != nullptr) {

                /*
                * Initialize thread_local_structure using default constructor
                * through placement new, considering the fact that we discard value returned by placement new,
                * later in the program our object can be accessed only through std::launder.
                */

                new(allocated_memory) thread_local_structure{};
                TlsSetValue(tls_index, allocated_memory);
            }

            break;

        case DLL_THREAD_DETACH:
            allocated_memory = TlsGetValue(tls_index);
            if (allocated_memory != nullptr) {
                // Kinda sketchy, but it has trivial destructor, so its behavior is predictable.
                backend::get_thread_local_structure()->~thread_local_structure();
                LocalFree(allocated_memory);
            }

            break;

        case DLL_PROCESS_DETACH:
            allocated_memory = TlsGetValue(tls_index);
            if (allocated_memory != nullptr) {
                backend::get_thread_local_structure()->~thread_local_structure();
                LocalFree(allocated_memory);
            }

            TlsFree(tls_index);
            break;

        default:
            // Unexpected reason for call.
            return FALSE;
    }

    return TRUE;
}
