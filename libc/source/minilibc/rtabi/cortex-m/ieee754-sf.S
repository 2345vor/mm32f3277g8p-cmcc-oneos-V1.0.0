/* ieee754-sf.S single-precision floating point support for ARM

   Copyright (C) 2003-2020 Free Software Foundation, Inc.
   Contributed by Nicolas Pitre (nico@fluxnic.net)

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

/*
 * Notes:
 *
 * The goal of this code is to be as fast as possible.  This is
 * not meant to be easy to understand for the casual reader.
 *
 * Only the default rounding mode is intended for best performances.
 * Exceptions aren't supported yet, but that can be added quite easily
 * if necessary without impacting performances.
 *
 */
	.syntax unified
	.text
	.thumb

#define xl r0
#define xh r1
#define yl r2
#define yh r3
#define al r0
#define ah r1


.macro ARM_FUNC_START name
	.thumb_func
	.globl \name
	.align 0
\name:
.endm

ARM_FUNC_START __aeabi_fneg
	eor	r0, r0, #0x80000000	@ flip sign bit
	bx lr


ARM_FUNC_START __aeabi_frsub
	eor	r0, r0, #0x80000000	@ flip sign bit of first arg
	b	1f

ARM_FUNC_START __aeabi_fsub

	eor	r2, r2, #0x80000000	@ flip sign bit of second arg

ARM_FUNC_START __aeabi_fadd

1:	@ Look for zeroes, equal values, INF, or NAN.
	movs	r2, r0, lsl #1
	itttt	ne
	movsne	r3, r1, lsl #1
	teqne	r2, r3
	mvnsne	ip, r2, asr #24
	mvnsne	ip, r3, asr #24
	beq	.fLad_s

	@ Compute exponent difference.  Make largest exponent in r2,
	@ corresponding arg in r0, and positive exponent difference in r3.
	mov	r2, r2, lsr #24
	rsbs	r3, r2, r3, lsr #24
	itttt	gt
	addgt	r2, r2, r3
	eorgt	r1, r0, r1
	eorgt	r0, r1, r0
	eorgt	r1, r0, r1
	it	lt
	rsblt	r3, r3, #0

	@ If exponent difference is too large, return largest argument
	@ already in r0.  We need up to 25 bit to handle proper rounding
	@ of 0x1p25 - 1.1.
	cmp	r3, #25
	it	hi
	bxhi lr

	@ Convert mantissa to signed integer.
	tst	r0, #0x80000000
	orr	r0, r0, #0x00800000
	bic	r0, r0, #0xff000000
	it	ne
	rsbne	r0, r0, #0
	tst	r1, #0x80000000
	orr	r1, r1, #0x00800000
	bic	r1, r1, #0xff000000
	it	ne
	rsbne	r1, r1, #0

	@ If exponent == difference, one or both args were denormalized.
	@ Since this is not common case, rescale them off line.
	teq	r2, r3
	beq	.fLad_d
.fLad_x:

	@ Compensate for the exponent overlapping the mantissa MSB added later
	sub	r2, r2, #1

	@ Shift and add second arg to first arg in r0.
	@ Keep leftover bits into r1.
	asr ip, r1, r3
	adds r0, r0, ip
	rsb	r3, r3, #32
	lsl r1, r1, r3

	@ Keep absolute value in r0-r1, sign in r3 (the n bit was set above)
	and	r3, r0, #0x80000000
	bpl	.fLad_p

	negs	r1, r1
	sbc	r0, r0, r0, lsl #1

	@ Determine how to normalize the result.
.fLad_p:
	cmp	r0, #0x00800000
	bcc	.fLad_a
	cmp	r0, #0x01000000
	bcc	.fLad_e

	@ Result needs to be shifted right.
	movs	r0, r0, lsr #1
	mov	r1, r1, rrx
	add	r2, r2, #1

	@ Make sure we did not bust our exponent.
	cmp	r2, #254
	bhs	.fLad_o

	@ Our result is now properly aligned into r0, remaining bits in r1.
	@ Pack final result together.
	@ Round with MSB of r1. If halfway between two numbers, round towards
	@ LSB of r0 = 0. 
