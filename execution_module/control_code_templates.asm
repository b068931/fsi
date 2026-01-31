; Refer to https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64?view=msvc-170#chained-unwind-info-structures,
;          https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170.
; The implementation relies on rather tricky details of Windows' x64 calling convention and exception handling to facilitate 
; context switching between executor thread and dynamically generated functions (user programs).
; This can also be considered a manual implementation of the setjmp/longjmp functionality.
; The difference is that this implementation specifically accomodates the needs of dynamically generated functions.

PUBLIC CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD
PUBLIC CONTROL_CODE_TEMPLATE_LOAD_PROGRAM
PUBLIC CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE
PUBLIC CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION
PUBLIC CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE

PUBLIC CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_SIZE
PUBLIC CONTROL_CODE_TEMPLATE_LOAD_PROGRAM_SIZE
PUBLIC CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE_SIZE
PUBLIC CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_SIZE
PUBLIC CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE_SIZE

PUBLIC CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT 
PUBLIC CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT 

USER_PROGRAM_COMPARISON_STATE_DISPLACEMENT              EQU 0
USER_PROGRAM_RETURN_ADDRESS_DISPLACEMENT                EQU 8
USER_PROGRAM_JUMP_TABLE_DISPLACEMENT                    EQU 16
USER_PROGRAM_MY_STATE_ADDRESS_DISPLACEMENT              EQU 24
USER_PROGRAM_CURRENT_STACK_POSITION_DISPLACEMENT        EQU 32
USER_PROGRAM_STACK_END_DISPLACEMENT                     EQU 40
USER_PROGRAM_CONTROL_FUNCTIONS_DISPLACEMENT             EQU 48
USER_PROGRAM_EXECUTOR_FRAME_STACK_TOP_DISPLACEMENT      EQU 56
USER_PROGRAM_EXECUTOR_FRAME_REGISTER_VALUE_DISPLACEMENT EQU 64

FRAME_POINTER_DISPLACEMENT EQU 0h
FRAME_POINTER_REGISTER     EQU r13
FRAME_SHADOW_SPACE_SIZE    EQU 40

CONTROL_FUNCTIONS_RUNTIME_MODULE_CALL_TRAMPOLINE_DISPLACEMENT EQU 0
CONTROL_FUNCTIONS_RUNTIME_MODULE_CALL_DISPLACEMENT            EQU 8
CONTROL_FUNCTIONS_PROGRAM_END_TRAMPOLINE_DISPLACEMENT         EQU 16
CONTROL_FUNCTIONS_PROGRAM_TERMINATION_DISPLACEMENT            EQU 24

EXECUTOR_THREAD_STATE_STACK_TOP_DISPLACEMENT            EQU 0
EXECUTOR_THREAD_STATE_FRAME_REGISTER_DISPLACEMENT EQU 8

; Must be aligned on 4-byte boundary for UNWIND_INFO.
xdata SEGMENT DWORD READ ALIAS(".xdata")
SHARED_CHAINED_UNWIND_INFO DB 21h ; Version 1, UNW_FLAG_CHAININFO.
                           DB 00h ; Size of prolog.
                           DB 00h ; Count of unwind codes.
                           DB 00h ; Frame register and offset. Documentation says that this, along with fixed stack allocation, 
                                  ; must match primary unwind info's frame register and offset. They, however, didn't mention why.
                                  ; They also didn't mention how that information is going to be used. So leave it as zero.
                           DD (IMAGEREL CONTROL_CODE_TEMPLATE_LOAD_PROGRAM) ; Now point to LOAD_PROGRAM's RUNTIME_FUNCTION.
                           DD (IMAGEREL LOAD_PROGRAM_ENDP)

                           ; Worth noting that "$xdayasym" is an undocumented feature, I found it by inspecting compiler-generated unwind info.
                           DD (IMAGEREL $xdatasym)                          ; Pointer to the beginning of xdata segment for a different function table.
xdata ENDS                                                                  ; There lies the unwind info for LOAD_PROGRAM.

