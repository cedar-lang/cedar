		.section	__TEXT,__text,regular,pure_instructions
		.global _get_sp
		.intel_syntax noprefix
_get_sp:
	.cfi_startproc
	push rbp
	mov	 rbp, rsp
	mov  rax, rsp
	pop  rbp
	ret
	.cfi_endproc



		.global _add
_add:
	.cfi_startproc
	push   rbp
	mov    rbp, rsp ; rbp = rsp

	# edi = arg1
	# esi = arg2

	mov rdi, qword ptr [rdi]
	mov rsi, qword ptr [rsi]

	# offload args to stack
	mov qword ptr [rbp-8],  rdi
	mov qword ptr [rbp-16], rsi



	mov eax, edi

	cvtsi2ss xmm0, eax


	#return
	pop rbp
	ret


	mov    rax, rdi
	shr    rax, 48
	cmp    rax, 0x80
	jne    error_not_valid



	mov    rax, rsi
	shr    rax, 48
	cmp    rax, 0x80
	jne    error_not_valid

	# they are both ints, add them

	xor rax, rax
	add eax, edi
	add eax, esi

	pop rbp
	ret


error_not_valid:
	xor eax, eax
	cvtsi2ss xmm0, eax
	pop rbp
	ret
	.cfi_endproc