.fLad_e:
	cmp	r1, #0x80000000
	adc	r0, r0, r2, lsl #23
	it	eq
	biceq	r0, r0, #1
	orr	r0, r0, r3
	bx lr

	@ Result must be shifted left and exponent adjusted.
.fLad_a:
	movs	r1, r1, lsl #1
	adc	r0, r0, r0
	subs	r2, r2, #1
	it	hs
	cmphs	r0, #0x00800000
	bhs	.fLad_e

	@ No rounding necessary since r1 will always be 0 at this point.
.fLad_l:


	movs	ip, r0, lsr #12
	moveq	r0, r0, lsl #12
	subeq	r2, r2, #12
	tst	r0, #0x00ff0000
	moveq	r0, r0, lsl #8
	subeq	r2, r2, #8
	tst	r0, #0x00f00000
	moveq	r0, r0, lsl #4
	subeq	r2, r2, #4
	tst	r0, #0x00c00000
	moveq	r0, r0, lsl #2
	subeq	r2, r2, #2
	cmp	r0, #0x00800000
	movcc	r0, r0, lsl #1
	sbcs	r2, r2, #0


	@ Final result with sign
	@ If exponent negative, denormalize result.
	itet	ge
	addge	r0, r0, r2, lsl #23
	rsblt	r2, r2, #0
	orrge	r0, r0, r3

	itt	lt
	lsrlt	r0, r0, r2
	orrlt	r0, r3, r0
	bx lr

	@ Fixup and adjust bit position for denormalized arguments.
	@ Note that r2 must not remain equal to 0.
.fLad_d:
	teq	r2, #0
	eor	r1, r1, #0x00800000
	itte	eq
	eoreq	r0, r0, #0x00800000
	addeq	r2, r2, #1
	subne	r3, r3, #1
	b	.fLad_x

.fLad_s:
	mov	r3, r1, lsl #1

	mvns	ip, r2, asr #24
	it	ne
	mvnsne	ip, r3, asr #24
	beq	.fLad_i

	teq	r2, r3
	beq	1f

	@ Result is x + 0.0 = x or 0.0 + y = y.
	teq	r2, #0
	it	eq
	moveq	r0, r1
	bx lr

1:	teq	r0, r1

	@ Result is x - x = 0.
	itt	ne
	movne	r0, #0
	bxne lr

	@ Result is x + x = 2x.
	tst	r2, #0xff000000
	bne	2f
	movs	r0, r0, lsl #1
	it	cs
	orrcs	r0, r0, #0x80000000
	bx lr
2:	adds	r2, r2, #(2 << 24)
	itt	cc
	addcc	r0, r0, #(1 << 23)
	bxcc lr
	and	r3, r0, #0x80000000

	@ Overflow: return INF.
.fLad_o:
	orr	r0, r3, #0x7f000000
	orr	r0, r0, #0x00800000
	bx lr

	@ At least one of r0/r1 is INF/NAN.
	@   if r0 != INF/NAN: return r1 (which is INF/NAN)
	@   if r1 != INF/NAN: return r0 (which is INF/NAN)
	@   if r0 or r1 is NAN: return NAN
	@   if opposite sign: return NAN
	@   otherwise return r0 (which is INF or -INF)
.fLad_i:
	mvns	r2, r2, asr #24
	itet	ne
	movne	r0, r1
	mvnseq	r3, r3, asr #24
	movne	r1, r0
	movs	r2, r0, lsl #9
	itte	eq
	movseq	r3, r1, lsl #9
	teqeq	r0, r1
	orrne	r0, r0, #0x00400000	@ quiet NAN
	bx lr


ARM_FUNC_START __aeabi_ui2f
	mov	r3, #0
	b	1f

ARM_FUNC_START __aeabi_i2f
	ands	r3, r0, #0x80000000
	it	mi
	rsbmi	r0, r0, #0