; Must be aligned on 4-byte boundary for RUNTIME_FUNCTION.
; This provides the information as to where the unwind information for each function is located.
; This is critical for SEH and debugger support (stack walking).
pdata SEGMENT DWORD READ ALIAS(".pdata")
    ; Trampoline functions are fully covered by unwind info which is chained to LOAD_PROGRAM's unwind info.
    DD (IMAGEREL CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE)
    DD (IMAGEREL CALL_MODULE_TRAMPOLINE_ENDP)
    DD (IMAGEREL SHARED_CHAINED_UNWIND_INFO)
    
    DD (IMAGEREL CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE)
    DD (IMAGEREL PROGRAM_END_TRAMPOLINE_ENDP)
    DD (IMAGEREL SHARED_CHAINED_UNWIND_INFO)

    ; While functions that act like context switch points have RUNTIME_FUNCTION set up
    ; in the middle of their code. These functions may look kind of trippy because it is as if
    ; they can remove stack frames in the middle of the stack.
    DD (IMAGEREL CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT)
    DD (IMAGEREL RESUME_PROGRAM_EXECUTION_ENDP)
    DD (IMAGEREL SHARED_CHAINED_UNWIND_INFO)

    DD (IMAGEREL CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT)
    DD (IMAGEREL LOAD_EXECUTION_THREAD_ENDP)
    DD (IMAGEREL SHARED_CHAINED_UNWIND_INFO)
pdata ENDS

.data

CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_SIZE      DQ (LOAD_EXECUTION_THREAD_ENDP - CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD)
CONTROL_CODE_TEMPLATE_LOAD_PROGRAM_SIZE               DQ (LOAD_PROGRAM_ENDP - CONTROL_CODE_TEMPLATE_LOAD_PROGRAM)
CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE_SIZE     DQ (CALL_MODULE_TRAMPOLINE_ENDP - CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE)
CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_SIZE   DQ (RESUME_PROGRAM_EXECUTION_ENDP - CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION)
CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE_SIZE     DQ (PROGRAM_END_TRAMPOLINE_ENDP - CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE)

.code

; Loads the initial state of a thread that switched to a user program. This one is also
; marked as [noreturn] in C++. The same restriction of using only trivially destructible objects,
; as with "RESUME_PROGRAM_EXECUTION", applies here as well. 
; Here we essentially just unwind the stack to the frame that was set up by "LOAD_PROGRAM".
; As both "LOAD_EXECUTION_THREAD" and "LOAD_PROGRAM" can only be used from the same thread, 
; from the point of view of the "LOAD_PROGRAM" function caller this will look like a normal function return.
CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD PROC
    ; First restore the frame pointer to prepare for the context switch.
    ; Note that the order of restoring registers here is important. RSP can be modified
    ; arbitrarily if we have a frame pointer. This behavior is documented and required by
    ; some of the functions which allocate memory on stack in dynamic fashion.
    mov FRAME_POINTER_REGISTER, [rcx + EXECUTOR_THREAD_STATE_FRAME_REGISTER_DISPLACEMENT]

    ; Set the point where the context switch happens. From this point onward,
    ; SEH and debugger will see our RUNTIME_FUNCTION with stack frame set up by "LOAD_PROGRAM".
    CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD_CONTEXT_SWITCH_POINT LABEL PTR

    ; Unwind the stack back to the frame set up by the "LOAD_PROGRAM" function.
    ; Remove shadow space and possibly all other allocated memory.
    lea rsp, [FRAME_POINTER_REGISTER + FRAME_SHADOW_SPACE_SIZE]

    ; Start stack unwinding.
    ; Restore all non-volatile registers from stack.
    pop r15
    pop r14
    pop r12
    pop rsi
    pop rdi
    pop rbp
    pop rbx

    ; Notice that FRAME_POINTER_REGISTER is restored last.
    ; This specific order seems to be important. Yet, documentation doesn't mention it.
    pop r13

    ; Finally, continue execution of the function which has called "LOAD_PROGRAM".
    ret
CONTROL_CODE_TEMPLATE_LOAD_EXECUTION_THREAD ENDP
LOAD_EXECUTION_THREAD_ENDP LABEL PTR

