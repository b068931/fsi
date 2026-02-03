#include "pch.h"
#include "module_interoperation.h"
#include "execution_module.h"
#include "thread_manager.h"
#include "control_code_templates.h"
#include "execution_backend_functions.h"
#include "runtime_traps.h"
#include "unwind_info.h"

#include "../startup_components/local_crash_handlers.h"

namespace {
    module_mediator::module_part* part = nullptr;
    thread_manager* manager = nullptr;
    char* runtime_trap_table = nullptr;

    std::optional<std::string> verify_control_code_pdata_xdata_setup() {
        DWORD64 dw64LoadProgramBase = 0;
        PRUNTIME_FUNCTION prfLoadProgram = RtlLookupFunctionEntry(
            std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_LOAD_PROGRAM),
            &dw64LoadProgramBase,
            nullptr
        );

        if (prfLoadProgram == nullptr) {
            return "Failed to lookup function entry for CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.";
        }

        if (prfLoadProgram->EndAddress - prfLoadProgram->BeginAddress != 
            CONTROL_CODE_TEMPLATE_LOAD_PROGRAM_SIZE) {
            return "Incorrect size recorded for CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.";
        }

        UNWIND_INFO_HEADER* loadProgramUnwindInfo = std::bit_cast<UNWIND_INFO_HEADER*>(
            dw64LoadProgramBase + prfLoadProgram->UnwindData);

        if (loadProgramUnwindInfo->SizeOfProlog == 0 ||
            loadProgramUnwindInfo->CountOfCodes == 0 ||
            loadProgramUnwindInfo->FrameRegister == 0) {
            return "CONTROL_CODE_TEMPLATE_LOAD_PROGRAM does not have stack frame set up.";
        }

        if (loadProgramUnwindInfo->Flags != UNW_FLAG_NHANDLER) {
            return "CONTROL_CODE_TEMPLATE_LOAD_PROGRAM has incorrect unwind info flags.";
        }

        DWORD64 dw64CallModuleTrampolineBase = 0;
        PRUNTIME_FUNCTION prfCallModuleTrampoline = RtlLookupFunctionEntry(
            std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE),
            &dw64CallModuleTrampolineBase,
            nullptr
        );

        if (prfCallModuleTrampoline == nullptr) {
            return "Failed to lookup function entry for CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE.";
        }

        DWORD64 dw64ProgramEndTrampolineBase = 0;
        PRUNTIME_FUNCTION prfProgramEndTrampoline = RtlLookupFunctionEntry(
            std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE),
            &dw64ProgramEndTrampolineBase,
            nullptr
        );

        if (prfProgramEndTrampoline == nullptr) {
            return "Failed to lookup function entry for CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE.";
        }

        DWORD64 dw64LoadExecutionThreadContextSwitchPointBase = 0;
        PRUNTIME_FUNCTION prfLoadExecutionThreadContextSwitchPoint = RtlLookupFunctionEntry(
            std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT),
            &dw64LoadExecutionThreadContextSwitchPointBase,
            nullptr
        );

        if (prfLoadExecutionThreadContextSwitchPoint == nullptr) {
            return "Failed to lookup function entry for CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT.";
        }

        DWORD64 dw64ResumeProgramExecutionContextSwitchPointBase = 0;
        PRUNTIME_FUNCTION prfResumeProgramExecutionContextSwitchPoint = RtlLookupFunctionEntry(
            std::bit_cast<DWORD64>(&CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT),
            &dw64ResumeProgramExecutionContextSwitchPointBase,
            nullptr
        );

        if (prfResumeProgramExecutionContextSwitchPoint == nullptr) {
            return "Failed to lookup function entry for CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT.";
        }
        if (dw64LoadProgramBase != dw64CallModuleTrampolineBase ||
            dw64LoadProgramBase != dw64ProgramEndTrampolineBase ||
            dw64LoadProgramBase != dw64LoadExecutionThreadContextSwitchPointBase ||
            dw64LoadProgramBase != dw64ResumeProgramExecutionContextSwitchPointBase) {
            return "CONTROL_CODE_TEMPLATE functions or context switch points are registered in modules.";
        }

        if (prfCallModuleTrampoline->EndAddress - prfCallModuleTrampoline->BeginAddress != 
            CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE_SIZE) {
            return "Incorrect size recorded for CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE.";
        }

        if (prfProgramEndTrampoline->EndAddress - prfProgramEndTrampoline->BeginAddress != 
            CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE_SIZE) {
            return "Incorrect size recorded for CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE.";
        }
        
        CHAINED_UNWIND_INFO* callModuleTrampolineChainedInfo = std::bit_cast<CHAINED_UNWIND_INFO*>(
            dw64CallModuleTrampolineBase + prfCallModuleTrampoline->UnwindData);

        CHAINED_UNWIND_INFO* programEndTrampolineChainedInfo = std::bit_cast<CHAINED_UNWIND_INFO*>(
            dw64ProgramEndTrampolineBase + prfProgramEndTrampoline->UnwindData);

        CHAINED_UNWIND_INFO* loadExecutionThreadContextSwitchPointChainedInfo = std::bit_cast<CHAINED_UNWIND_INFO*>(
            dw64LoadExecutionThreadContextSwitchPointBase + prfLoadExecutionThreadContextSwitchPoint->UnwindData);

        CHAINED_UNWIND_INFO* resumeProgramExecutionContextSwitchPointChainedInfo = std::bit_cast<CHAINED_UNWIND_INFO*>(
            dw64ResumeProgramExecutionContextSwitchPointBase + 
            prfResumeProgramExecutionContextSwitchPoint->UnwindData);

        auto compare_runtime_functions = [](const RUNTIME_FUNCTION& a, const RUNTIME_FUNCTION& b) {
            return a.BeginAddress == b.BeginAddress &&
                   a.EndAddress == b.EndAddress &&
                   a.UnwindData == b.UnwindData;
            };

        if (!compare_runtime_functions(*prfLoadProgram, callModuleTrampolineChainedInfo->ChainedFunction)) {
            return "CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE unwind info is not chained to "
                   "CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.";
        }

        if (!compare_runtime_functions(*prfLoadProgram, programEndTrampolineChainedInfo->ChainedFunction)) {
            return "CONTROL_CODE_TEMPLATE_CALL_PROGRAM_END_TRAMPOLINE unwind info is not chained to "
                   "CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.";
        }

        if (!compare_runtime_functions(*prfLoadProgram, loadExecutionThreadContextSwitchPointChainedInfo->ChainedFunction)) {
            return "CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT unwind info is not chained to "
                   "CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.";
        }

        if (!compare_runtime_functions(*prfLoadProgram, resumeProgramExecutionContextSwitchPointChainedInfo->ChainedFunction)) {
            return "CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT unwind info is not chained to "
                   "CONTROL_CODE_TEMPLATE_LOAD_PROGRAM.";
        }

        if (callModuleTrampolineChainedInfo->Header.Flags != UNW_FLAG_CHAININFO ||
            programEndTrampolineChainedInfo->Header.Flags != UNW_FLAG_CHAININFO) {
            return "Chained unwind info for CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE or "
                   "CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE does not have CHAININFO flag set.";
        }

        if (loadExecutionThreadContextSwitchPointChainedInfo->Header.Flags != UNW_FLAG_CHAININFO ||
            resumeProgramExecutionContextSwitchPointChainedInfo->Header.Flags != UNW_FLAG_CHAININFO) {
            return "Chained unwind info for CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT or "
                "CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT does not have CHAININFO flag set.";
        }

        return {};
    }
}