1:	movs	ip, r0
	it	eq
	bxeq lr

	@ Add initial exponent to sign
	orr	r3, r3, #((127 + 23) << 23)

	.ifnc	ah, r0
	mov	ah, r0
	.endif
	mov	al, #0
	b	2f


ARM_FUNC_START __aeabi_ul2f
	orrs	r2, r0, r1
	it	eq
	bxeq lr

	mov	r3, #0
	b	1f

ARM_FUNC_START __aeabi_l2f
	orrs	r2, r0, r1
	it	eq
	bxeq lr

	ands	r3, ah, #0x80000000	@ sign bit in r3
	bpl	1f

	negs	al, al
	sbc	ah, ah, ah, lsl #1
1:
	movs	ip, ah
	ittt	eq
	moveq	ip, al
	moveq	ah, al
	moveq	al, #0

	@ Add initial exponent to sign
	orr	r3, r3, #((127 + 23 + 32) << 23)
	it	eq
	subeq	r3, r3, #(32 << 23)
2:	sub	r3, r3, #(1 << 23)


	mov	r2, #23
	cmp	ip, #(1 << 16)
	itt	hs
	movhs	ip, ip, lsr #16
	subhs	r2, r2, #16
	cmp	ip, #(1 << 8)
	itt	hs
	movhs	ip, ip, lsr #8
	subhs	r2, r2, #8
	cmp	ip, #(1 << 4)
	itt	hs
	movhs	ip, ip, lsr #4
	subhs	r2, r2, #4
	cmp	ip, #(1 << 2)
	ite	hs
	subhs	r2, r2, #2
	sublo	r2, r2, ip, lsr #1
	subs	r2, r2, ip, lsr #3


	sub	r3, r3, r2, lsl #23
	blt	3f

	lsl ip, ah, r2
	add r3, r3, ip
	lsl ip, al, r2
	rsb	r2, r2, #32
	cmp	ip, #0x80000000
	lsr r2, al, r2
	adc r0, r3, r2
	it	eq
	biceq	r0, r0, #1
	bx lr

3:	add	r2, r2, #32
	lsl ip, ah, r2
	rsb	r2, r2, #32
	orrs	al, al, ip, lsl #1
	lsr r2, ah, r2
	adc r0, r3, r2
	it	eq
	biceq	r0, r0, ip, lsr #31
	bx lr


ARM_FUNC_START __aeabi_fmul
	@ Mask out exponents, trap any zero/denormal/INF/NAN.
	mov	ip, #0xff
	ands	r2, ip, r0, lsr #23
	ittt	ne
	andsne	r3, ip, r1, lsr #23
	teqne	r2, ip
	teqne	r3, ip
	beq	.fLml_s
.fLml_x:

	@ Add exponents together
	add	r2, r2, r3

	@ Determine final sign.
	eor	ip, r0, r1

	@ Convert mantissa to unsigned integer.
	@ If power of two, branch to a separate path.
	@ Make up for final alignment.
	movs	r0, r0, lsl #9
	it	ne
	movsne	r1, r1, lsl #9
	beq	.fLml_1
	mov	r3, #0x08000000
	orr	r0, r3, r0, lsr #5
	orr	r1, r3, r1, lsr #5

	@ The actual multiplication.
	@ This code works on architecture versions >= 4
	umull	r3, r1, r0, r1

	@ Put final sign in r0.
	and	r0, ip, #0x80000000

	@ Adjust result upon the MSB position.
	cmp	r1, #(1 << 23)
	ittt	cc
	movcc	r1, r1, lsl #1
	orrcc	r1, r1, r3, lsr #31
	movcc	r3, r3, lsl #1

	@ Add sign to result.
	orr	r0, r0, r1

	@ Apply exponent bias, check for under/overflow.
	sbc	r2, r2, #127
	cmp	r2, #(254 - 1)
	bhi	.fLml_u

	@ Round the result, merge final exponent.
	cmp	r3, #0x80000000
	adc	r0, r0, r2, lsl #23
	it	eq
	biceq	r0, r0, #1
	bx lr

	@ Multiplication by 0x1p*: let''s shortcut a lot of code.
