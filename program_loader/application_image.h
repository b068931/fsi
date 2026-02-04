#ifndef PROGRAM_LOADER_APPLICATION_IMAGE_H
#define PROGRAM_LOADER_APPLICATION_IMAGE_H

#include "pch.h"

#include "../execution_module/unwind_info.h"

struct application_image {
    void* image_base;
    std::uint64_t image_size;

    void** function_addresses;
    PRUNTIME_FUNCTION runtime_functions;
    UNWIND_INFO_DISPATCHER_PROLOGUE* unwind_info;
};

#endif