namespace interoperation {
    module_mediator::module_part* get_module_part() {
        return part;
    }
}

namespace backend {
    thread_manager& get_thread_manager() {
        return *manager;
    }

    char* get_runtime_trap_table() {
        return runtime_trap_table;
    }
}

void initialize_m(module_mediator::module_part* module_part) {
    constexpr std::uint64_t runtime_trap_table_size = 4;
    constexpr std::uint64_t runtime_module_call_trampoline_index = 0;
    constexpr std::uint64_t runtime_module_call_trap_index = 1;
    constexpr std::uint64_t program_termination_trampoline_index = 2;
    constexpr std::uint64_t program_termination_trap_index = 3;

    part = module_part;
    manager = new thread_manager{};

    // Allocate and initialize runtime trap table. I intentionally don't use linking between
    // this object file and the assembly one to allow for the machine code in that file to 
    // reside in a different part of process memory.
    runtime_trap_table = new char[runtime_trap_table_size * sizeof(std::uint64_t)] {};

    backend::fill_in_register_array_entry(
        runtime_module_call_trampoline_index,
        runtime_trap_table,
        reinterpret_cast<std::uintptr_t>(&CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE)
    );

    backend::fill_in_register_array_entry(
        runtime_module_call_trap_index,
        runtime_trap_table,
        reinterpret_cast<std::uintptr_t>(&runtime_traps::runtime_module_call)
    );

    backend::fill_in_register_array_entry(
        program_termination_trampoline_index,
        runtime_trap_table,
        reinterpret_cast<std::uintptr_t>(&CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE)
    );

    backend::fill_in_register_array_entry(
        program_termination_trap_index,
        runtime_trap_table,
        reinterpret_cast<std::uintptr_t>(&runtime_traps::program_termination_request)
    );

    auto runtime_verification_result = verify_control_code_pdata_xdata_setup();
    if (runtime_verification_result.has_value()) {
        std::cerr << std::format(
            "*** RUNTIME VERIFICATION FAILED: {}\n",
            runtime_verification_result.value());

        // There isn't much we can do if this fails.
        ENVIRONMENT_REQUEST_TERMINATION();
    }

    logger_module::global_logging_instance::set_logging_enabled(true);
}

void free_m() {
    logger_module::global_logging_instance::set_logging_enabled(false);

    delete[] runtime_trap_table;
    delete manager;
}