.fLml_1:
	teq	r0, #0
	and	ip, ip, #0x80000000
	it	eq
	moveq	r1, r1, lsl #9
	orr	r0, ip, r0, lsr #9
	orr	r0, r0, r1, lsr #9
	subs	r2, r2, #127
	ittt	gt
	rsbsgt	r3, r2, #255
	orrgt	r0, r0, r2, lsl #23
	bxgt lr

	@ Under/overflow: fix things up for the code below.
	orr	r0, r0, #0x00800000
	mov	r3, #0
	subs	r2, r2, #1

.fLml_u:
	@ Overflow?
	bgt	.fLml_o

	@ Check if denormalized result is possible, otherwise return signed 0.
	cmn	r2, #(24 + 1)
	itt	le
	bicle	r0, r0, #0x7fffffff
	bxle lr

	@ Shift value right, round, etc.
	rsb	r2, r2, #0
	movs	r1, r0, lsl #1
	lsr r1, r1, r2
	rsb	r2, r2, #32
	lsl ip, r0, r2
	movs	r0, r1, rrx
	adc	r0, r0, #0
	orrs	r3, r3, ip, lsl #1
	it	eq
	biceq	r0, r0, ip, lsr #31
	bx lr

	@ One or both arguments are denormalized.
	@ Scale them leftwards and preserve sign bit.
.fLml_d:
	teq	r2, #0
	and	ip, r0, #0x80000000
1:	ittt	eq
	moveq	r0, r0, lsl #1
	tsteq	r0, #0x00800000
	subeq	r2, r2, #1
	beq	1b
	orr	r0, r0, ip
	teq	r3, #0
	and	ip, r1, #0x80000000
2:	ittt	eq
	moveq	r1, r1, lsl #1
	tsteq	r1, #0x00800000
	subeq	r3, r3, #1
	beq	2b
	orr	r1, r1, ip
	b	.fLml_x

.fLml_s:
	@ Isolate the INF and NAN cases away
	and	r3, ip, r1, lsr #23
	teq	r2, ip
	it	ne
	teqne	r3, ip
	beq	1f

	@ Here, one or more arguments are either denormalized or zero.
	bics	ip, r0, #0x80000000
	it	ne
	bicsne	ip, r1, #0x80000000
	bne	.fLml_d

	@ Result is 0, but determine sign anyway.
.fLml_z:
	eor	r0, r0, r1
	bic	r0, r0, #0x7fffffff
	bx lr

1:	@ One or both args are INF or NAN.
	teq	r0, #0x0
	itett	ne
	teqne	r0, #0x80000000
	moveq	r0, r1
	teqne	r1, #0x0
	teqne	r1, #0x80000000
	beq	.fLml_n		@ 0 * INF or INF * 0 -> NAN
	teq	r2, ip
	bne	1f
	movs	r2, r0, lsl #9
	bne	.fLml_n		@ NAN * <anything> -> NAN
1:	teq	r3, ip
	bne	.fLml_i
	movs	r3, r1, lsl #9
	it	ne
	movne	r0, r1
	bne	.fLml_n		@ <anything> * NAN -> NAN

	@ Result is INF, but we need to determine its sign.
.fLml_i:
	eor	r0, r0, r1

	@ Overflow: return INF (sign already in r0).
.fLml_o:
	and	r0, r0, #0x80000000
	orr	r0, r0, #0x7f000000
	orr	r0, r0, #0x00800000
	bx lr

	@ Return a quiet NAN.
.fLml_n:
	orr	r0, r0, #0x7f000000
	orr	r0, r0, #0x00c00000
	bx lr


ARM_FUNC_START __aeabi_fdiv
	@ Mask out exponents, trap any zero/denormal/INF/NAN.
	mov	ip, #0xff
	ands	r2, ip, r0, lsr #23
	ittt	ne
	andsne	r3, ip, r1, lsr #23
	teqne	r2, ip
	teqne	r3, ip
	beq	.fLdv_s
