# system call linkage
.text 

# kernal to user level linkages for syscall
.global syscall_linkage

#system call linkage
syscall_linkage:
	# save caller save reg
	pushfl
	pushl %esi
	pushl %edi
	pushl %ebp
	
	# check whether index is valid
	decl %eax
	cmpl $0, %eax
	jl INVALID
	cmpl $9, %eax
	jg INVALID

	# push arguments of the syscall functions
	pushl %edx
	pushl %ecx
	pushl %ebx
	cli
	call *jump_table(, %eax, 4);
	
	# pop the arguments
	popl %ebx
	popl %ecx
	popl %edx

DONE:
	# restore caller save reg
	popl %ebp
	popl %edi
	popl %esi
	#popal
	popfl
	iret

# INVALID index for system call
INVALID:
	movl $-1, %eax
	jmp DONE

jump_table:
	.long system_halt, system_execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn

