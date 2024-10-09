.section .text
	.globl add
	.p2align 2
add:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]
	add	x29, sp, #16
	str	w0, [sp, #12]
	str	w1, [sp, #8]
	ldr	w0, [sp, #12]
	ldr	w1, [sp, #8]
	add	w0, w0, w1
	str	w0, [sp, #4]
	mov	w0, w0
	ldp	x29, x30, [sp, #16]
	add	sp, sp, #32
	ret

	.globl main
	.p2align 2
main:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]
	add	x29, sp, #16
	mov	w0, #0
	str	w0, [sp, #12]
	mov	w1, #0
	str	w1, [sp, #8]
	mov	w0, #0
	str	w0, [sp, #12]
;loop start
_lb_2:
	ldr	w0, [sp, #12]
	cmp	w0, #10
	b.lt	_lb_0
	b	_lb_1
_lb_0:
	ldr	w1, [sp, #8]
	mov	w0, w1
	mov	w1, #1
	bl	add
	mov	w0, w0
	str	w0, [sp, #8]
	ldr	w1, [sp, #12]
	add	w1, w1, #1
	str	w1, [sp, #4]
	mov	w2, w1
	str	w2, [sp, #12]
	b	_lb_2
_lb_1:
;loop end
	ldr	w0, [sp, #8]
	mov	w0, w0
	ldp	x29, x30, [sp, #16]
	add	sp, sp, #32
	ret