.fLdv_x:

	@ Subtract divisor exponent from dividend''s
	sub	r2, r2, r3

	@ Preserve final sign into ip.
	eor	ip, r0, r1

	@ Convert mantissa to unsigned integer.
	@ Dividend -> r3, divisor -> r1.
	movs	r1, r1, lsl #9
	mov	r0, r0, lsl #9
	beq	.fLdv_1
	mov	r3, #0x10000000
	orr	r1, r3, r1, lsr #4
	orr	r3, r3, r0, lsr #4

	@ Initialize r0 (result) with final sign bit.
	and	r0, ip, #0x80000000

	@ Ensure result will land to known bit position.
	@ Apply exponent bias accordingly.
	cmp	r3, r1
	it	cc
	movcc	r3, r3, lsl #1
	adc	r2, r2, #(127 - 2)

	@ The actual division loop.
	mov	ip, #0x00800000
1:	cmp	r3, r1
	itt	cs
	subcs	r3, r3, r1
	orrcs	r0, r0, ip
	cmp	r3, r1, lsr #1
	itt	cs
	subcs	r3, r3, r1, lsr #1
	orrcs	r0, r0, ip, lsr #1
	cmp	r3, r1, lsr #2
	itt	cs
	subcs	r3, r3, r1, lsr #2
	orrcs	r0, r0, ip, lsr #2
	cmp	r3, r1, lsr #3
	itt	cs
	subcs	r3, r3, r1, lsr #3
	orrcs	r0, r0, ip, lsr #3
	movs	r3, r3, lsl #4
	it	ne
	movsne	ip, ip, lsr #4
	bne	1b

	@ Check exponent for under/overflow.
	cmp	r2, #(254 - 1)
	bhi	.fLml_u

	@ Round the result, merge final exponent.
	cmp	r3, r1
	adc	r0, r0, r2, lsl #23
	it	eq
	biceq	r0, r0, #1
	bx lr

	@ Division by 0x1p*: let''s shortcut a lot of code.
.fLdv_1:
	and	ip, ip, #0x80000000
	orr	r0, ip, r0, lsr #9
	adds	r2, r2, #127
	ittt	gt
	rsbsgt	r3, r2, #255
	orrgt	r0, r0, r2, lsl #23
	bxgt lr

	orr	r0, r0, #0x00800000
	mov	r3, #0
	subs	r2, r2, #1
	b	.fLml_u

	@ One or both arguments are denormalized.
	@ Scale them leftwards and preserve sign bit.
.fLdv_d:
	teq	r2, #0
	and	ip, r0, #0x80000000
1:	ittt	eq
	moveq	r0, r0, lsl #1
	tsteq	r0, #0x00800000
	subeq	r2, r2, #1
	beq	1b
	orr	r0, r0, ip
	teq	r3, #0
	and	ip, r1, #0x80000000
2:	ittt	eq
	moveq	r1, r1, lsl #1
	tsteq	r1, #0x00800000
	subeq	r3, r3, #1
	beq	2b
	orr	r1, r1, ip
	b	.fLdv_x

	@ One or both arguments are either INF, NAN, zero or denormalized.
.fLdv_s:
	and	r3, ip, r1, lsr #23
	teq	r2, ip
	bne	1f
	movs	r2, r0, lsl #9
	bne	.fLml_n		@ NAN / <anything> -> NAN
	teq	r3, ip
	bne	.fLml_i		@ INF / <anything> -> INF
	mov	r0, r1
	b	.fLml_n		@ INF / (INF or NAN) -> NAN
1:	teq	r3, ip
	bne	2f
	movs	r3, r1, lsl #9
	beq	.fLml_z		@ <anything> / INF -> 0
	mov	r0, r1
	b	.fLml_n		@ <anything> / NAN -> NAN
