/* Miscellaneous BPABI functions.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.
   Contributed by CodeSourcery, LLC.

   This file is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */


	.syntax unified
	.text
	.thumb

#define xxh r1
#define xxl r0
#define yyh r3
#define yyl r2

.macro ARM_FUNC_START1 name
	.thumb_func
	.globl \name
	.align 0
\name:
.endm

ARM_FUNC_START1 __aeabi_lcmp
	cmp	xxh, yyh
	it	lt
	movlt	r0, #-1
	it	gt
	movgt	r0, #1
	it	ne
	bxne lr
	subs	r0, xxl, yyl
	it	lo
	movlo	r0, #-1
	it	hi
	movhi	r0, #1
	bx lr

ARM_FUNC_START1 __aeabi_ulcmp
	cmp	xxh, yyh
	it	lo
	movlo	r0, #-1
	it	hi
	movhi	r0, #1
	it	ne
	bxne lr
	cmp	xxl, yyl
	it	lo
	movlo	r0, #-1
	it	hi
	movhi	r0, #1
	it	eq
	moveq	r0, #0
	bx lr

.macro test_div_by_zero signed
/* Tail-call to divide-by-zero handlers which may be overridden by the user,
   so unwinding works properly.  */
	cbnz	yyh, 2f
	cbnz	yyl, 2f
	cmp	xxh, #0
	.ifc \signed, unsigned
	it	eq
	cmpeq	xxl, #0
	itt	ne
	movne	xxh, #0xffffffff
	movne	xxl, #0xffffffff
	.else
	ittt	lt
	movlt	xxl, #0
	movlt	xxh, #0x80000000
	blt	1f
	it	eq
	cmpeq	xxl, #0
	itt	ne
	movne	xxh, #0x7fffffff
	movne	xxl, #0xffffffff
	.endif
1:	
	b	__aeabi_ldiv0
2:
.endm


/* set up stack from for call to __udivmoddi4. At the end of the macro the
   stack is arranged as follows:
		sp+12	/ space for remainder
		sp+8	\ (written by __udivmoddi4)
		sp+4	lr
		sp+0	sp+8 [rp (remainder pointer) argument for __udivmoddi4]

 */
.macro push_for_divide
	sub	ip, sp, #8
	strd	ip, lr, [sp, #-16]!
.endm

/* restore stack */
.macro pop_for_divide
	ldr	lr, [sp, #4]
	ldrd	r2, r3, [sp, #8]
	add	sp, sp, #16
.endm

/* Perform 64 bit signed division.
   Inputs:
	r0:r1	numerator
	r2:r3	denominator
   Outputs:
	r0:r1	quotient
	r2:r3	remainder
 */
ARM_FUNC_START1 __aeabi_ldivmod 
	test_div_by_zero	signed

	push_for_divide
	cmp	xxh, #0
	blt	1f
	cmp	yyh, #0
	blt	2f
	/* arguments in (r0:r1), (r2:r3) and *sp */
	bl	__udivmoddi4
	pop_for_divide
	bx lr

1: /* xxh:xxl is negative */
	negs	xxl, xxl
	sbc	xxh, xxh, xxh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	cmp	yyh, #0
	blt	3f
	/* arguments in (r0:r1), (r2:r3) and *sp */
	bl	__udivmoddi4
	pop_for_divide
	negs	xxl, xxl
	sbc	xxh, xxh, xxh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	negs	yyl, yyl
	sbc	yyh, yyh, yyh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	bx lr

2: /* only yyh:yyl is negative */
	negs	yyl, yyl
	sbc	yyh, yyh, yyh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	/* arguments in (r0:r1), (r2:r3) and *sp */
	bl	__udivmoddi4
	pop_for_divide
	negs	xxl, xxl
	sbc	xxh, xxh, xxh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	bx lr

3: /* both xxh:xxl and yyh:yyl are negative */
	negs	yyl, yyl
	sbc	yyh, yyh, yyh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	/* arguments in (r0:r1), (r2:r3) and *sp */
	bl	__udivmoddi4
	pop_for_divide
	negs	yyl, yyl
	sbc	yyh, yyh, yyh, lsl #1	/* Thumb-2 has no RSC, so use X - 2X */
	bx lr
	

/* Perform 64 bit signed division.
   Inputs:
	r0:r1	numerator
	r2:r3	denominator
   Outputs:
	r0:r1	quotient
	r2:r3	remainder
 */
ARM_FUNC_START1 __aeabi_uldivmod	
	test_div_by_zero	unsigned

	push_for_divide
	/* arguments in (r0:r1), (r2:r3) and *sp */
	bl	__udivmoddi4
	pop_for_divide
	bx lr

