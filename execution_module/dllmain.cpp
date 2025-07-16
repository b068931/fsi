#include "pch.h"
#include "thread_local_structure.h"

namespace {
    DWORD tls_index;
}

extern thread_local_structure* get_thread_local_structure() {
    return std::launder(static_cast<thread_local_structure*>(TlsGetValue(tls_index)));
}

BOOL APIENTRY DllMain(HMODULE hModule,  // NOLINT(misc-use-internal-linkage)
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    LPVOID allocated_memory;
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            if ((tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) { //allocate tls index 
                return FALSE;
            }

            // no break: initialize the index for first thread.
            [[fallthrough]];

        case DLL_THREAD_ATTACH:
            allocated_memory = (LPVOID)LocalAlloc(LPTR, sizeof(thread_local_structure));
            if (allocated_memory != NULL) {

                /*
                * initialize thread_local_structure using default constructor
                * through placement new, considering the fact that we discard value returned by placement new,
                * later in the program our object can be accessed only through std::launder.
                */

                new((void*)allocated_memory) thread_local_structure{};
                TlsSetValue(tls_index, allocated_memory);
            }

            break;
        case DLL_THREAD_DETACH:
            allocated_memory = TlsGetValue(tls_index);
            if (allocated_memory != NULL) {
                delete[] get_thread_local_structure()->execution_thread_state;
                LocalFree((HLOCAL)allocated_memory);
            }

            break;
        case DLL_PROCESS_DETACH:
            allocated_memory = TlsGetValue(tls_index);
            if (allocated_memory != NULL) {
                delete[] get_thread_local_structure()->execution_thread_state;
                LocalFree((HLOCAL)allocated_memory);
            }

            TlsFree(tls_index);
            break;
        default:
            return FALSE; // unexpected reason for call
    }

    return TRUE;
}