PUBLIC load_execution_thread
PUBLIC load_program
PUBLIC special_call_module
PUBLIC resume_program_execution

.data
.code

;loads the state of a thread that switched to a program. allows swiching between programs
load_execution_thread PROC
	mov rax, [rcx]
	mov rdx, [rcx + 16]
	mov r8, [rcx + 24]
	mov r9, [rcx + 32]
	mov r10, [rcx + 40]
	mov r11, [rcx + 48]
	mov r12, [rcx + 56]
	mov r13, [rcx + 64]
	mov r14, [rcx + 72]
	mov r15, [rcx + 80]
	mov rdi, [rcx + 88]
	mov rsi, [rcx + 96]
	mov rbx, [rcx + 104]
	mov rbp, [rcx + 112]
	mov rsp, [rcx + 120]

	push [rcx + 128]
	mov rcx, [rcx + 8]

	ret
load_execution_thread ENDP

;saves the state of this thread and loads the state of a program, third argument is used to determine whether this thread needs additional configuration for startup or not
load_program PROC
	;--------------------
	;save executor state
	
	mov [rcx], rax
	mov [rcx + 16], rdx 
	mov [rcx + 24], r8
	mov [rcx + 32], r9 
	mov [rcx + 40], r10 
	mov [rcx + 48], r11 
	mov [rcx + 56], r12 
	mov [rcx + 64], r13 
	mov [rcx + 72], r14 
	mov [rcx + 80], r15 
	mov [rcx + 88], rdi 
	mov [rcx + 96], rsi 
	mov [rcx + 104], rbx 
	mov [rcx + 112], rbp 

	;save return address
	mov rax, [rsp]
	mov [rcx + 128], rax
	
	mov rax, rsp
	add rax, 8
	
	mov [rcx + 120], rax
	mov [rcx + 8], rcx

	;--------------------
	;load program state
	
	mov r11, [rdx + 16]
	mov rcx, [rdx + 24]
	mov rbp, [rdx + 32]
	mov r9, [rdx + 40]
	mov r10, [rdx + 48]
	mov [rdx + 56], rsp

	;--------------------
	;check if thread requires startup
	cmp r8, 1
	jne program_start

	push [r10 + 24]

	program_start:
	jmp qword ptr [rdx + 8]
load_program ENDP

;saves the state of a program and calls a "call_module" function
special_call_module PROC
	;--------------------
	;save program state
	
	mov [rcx + 32], rbp 
	mov [rcx + 40], r9 

	mov r14, [rsp]
	mov [rcx + 8], r14

	mov rsp, [rcx + 56] ;make sure that stack is properly aligned
	;--------------------
	;call "call_module"

	mov rcx, rax
	mov rdx, r8
	mov r8, r15
	jmp qword ptr [r10 + 8]

special_call_module ENDP

;used to continue an execution of a program (after module has done its work)
resume_program_execution PROC
	mov r11, [rcx + 16]
	mov rcx, [rcx + 24]
	mov rbp, [rcx + 32]
	mov r9, [rcx + 40]
	mov r10, [rcx + 48]
	mov rsp, [rcx + 56]

	jmp qword ptr [rcx + 8]
resume_program_execution ENDP

END