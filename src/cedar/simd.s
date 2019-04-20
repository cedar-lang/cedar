	.section .text
	.global vecadd
	.intel_syntax noprefix
vecadd:
	.cfi_startproc
	movaps xmm0, [rsi]
	addps  xmm0, [rdx]
	movaps [rdi], xmm0
	ret
	.cfi_endproc
