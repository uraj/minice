	.file	"main.c"
	.text
	.align	2
	.global	main
	.type	main,function
main:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stw	pc, [sp-], #4;
	stw	lr, [sp-], #8;
	stw	ip, [sp-], #12;
	stw	ip, [sp-], #12;stw	fp, [sp-], #16;
	stw	ip, [sp-], #12;stw	fp, [sp-], #16;sub	sp, sp, #16;
	sub	fp, ip, #4
	mov	r15, #511

	
	stw r15, [sp-], #0
	ldb r15, [sp+], #3
	ldb r15, [sp+], #2
	ldb r15, [sp+], #1
	ldb r15, [sp+], #0
	stb r15, [sp+], #3
	
	mov	r0, r15
	mov	ip, fp
	ldw	fp,  [fp-], #12
	ldw	sp,  [ip-], #8
	ldw	ip,  [ip-], #4
	jump	ip
	.size	main, .-main
	.ident	"GCC: (UC4_1.0_gama_20101126) 4.4.2"