2:	@ If both are nonzero, we need to normalize and resume above.
	bics	ip, r0, #0x80000000
	it	ne
	bicsne	ip, r1, #0x80000000
	bne	.fLdv_d
	@ One or both arguments are zero.
	bics	r2, r0, #0x80000000
	bne	.fLml_i		@ <non_zero> / 0 -> INF
	bics	r3, r1, #0x80000000
	bne	.fLml_z		@ 0 / <non_zero> -> 0
	b	.fLml_n		@ 0 / 0 -> NAN


@ The return value in r0 is
@
@   0  if the operands are equal
@   1  if the first operand is greater than the second, or
@      the operands are unordered and the operation is
@      CMP, LT, LE, NE, or EQ.
@   -1 if the first operand is less than the second, or
@      the operands are unordered and the operation is GT
@      or GE.
@
@ The Z flag will be set iff the operands are equal.
@
@ The following registers are clobbered by this function:
@   ip, r0, r1, r2, r3

ARM_FUNC_START __gtsf2
ARM_FUNC_START __gesf2
	mov	ip, #-1
	b	1f

ARM_FUNC_START __ltsf2
ARM_FUNC_START __lesf2
	mov	ip, #1
	b	1f

ARM_FUNC_START __cmpsf2
ARM_FUNC_START __nesf2
ARM_FUNC_START __eqsf2
	mov	ip, #1			@ how should we specify unordered here?