; Saves the state of the executor thread and loads the state of a program, 
; third argument is used to determine whether this thread needs additional 
; configuration for startup or not.
; Dynamically generated functions have their UNWIND_INFO copied from this function's
; UNWIND_INFO data. We can't use chaining because dynamic functions may reside in arbitrary
; memory locations. That is, we are not guaranteed that their location lies within 2GB of this function.
; This is an (ab)use of the chaining feature of unwind info.
; This allows us to have proper stack unwinding for SEH and proper stack debugging support.
CONTROL_CODE_TEMPLATE_LOAD_PROGRAM PROC FRAME
    ; Save all non-volatile registers on stack, then set up a frame.
    ; This frame will be used by the debugger and exception handling.
    ; Setting up a frame will also allow us to arbitrarily change RSP,
    ; as long as we properly set up dynamic functions to also specify 
    ; that they use FRAME_POINTER_REGISTER as a frame pointer.

    ; Stack aligned on 16-byte boundary here.
    ; Notice that FRAME_POINTER_REGISTER is r13. We must push
    ; it first before setting up the frame. I don't think that this behaviour is documented.
    ; I inferred it by observing debugger and SEH behavior.
    ; In fact, if you push this register later, Visual Studio's debugger will try
    ; to skip the entire epilogue in "LOAD_EXECUTION_THREAD" function, leading to strange behavior.
    ; The debugger will behave as if you pressed "Continue" button.
    push r13
    .pushreg r13

    ; Stack is misaligned here.
    push rbx
    .pushreg rbx

    ; Stack aligned on 16-byte boundary here.
    push rbp
    .pushreg rbp

    ; Stack is misaligned here.
    push rdi
    .pushreg rdi

    ; Stack aligned on 16-byte boundary here.
    push rsi
    .pushreg rsi

    ; Stack is misaligned here.
    push r12
    .pushreg r12

    ; Stack aligned on 16-byte boundary here.
    push r14
    .pushreg r14

    ; Stack is misaligned here.
    push r15
    .pushreg r15

    ; Stack is misaligned, so use 40 bytes for shadow space.
    sub rsp,    FRAME_SHADOW_SPACE_SIZE
    .allocstack FRAME_SHADOW_SPACE_SIZE

    ; Use FRAME_POINTER_REGISTER as frame pointer. We must ensure to never modify it.
    ; Otherwise, SEH and the debugger stack view will break.
    mov       FRAME_POINTER_REGISTER, rsp
    .setframe FRAME_POINTER_REGISTER, FRAME_POINTER_DISPLACEMENT
    .endprolog

    ; Save stack pointer and fram pointer to executor state memory.
    ; Saving both of them enables us to switch stack frames mid-procedures.
    ; Look for *_CONTEXT_SWITCH_POINT labels to see it in action.
    mov [rcx + EXECUTOR_THREAD_STATE_STACK_TOP_DISPLACEMENT],      rsp
    mov [rcx + EXECUTOR_THREAD_STATE_FRAME_REGISTER_DISPLACEMENT], FRAME_POINTER_REGISTER
    
    ; Now we load the state of the dynamic function to be executed.
    ; We additionally save RSP to use it when resuming the program execution,
    ; that is to guard against the possiblity that compiler won't use "CALL" for
    ; [noreturn] functions. I am not sure whether standard even specifies this behavior.
    ; We also save FRAME_POINTER_REGISTER, as it is used as frame pointer, and we must restore it when resuming the program.
    mov r11, [rdx + USER_PROGRAM_JUMP_TABLE_DISPLACEMENT]
    mov rcx, [rdx + USER_PROGRAM_MY_STATE_ADDRESS_DISPLACEMENT]
    mov rbp, [rdx + USER_PROGRAM_CURRENT_STACK_POSITION_DISPLACEMENT]
    mov r9,  [rdx + USER_PROGRAM_STACK_END_DISPLACEMENT]
    mov r10, [rdx + USER_PROGRAM_CONTROL_FUNCTIONS_DISPLACEMENT]
    mov [rdx + USER_PROGRAM_EXECUTOR_FRAME_STACK_TOP_DISPLACEMENT],      rsp
    mov [rdx + USER_PROGRAM_EXECUTOR_FRAME_REGISTER_VALUE_DISPLACEMENT], FRAME_POINTER_REGISTER

    ; TEST only sets the RFLAGS. We use it to check whether third argument is zero or not.
    ; If it is zero, FALSE was passed, meaning no additional setup is required.
    test r8, r8
    jz skip_end_trap_setup

    ; Use trampoline function to ensure proper stack alignment on call: RSP % 16 == 8.
    ; Program loader already generates code that will use "call" instruction.
    push qword ptr [r10 + CONTROL_FUNCTIONS_PROGRAM_END_TRAMPOLINE_DISPLACEMENT]

    skip_end_trap_setup:
    jmp qword ptr [rdx + USER_PROGRAM_RETURN_ADDRESS_DISPLACEMENT]
