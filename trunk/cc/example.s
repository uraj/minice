	.file	"example.c"
	.comm	a, 4, 4
	.comm	b, 4, 4
	.comm	c, 40, 4
	.comm	d, 1, 1
	.comm	e, 4, 4
	.comm	f, 1, 1
	.comm	g, 10, 4
	.section	.rodata
	.align	3
.LC0:
	.ascii	"hello world again.\000"
	.align	3
.LC1:
	.ascii	"good-bye world again.\000"
	.text
	.align	2
	.global	test
	.type	test,function
test:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stw	pc, [sp-], #4;
	stw	lr, [sp-], #8;
	stw	ip, [sp-], #12;
	stw	ip, [sp-], #12;stw	fp, [sp-], #16;
	stw	ip, [sp-], #12;stw	fp, [sp-], #16;sub	sp, sp, #16;
	sub	fp, ip, #4
	ldw	r15, .L3
	ldw	r14, .L3+4
	stw	r14, [r15+], #0
	ldw	r15, .L3
	ldw	r14, .L3+8
	stw	r14, [r15+], #0
	mov	r15, #1
	mov	r0, r15
	mov	ip, fp
	ldw	fp,  [fp-], #12
	ldw	sp,  [ip-], #8
	ldw	ip,  [ip-], #4
	jump	ip
.L4:
	.align	2
.L3:
	.word	e
	.word	.LC0
	.word	.LC1
	.size	test, .-test
	.align	2
	.global	no_str
	.type	no_str,function
no_str:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stw	pc, [sp-], #4;
	stw	lr, [sp-], #8;
	stw	ip, [sp-], #12;
	stw	ip, [sp-], #12;stw	fp, [sp-], #16;
	stw	ip, [sp-], #12;stw	fp, [sp-], #16;sub	sp, sp, #16;
	sub	fp, ip, #4
	sub	sp, sp, #8
	stw	r0, [fp+], #-16
	stw	r1, [fp+], #-20
	ldw	r15, [fp+], #-16
	add	r15, r15, #1
	stw	r15, [fp+], #-16
	ldw	r15, [fp+], #-20
	add	r15, r15, #1
	stw	r15, [fp+], #-20
	ldw	r15, [fp+], #-16
	mov	r0, r15
	mov	ip, fp
	ldw	fp,  [fp-], #12
	ldw	sp,  [ip-], #8
	ldw	ip,  [ip-], #4
	jump	ip
	.size	no_str, .-no_str
	.section	.rodata
	.align	3
.LC2:
	.ascii	"hello world.\000"
	.align	3
.LC3:
	.ascii	"good-bye world.\000"
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
	ldw	r15, .L9
	ldw	r14, .L9+4
	stw	r14, [r15+], #0
	ldw	r15, .L9
	ldw	r14, .L9+8
	stw	r14, [r15+], #0
	mov	r15, #1
	mov	r0, r15
	mov	ip, fp
	ldw	fp,  [fp-], #12
	ldw	sp,  [ip-], #8
	ldw	ip,  [ip-], #4
	jump	ip
.L10:
	.align	2
.L9:
	.word	e
	.word	.LC2
	.word	.LC3
	.size	main, .-main
	.ident	"GCC: (UC4_1.0_gama_20101126) 4.4.2"