1:	str	ip, [sp, #-4]!

	@ Trap any INF/NAN first.
	mov	r2, r0, lsl #1
	mov	r3, r1, lsl #1
	mvns	ip, r2, asr #24
	it	ne
	mvnsne	ip, r3, asr #24
	beq	3f

	@ Compare values.
	@ Note that 0.0 is equal to -0.0.
2:	add	sp, sp, #4

	orrs	ip, r2, r3, lsr #1	@ test if both are 0, clear C flag
	it	ne
	teqne	r0, r1			@ if not 0 compare sign
	it	pl
	subspl	r0, r2, r3		@ if same sign compare values, set r0

	@ Result:
	it	hi
	movhi	r0, r1, asr #31
	it	lo
	mvnlo	r0, r1, asr #31
	it	ne
	orrne	r0, r0, #1
	bx lr

3:	@ Look for a NAN.

	mvns	ip, r2, asr #24
	bne	4f
	movs	ip, r0, lsl #9
	bne	5f			@ r0 is NAN
4:	mvns	ip, r3, asr #24
	bne	2b
	movs	ip, r1, lsl #9
	beq	2b			@ r1 is not NAN

5:	ldr	r0, [sp], #4		@ return unordered code.
	bx lr


ARM_FUNC_START __aeabi_cfrcmple
	mov	ip, r0
	mov	r0, r1
	mov	r1, ip
	b	6f

ARM_FUNC_START __aeabi_cfcmpeq
ARM_FUNC_START __aeabi_cfcmple

	@ The status-returning routines are required to preserve all
	@ registers except ip, lr, and cpsr.
6:	push	{r0, r1, r2, r3, lr}

	bl __cmpsf2
	@ Set the Z flag correctly, and the C flag unconditionally.
	cmp	r0, #0
	@ Clear the C flag if the return value was -1, indicating
	@ that the first operand was smaller than the second.
	it	mi
	cmnmi	r0, #0
	pop {r0, r1, r2, r3, lr}
	bx lr


ARM_FUNC_START	__aeabi_fcmpeq
	str	lr, [sp, #-8]!    @ sp -= 8

	bl __aeabi_cfcmple
	ite	eq
	moveq	r0, #1	@ Equal to.
	movne	r0, #0	@ Less than, greater than, or unordered.
	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START	__aeabi_fcmplt
	str	lr, [sp, #-8]!    @ sp -= 8

	bl __aeabi_cfcmple
	ite	cc
	movcc	r0, #1	@ Less than.
	movcs	r0, #0	@ Equal to, greater than, or unordered.
	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START	__aeabi_fcmple
	str	lr, [sp, #-8]!    @ sp -= 8

	bl __aeabi_cfcmple
	ite	ls
	movls	r0, #1  @ Less than or equal to.
	movhi	r0, #0	@ Greater than or unordered.
	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START	__aeabi_fcmpge
	str	lr, [sp, #-8]!    @ sp -= 8

	bl __aeabi_cfrcmple
	ite	ls
	movls	r0, #1	@ Operand 2 is less than or equal to operand 1.
	movhi	r0, #0	@ Operand 2 greater than operand 1, or unordered.
	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START	__aeabi_fcmpgt

	str	lr, [sp, #-8]!    @ sp -= 8

	bl __aeabi_cfrcmple
	ite	cc
	movcc	r0, #1	@ Operand 2 is less than operand 1.
	movcs	r0, #0  @ Operand 2 is greater than or equal to operand 1,
			@ or they are unordered.
	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START __aeabi_fcmpun
	mov	r2, r0, lsl #1
	mov	r3, r1, lsl #1
	mvns	ip, r2, asr #24
	bne	1f
	movs	ip, r0, lsl #9
	bne	3f			@ r0 is NAN
1:	mvns	ip, r3, asr #24
	bne	2f
	movs	ip, r1, lsl #9
	bne	3f			@ r1 is NAN
2:	mov	r0, #0			@ arguments are ordered.
	bx lr
3:	mov	r0, #1			@ arguments are unordered.
	bx lr


ARM_FUNC_START __aeabi_f2iz
	@ check exponent range.
	mov	r2, r0, lsl #1
	cmp	r2, #(127 << 24)
	bcc	1f			@ value is too small
	mov	r3, #(127 + 31)
	subs	r2, r3, r2, lsr #24
	bls	2f			@ value is too large

	@ scale value
	mov	r3, r0, lsl #8
	orr	r3, r3, #0x80000000
	tst	r0, #0x80000000		@ the sign bit
	lsr r0, r3, r2
	it	ne
	rsbne	r0, r0, #0
	bx lr

1:	mov	r0, #0
	bx lr

2:	cmp	r2, #(127 + 31 - 0xff)
	bne	3f
	movs	r2, r0, lsl #9
	bne	4f			@ r0 is NAN.
3:	ands	r0, r0, #0x80000000	@ the sign bit
	it	eq
	moveq	r0, #0x7fffffff		@ the maximum signed positive si
	bx lr

4:	mov	r0, #0			@ What should we convert NAN to?
	bx lr


ARM_FUNC_START __aeabi_f2uiz
	@ check exponent range.
	movs	r2, r0, lsl #1
	bcs	1f			@ value is negative
	cmp	r2, #(127 << 24)
	bcc	1f			@ value is too small
	mov	r3, #(127 + 31)
	subs	r2, r3, r2, lsr #24
	bmi	2f			@ value is too large

	@ scale the value
	mov	r3, r0, lsl #8
	orr	r3, r3, #0x80000000
	lsr r0, r3, r2
	bx lr

1:	mov	r0, #0
	bx lr

2:	cmp	r2, #(127 + 31 - 0xff)
	bne	3f
	movs	r2, r0, lsl #9
	bne	4f			@ r0 is NAN.
3:	mov	r0, #0xffffffff		@ maximum unsigned si
	bx lr

4:	mov	r0, #0			@ What should we convert NAN to?
	bx lr

ARM_FUNC_START __aeabi_f2h
	push {lr}
	bl __gnu_f2h_ieee
	pop {lr}
	bx lr
	
ARM_FUNC_START __aeabi_f2h_alt
	push {lr}
	bl __gnu_f2h_alternative
	pop {lr}
	bx lr
	
ARM_FUNC_START __aeabi_h2f
	push {lr}
	bl __gnu_h2f_ieee
	pop {lr}
	bx lr
	
ARM_FUNC_START __aeabi_h2f_alt
	push {lr}
	bl __gnu_h2f_alternative
	pop {lr}
	bx lr