CONTROL_CODE_TEMPLATE_LOAD_PROGRAM ENDP
LOAD_PROGRAM_ENDP LABEL PTR

; Used to call into a module from a dynamically generated function.
; Treat this as an adapter which saves necessary state for the dynamically generated function,
; then calls into the module's "call_module" trap (defined in C++).
CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE PROC
    ; Save program state. Save return address without popping it from stack.
    ; This ensures rsp % 16 == 8, which C++ code expects on function entry.
    mov r14, [rsp]
    mov [rcx + USER_PROGRAM_RETURN_ADDRESS_DISPLACEMENT],         r14

    mov [rcx + USER_PROGRAM_CURRENT_STACK_POSITION_DISPLACEMENT], rbp 
    mov [rcx + USER_PROGRAM_STACK_END_DISPLACEMENT],              r9 

    ; Call into "call_module" with x64 calling convention.
    ; Shadow space was already set up by the "LOAD_PROGRAM" function.
    mov rcx, rax
    mov rdx, r8
    mov r8, r15

    jmp qword ptr [r10 + CONTROL_FUNCTIONS_RUNTIME_MODULE_CALL_DISPLACEMENT]
CONTROL_CODE_TEMPLATE_CALL_MODULE_TRAMPOLINE ENDP
CALL_MODULE_TRAMPOLINE_ENDP LABEL PTR

; Used to continue an execution of a program after a module has done its work.
; This function is marked as [noreturn] in C++. It must not be called from functions
; which rely on destructor side effects, as those will not be executed. That is,
; this function must only be called from functions which have only trivially 
; destructible objects.
; Unwinds the stack and sets up the frame for a dynamic function to continue.
CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION PROC
    ; Restore FRAME_POINTER_REGISTER, as it is used as a frame pointer. Dynamically generated
    ; functions' chained UNWIND_INFO have identical unwind info to "LOAD_PROGRAM"'s RUNTIME_FUNCTION,
    ; and that uses FRAME_POINTER_REGISTER as frame pointer.
    mov FRAME_POINTER_REGISTER, [rcx + USER_PROGRAM_EXECUTOR_FRAME_REGISTER_VALUE_DISPLACEMENT]

    ; In order to maintain the continuity of stack frames for the debugger and SEH,
    ; the code after this label up to the end of the function will be put as a RUNTIME_FUNCTION,
    ; which has its unwind info chained to "LOAD_PROGRAM"'s RUNTIME_FUNCTION.
    CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION_CONTEXT_SWITCH_POINT LABEL PTR

    ; In C++ this function is marked as [noreturn], I have no idea 
    ; whether compiler is still required to use calling convention properly.
    mov rsp, [rcx + USER_PROGRAM_EXECUTOR_FRAME_STACK_TOP_DISPLACEMENT] 

    ; Now restore the rest of the program state.
    mov r11, [rcx + USER_PROGRAM_JUMP_TABLE_DISPLACEMENT]
    mov rcx, [rcx + USER_PROGRAM_MY_STATE_ADDRESS_DISPLACEMENT]
    mov rbp, [rcx + USER_PROGRAM_CURRENT_STACK_POSITION_DISPLACEMENT]
    mov r9,  [rcx + USER_PROGRAM_STACK_END_DISPLACEMENT]
    mov r10, [rcx + USER_PROGRAM_CONTROL_FUNCTIONS_DISPLACEMENT]

    jmp qword ptr [rcx + USER_PROGRAM_RETURN_ADDRESS_DISPLACEMENT]
CONTROL_CODE_TEMPLATE_RESUME_PROGRAM_EXECUTION ENDP
RESUME_PROGRAM_EXECUTION_ENDP LABEL PTR

; This trampoline, just like the runtime module call trampoline, ensures that
; the stack is set up properly before entering C++ code (trap function).
CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE PROC
    ; Set error code value to zero.
    xor rcx, rcx

    ; In our case, no specific set up is required. Just call the trap function.
    ; Shadow space was already set up by the "LOAD_PROGRAM" function.
    call qword ptr [r10 + CONTROL_FUNCTIONS_PROGRAM_TERMINATION_DISPLACEMENT]

    ; Called trap function must not return control flow to the trampoline.
    int 3
CONTROL_CODE_TEMPLATE_PROGRAM_END_TRAMPOLINE ENDP
PROGRAM_END_TRAMPOLINE_ENDP LABEL PTR

END