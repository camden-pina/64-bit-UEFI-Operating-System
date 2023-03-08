.section .text

.global _start
_start:
	# Set up end of the stack frame linked list.
	xorl %ebp, %ebp
	pushq %rbp # rip=0
	pushq %rbp # rbp=0
	movq %rsp, %rbp

	movq %rcx, environ # envp

	# Initialize the standard library.
	subq $8, %rsp
	pushq %rsi
	pushq %rdi
	pushq %rcx
	call initialize_standard_library

	# Run the global constructors.
	call _init

	# Run main
	popq %rdx # Note! envp is now %rdx (previously %rcx)
	popq %rdi
	popq %rsi
	addq $8, %rsp
	call main

	# Terminate the process with main's exit code.
	movl %eax, %edi
	call exit

.size _start, . - _start

