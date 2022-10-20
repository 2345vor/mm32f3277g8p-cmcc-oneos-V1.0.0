/* ieee754-df.S double-precision floating point support for ARM

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
 * For slightly simpler code please see the single precision version
 * of this file.
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

.macro ARM_FUNC_START2 name
	.thumb_func
	.globl \name
	.align 0
\name:
.endm

ARM_FUNC_START2 __aeabi_dneg
	@ flip sign bit
	eor	xh, xh, #0x80000000
	bx lr
	

ARM_FUNC_START2 __aeabi_drsub

	eor	xh, xh, #0x80000000	@ flip sign bit of first arg
	b	1f	

ARM_FUNC_START2 __aeabi_dsub

	eor	yh, yh, #0x80000000	@ flip sign bit of second arg

ARM_FUNC_START2 __aeabi_dadd

1:  push {r4, r5, lr}        @ sp -= 12

	@ Look for zeroes, equal values, INF, or NAN.
	lsl r4, xh, #1
	lsl r5, yh, #1
	teq	r4, r5
	it	eq
	teqeq	xl, yl
	itttt	ne
	orrsne	ip, r4, xl
	orrsne	ip, r5, yl
	mvnsne	ip, r4, asr #21
	mvnsne	ip, r5, asr #21
	beq	.Lad_s

	@ Compute exponent difference.  Make largest exponent in r4,
	@ corresponding arg in xh-xl, and positive exponent difference in r5.
	lsr  r4, r4, #21
	rsbs	r5, r4, r5, lsr #21
	it	lt
	rsblt	r5, r5, #0
	ble	1f
	add	r4, r4, r5
	eor	yl, xl, yl
	eor	yh, xh, yh
	eor	xl, yl, xl
	eor	xh, yh, xh
	eor	yl, xl, yl
	eor	yh, xh, yh
1:
	@ If exponent difference is too large, return largest argument
	@ already in xh-xl.  We need up to 54 bit to handle proper rounding
	@ of 0x1p54 - 1.1.
	cmp	r5, #54
	it	hi
	pophi {r4, r5, lr}
	bxhi lr
	
	@ Convert mantissa to signed integer.
	tst	xh, #0x80000000
	mov	xh, xh, lsl #12
	mov	ip, #0x00100000
	orr	xh, ip, xh, lsr #12
	beq	1f

	negs	xl, xl
	sbc	xh, xh, xh, lsl #1
1:
	tst	yh, #0x80000000
	mov	yh, yh, lsl #12
	orr	yh, ip, yh, lsr #12
	beq	1f

	negs	yl, yl
	sbc	yh, yh, yh, lsl #1
1:
	@ If exponent == difference, one or both args were denormalized.
	@ Since this is not common case, rescale them off line.
	teq	r4, r5
	beq	.Lad_d

.Lad_x:

	@ Compensate for the exponent overlapping the mantissa MSB added later
	sub	r4, r4, #1

	@ Shift yh-yl right per r5, add to xh-xl, keep leftover bits into ip.
	rsbs	lr, r5, #32
	blt	1f
	lsl  ip, yl, lr
	lsr yl,yl,r5
	adds xl, xl, yl
	adc	xh, xh, #0
	lsl yl, yh, lr
	adds xl, xl, yl
	asr yh, yh, r5
	adcs xh, xh, yh
	b	2f
1:	sub	r5, r5, #32
	add	lr, lr, #32
	cmp	yl, #1
	lsl ip, yh, lr
	it	cs
	orrcs	ip, ip, #2		@ 2 not 1, to allow lsr #1 later
	asr yh, yh, r5
	adds xl, xl, yh
	adcs	xh, xh, yh, asr #31
2:
	@ We now have a result in xh-xl-ip.
	@ Keep absolute value in xh-xl-ip, sign in r5 (the n bit was set above)
	and	r5, xh, #0x80000000
	bpl	.Lad_p

	mov	lr, #0
	negs	ip, ip
	sbcs	xl, lr, xl
	sbc	xh, lr, xh

	@ Determine how to normalize the result.
.Lad_p:
	cmp	xh, #0x00100000
	bcc	.Lad_a
	cmp	xh, #0x00200000
	bcc	.Lad_e

	@ Result needs to be shifted right.
	movs	xh, xh, lsr #1
	movs	xl, xl, rrx
	mov	ip, ip, rrx
	add	r4, r4, #1

	@ Make sure we did not bust our exponent.
	mov	r2, r4, lsl #21
	cmn	r2, #(2 << 21)
	bcs	.Lad_o

	@ Our result is now properly aligned into xh-xl, remaining bits in ip.
	@ Round with MSB of ip. If halfway between two numbers, round towards
	@ LSB of xl = 0.
	@ Pack final result together.
.Lad_e:
	cmp	ip, #0x80000000
	it	eq
	movseq	ip, xl, lsr #1
	adcs	xl, xl, #0
	adc	xh, xh, r4, lsl #20
	orr	xh, xh, r5
	pop {r4, r5, lr}
	bx lr

	@ Result must be shifted left and exponent adjusted.
.Lad_a:
	movs	ip, ip, lsl #1
	adcs	xl, xl, xl
	adc	xh, xh, xh
	subs	r4, r4, #1
	it	hs
	cmphs	xh, #0x00100000
	bhs	.Lad_e

	@ No rounding necessary since ip will always be 0 at this point.
.Lad_l:

	teq	xh, #0
	movne	r3, #20
	moveq	r3, #52
	moveq	xh, xl
	moveq	xl, #0
	mov	r2, xh
	cmp	r2, #(1 << 16)
	movhs	r2, r2, lsr #16
	subhs	r3, r3, #16
	cmp	r2, #(1 << 8)
	movhs	r2, r2, lsr #8
	subhs	r3, r3, #8
	cmp	r2, #(1 << 4)
	movhs	r2, r2, lsr #4
	subhs	r3, r3, #4
	cmp	r2, #(1 << 2)
	subhs	r3, r3, #2
	sublo	r3, r3, r2, lsr #1
	sub	r3, r3, r2, lsr #3

	@ determine how to shift the value.
	subs	r2, r3, #32
	bge	2f
	adds	r2, r2, #12
	ble	1f

	@ shift value left 21 to 31 bits, or actually right 11 to 1 bits
	@ since a register switch happened above.
	add	ip, r2, #20
	rsb	r2, r2, #12
	lsl  xl, xh, ip
	lsr  xh, xh, r2
	b	3f

	@ actually shift value left 1 to 20 bits, which might also represent
	@ 32 to 52 bits if counting the register switch that happened earlier.
1:	add	r2, r2, #20
2:	it	le
	rsble	ip, r2, #32
	lsl  xh, xh, r2

	lsr	ip, xl, ip
	itt	le
	orrle	xh, xh, ip
	lslle	xl, xl, r2

	@ adjust exponent accordingly.
3:	subs	r4, r4, r3
	ittt	ge
	addge	xh, xh, r4, lsl #20
	orrge	xh, xh, r5
	popge {r4, r5, lr}
	bxge lr

	@ Exponent too small, denormalize result.
	@ Find out proper shift value.
	mvn	r4, r4
	subs	r4, r4, #31
	bge	2f
	adds	r4, r4, #12
	bgt	1f

	@ shift result right of 1 to 20 bits, sign is in r5.
	add	r4, r4, #20
	rsb	r2, r4, #32
	lsr  xl, xl, r4
	lsl yh, xh, r2
	orr xl, xl, yh
	lsr yh, xh, r4
	orr xh, r5, yh
	pop {r4, r5, lr}
	bx lr

	@ shift result right of 21 to 31 bits, or left 11 to 1 bits after
	@ a register switch from xh to xl.
1:	rsb	r4, r4, #12
	rsb	r2, r4, #32
	lsr  xl, xl, r2
	lsl yh, xh, r4
	orr xl, xl, yh
	mov	xh, r5
	pop {r4, r5, lr}
	bx lr


	@ Shift value right of 32 to 64 bits, or 0 to 32 bits after a switch
	@ from xh to xl.
2:	lsr  xl, xh, r4
	mov	xh, r5
	pop {r4, r5, lr}
	bx lr


	@ Adjust exponents for denormalized arguments.
	@ Note that r4 must not remain equal to 0.
.Lad_d:
	teq	r4, #0
	eor	yh, yh, #0x00100000
	itte	eq
	eoreq	xh, xh, #0x00100000
	addeq	r4, r4, #1
	subne	r5, r5, #1
	b	.Lad_x


.Lad_s:
	mvns	ip, r4, asr #21
	it	ne
	mvnsne	ip, r5, asr #21
	beq	.Lad_i

	teq	r4, r5
	it	eq
	teqeq	xl, yl
	beq	1f

	@ Result is x + 0.0 = x or 0.0 + y = y.
	orrs	ip, r4, xl
	itt	eq
	moveq	xh, yh
	moveq	xl, yl
	pop {r4, r5, lr}
	bx lr


1:	teq	xh, yh

	@ Result is x - x = 0.
	ittt	ne
	movne	xh, #0
	movne	xl, #0
	popne {r4, r5, lr}
	bxne lr


	@ Result is x + x = 2x.
	movs	ip, r4, lsr #21
	bne	2f
	movs	xl, xl, lsl #1
	adcs	xh, xh, xh
	it	cs
	orrcs	xh, xh, #0x80000000
	pop {r4, r5, lr}
	bx lr

2:	adds	r4, r4, #(2 << 21)
	itt	cc
	addcc	xh, xh, #(1 << 20)
	popcc {r4, r5, lr}
	bxcc lr

	and	r5, xh, #0x80000000

	@ Overflow: return INF.
.Lad_o:
	orr	xh, r5, #0x7f000000
	orr	xh, xh, #0x00f00000
	mov	xl, #0
	pop {r4, r5, lr}
	bx lr


	@ At least one of x or y is INF/NAN.
	@   if xh-xl != INF/NAN: return yh-yl (which is INF/NAN)
	@   if yh-yl != INF/NAN: return xh-xl (which is INF/NAN)
	@   if either is NAN: return NAN
	@   if opposite sign: return NAN
	@   otherwise return xh-xl (which is INF or -INF)
.Lad_i:
	mvns	ip, r4, asr #21
	itte	ne
	movne	xh, yh
	movne	xl, yl
	mvnseq	ip, r5, asr #21
	itt	ne
	movne	yh, xh
	movne	yl, xl
	orrs	r4, xl, xh, lsl #12
	itte	eq
	orrseq	r5, yl, yh, lsl #12
	teqeq	xh, yh
	orrne	xh, xh, #0x00080000	@ quiet NAN
	pop {r4, r5, lr}
	bx lr



ARM_FUNC_START2 __aeabi_ui2d

	teq	r0, #0
	itt	eq
	moveq	r1, #0
	bxeq  lr

	push {r4, r5, lr}        @ sp -= 12

	mov	r4, #0x400		@ initial exponent
	add	r4, r4, #(52-1 - 1)
	mov	r5, #0			@ sign bit is 0
	.ifnc	xl, r0
	mov	xl, r0
	.endif
	mov	xh, #0
	b	.Lad_l


ARM_FUNC_START2 __aeabi_i2d

	teq	r0, #0
	itt	eq
	moveq	r1, #0
	bxeq lr

	push {r4, r5, lr}        @ sp -= 12

	mov	r4, #0x400		@ initial exponent
	add	r4, r4, #(52-1 - 1)
	ands	r5, r0, #0x80000000	@ sign bit in r5
	it	mi
	rsbmi	r0, r0, #0		@ absolute value
	.ifnc	xl, r0
	mov	xl, r0
	.endif
	mov	xh, #0
	b	.Lad_l


ARM_FUNC_START2 __aeabi_f2d

	movs	r2, r0, lsl #1		@ toss sign bit
	mov	xh, r2, asr #3		@ stretch exponent
	mov	xh, xh, rrx		@ retrieve sign bit
	mov	xl, r2, lsl #28		@ retrieve remaining bits
	itttt	ne
	andsne	r3, r2, #0xff000000	@ isolate exponent
	teqne	r3, #0xff000000		@ if not 0, check if INF or NAN
	eorne	xh, xh, #0x38000000	@ fixup exponent otherwise.
	bxne lr			@ and return it.

	bics	r2, r2, #0xff000000	@ isolate mantissa
	it	eq			@ if 0, that is ZERO or INF,
	bxeq lr			@ we are done already.

	teq	r3, #0xff000000		@ check for NAN
	itt	eq
	orreq	xh, xh, #0x00080000	@ change to quiet NAN
	bxeq lr			@ and return it.

	@ value was denormalized.  We can normalize it now.
	push	{r4, r5, lr}

	mov	r4, #0x380		@ setup corresponding exponent
	and	r5, xh, #0x80000000	@ move sign bit in r5
	bic	xh, xh, #0x80000000
	b	.Lad_l


ARM_FUNC_START2 __aeabi_ul2d

	orrs	r2, r0, r1
	it	eq
	bxeq lr

	push {r4, r5, lr}       @ sp -= 12

	mov	r5, #0
	b	2f

ARM_FUNC_START2 __aeabi_l2d

	orrs	r2, r0, r1
	it	eq
	bxeq lr

	push {r4, r5, lr}       @ sp -= 12

	ands	r5, ah, #0x80000000	@ sign bit in r5
	bpl	2f

	negs	al, al
	sbc	ah, ah, ah, lsl #1
2:
	mov	r4, #0x400		@ initial exponent
	add	r4, r4, #(52-1 - 1)

	@ If FP word order does not match integer word order, swap the words.
	.ifnc	xh, ah
	mov	ip, al
	mov	xh, ah
	mov	xl, ip
	.endif

	movs	ip, xh, lsr #22
	beq	.Lad_p

	@ The value is too big.  Scale it down a bit...
	mov	r2, #3
	movs	ip, ip, lsr #3
	it	ne
	addne	r2, r2, #3
	movs	ip, ip, lsr #3
	it	ne
	addne	r2, r2, #3
	add	r2, r2, ip, lsr #3

	rsb	r3, r2, #32
	lsl  ip, xl, r3
	lsr  xl, xl, r2
	lsl lr, xh, r3
	orr xl, xl, lr
	lsr  xh, xh, r2
	add	r4, r4, r2
	b	.Lad_p



ARM_FUNC_START2 __aeabi_dmul

	push {r4, r5, r6, lr}    @ sp -= 16

	@ Mask out exponents, trap any zero/denormal/INF/NAN.
	mov	ip, #0xff
	orr	ip, ip, #0x700
	ands	r4, ip, xh, lsr #20
	ittte	ne
	andsne	r5, ip, yh, lsr #20
	teqne	r4, ip
	teqne	r5, ip
	bleq	.Lml_s

	@ Add exponents together
	add	r4, r4, r5

	@ Determine final sign.
	eor	r6, xh, yh

	@ Convert mantissa to unsigned integer.
	@ If power of two, branch to a separate path.
	bic	xh, xh, ip, lsl #21
	bic	yh, yh, ip, lsl #21
	orrs	r5, xl, xh, lsl #12
	it	ne
	orrsne	r5, yl, yh, lsl #12
	orr	xh, xh, #0x00100000
	orr	yh, yh, #0x00100000
	beq	.Lml_1

	@ Here is the actual multiplication.
	@ This code works on architecture versions >= 4
	umull	ip, lr, xl, yl
	mov	r5, #0
	umlal	lr, r5, xh, yl
	and	yl, r6, #0x80000000
	umlal	lr, r5, xl, yh
	mov	r6, #0
	umlal	r5, r6, xh, yh

	@ The LSBs in ip are only significant for the final rounding.
	@ Fold them into lr.
	teq	ip, #0
	it	ne
	orrne	lr, lr, #1

	@ Adjust result upon the MSB position.
	sub	r4, r4, #0xff
	cmp	r6, #(1 << (20-11))
	sbc	r4, r4, #0x300
	bcs	1f
	movs	lr, lr, lsl #1
	adcs	r5, r5, r5
	adc	r6, r6, r6
1:
	@ Shift to final position, add sign to result.
	orr	xh, yl, r6, lsl #11
	orr	xh, xh, r5, lsr #21
	mov	xl, r5, lsl #11
	orr	xl, xl, lr, lsr #21
	mov	lr, lr, lsl #11

	@ Check exponent range for under/overflow.
	subs	ip, r4, #(254 - 1)
	it	hi
	cmphi	ip, #0x700
	bhi	.Lml_u

	@ Round the result, merge final exponent.
	cmp	lr, #0x80000000
	it	eq
	movseq	lr, xl, lsr #1
	adcs	xl, xl, #0
	adc	xh, xh, r4, lsl #20
	pop {r4, r5, r6, lr}
	bx lr


	@ Multiplication by 0x1p*: let''s shortcut a lot of code.
.Lml_1:
	and	r6, r6, #0x80000000
	orr	xh, r6, xh
	orr	xl, xl, yl
	eor	xh, xh, yh
	subs	r4, r4, ip, lsr #1
	ittt	gt
	rsbsgt	r5, r4, ip
	orrgt	xh, xh, r4, lsl #20
	popgt {r4, r5, r6, lr}
	bxgt lr


	@ Under/overflow: fix things up for the code below.
	orr	xh, xh, #0x00100000
	mov	lr, #0
	subs	r4, r4, #1
.Lml_u:
	@ Overflow?
	bgt	.Lml_o

	@ Check if denormalized result is possible, otherwise return signed 0.
	cmn	r4, #(53 + 1)
	ittt	le
	movle	xl, #0
	bicle	xh, xh, #0x7fffffff
	pople {r4, r5, r6, lr}
	bxle lr


	@ Find out proper shift value.
	rsb	r4, r4, #0
	subs	r4, r4, #32
	bge	2f
	adds	r4, r4, #12
	bgt	1f

	@ shift result right of 1 to 20 bits, preserve sign bit, round, etc.
	add	r4, r4, #20
	rsb	r5, r4, #32
	lsl  r3, xl, r5
	lsr  xl, xl, r4
	lsl r2, xh, r5
	orr xl, xl, r2
	and	r2, xh, #0x80000000
	bic	xh, xh, #0x80000000
	adds	xl, xl, r3, lsr #31
	lsr r6, xh, r4
	adc xh, r2, r6
	orrs	lr, lr, r3, lsl #1
	it	eq
	biceq	xl, xl, r3, lsr #31
	pop {r4, r5, r6, lr}
	bx lr


	@ shift result right of 21 to 31 bits, or left 11 to 1 bits after
	@ a register switch from xh to xl. Then round.
1:	rsb	r4, r4, #12
	rsb	r5, r4, #32
	lsl  r3, xl, r4
	lsr  xl, xl, r5
	lsl r2, xh, r4
	orr xl, xl, r2
	bic	xh, xh, #0x7fffffff
	adds	xl, xl, r3, lsr #31
	adc	xh, xh, #0
	orrs	lr, lr, r3, lsl #1
	it	eq
	biceq	xl, xl, r3, lsr #31
	pop {r4, r5, r6, lr}
	bx lr


	@ Shift value right of 32 to 64 bits, or 0 to 32 bits after a switch
	@ from xh to xl.  Leftover bits are in r3-r6-lr for rounding.
2:	rsb	r5, r4, #32
	lsl r2, xl, r5
	orr lr, lr, r2
	lsr  r3, xl, r4
	lsl r2, xh, r5
	orr r3, r3, r2
	lsr  xl, xh, r4
	bic	xh, xh, #0x7fffffff
	lsr r2, xh, r4
	bic xl, xl, r2
	add	xl, xl, r3, lsr #31
	orrs	lr, lr, r3, lsl #1
	it	eq
	biceq	xl, xl, r3, lsr #31
	pop {r4, r5, r6, lr}
	bx lr


	@ One or both arguments are denormalized.
	@ Scale them leftwards and preserve sign bit.
.Lml_d:
	teq	r4, #0
	bne	2f
	and	r6, xh, #0x80000000
1:	movs	xl, xl, lsl #1
	adc	xh, xh, xh
	tst	xh, #0x00100000
	it	eq
	subeq	r4, r4, #1
	beq	1b
	orr	xh, xh, r6
	teq	r5, #0
	it	ne
	bxne lr
2:	and	r6, yh, #0x80000000
3:	movs	yl, yl, lsl #1
	adc	yh, yh, yh
	tst	yh, #0x00100000
	it	eq
	subeq	r5, r5, #1
	beq	3b
	orr	yh, yh, r6
	bx lr

.Lml_s:
	@ Isolate the INF and NAN cases away
	teq	r4, ip
	and	r5, ip, yh, lsr #20
	it	ne
	teqne	r5, ip
	beq	1f

	@ Here, one or more arguments are either denormalized or zero.
	orrs	r6, xl, xh, lsl #1
	it	ne
	orrsne	r6, yl, yh, lsl #1
	bne	.Lml_d

	@ Result is 0, but determine sign anyway.
.Lml_z:
	eor	xh, xh, yh
	and	xh, xh, #0x80000000
	mov	xl, #0
	pop {r4, r5, r6, lr}
	bx lr


1:	@ One or both args are INF or NAN.
	orrs	r6, xl, xh, lsl #1
	itte	eq
	moveq	xl, yl
	moveq	xh, yh
	orrsne	r6, yl, yh, lsl #1
	beq	.Lml_n		@ 0 * INF or INF * 0 -> NAN
	teq	r4, ip
	bne	1f
	orrs	r6, xl, xh, lsl #12
	bne	.Lml_n		@ NAN * <anything> -> NAN
1:	teq	r5, ip
	bne	.Lml_i
	orrs	r6, yl, yh, lsl #12
	itt	ne
	movne	xl, yl
	movne	xh, yh
	bne	.Lml_n		@ <anything> * NAN -> NAN

	@ Result is INF, but we need to determine its sign.
.Lml_i:
	eor	xh, xh, yh

	@ Overflow: return INF (sign already in xh).
.Lml_o:
	and	xh, xh, #0x80000000
	orr	xh, xh, #0x7f000000
	orr	xh, xh, #0x00f00000
	mov	xl, #0
	pop {r4, r5, r6, lr}
	bx lr


	@ Return a quiet NAN.
.Lml_n:
	orr	xh, xh, #0x7f000000
	orr	xh, xh, #0x00f80000
	pop {r4, r5, r6, lr}
	bx lr



ARM_FUNC_START2 __aeabi_ddiv
	push	{r4, r5, r6, lr}

	@ Mask out exponents, trap any zero/denormal/INF/NAN.
	mov	ip, #0xff
	orr	ip, ip, #0x700
	ands	r4, ip, xh, lsr #20
	ittte	ne
	andsne	r5, ip, yh, lsr #20
	teqne	r4, ip
	teqne	r5, ip
	bleq	.Ldv_s

	@ Subtract divisor exponent from dividend''s.
	sub	r4, r4, r5

	@ Preserve final sign into lr.
	eor	lr, xh, yh

	@ Convert mantissa to unsigned integer.
	@ Dividend -> r5-r6, divisor -> yh-yl.
	orrs	r5, yl, yh, lsl #12
	mov	xh, xh, lsl #12
	beq	.Ldv_1
	mov	yh, yh, lsl #12
	mov	r5, #0x10000000
	orr	yh, r5, yh, lsr #4
	orr	yh, yh, yl, lsr #24
	mov	yl, yl, lsl #8
	orr	r5, r5, xh, lsr #4
	orr	r5, r5, xl, lsr #24
	mov	r6, xl, lsl #8

	@ Initialize xh with final sign bit.
	and	xh, lr, #0x80000000

	@ Ensure result will land to known bit position.
	@ Apply exponent bias accordingly.
	cmp	r5, yh
	it	eq
	cmpeq	r6, yl
	adc	r4, r4, #(255 - 2)
	add	r4, r4, #0x300
	bcs	1f
	movs	yh, yh, lsr #1
	mov	yl, yl, rrx
1:
	@ Perform first subtraction to align result to a nibble.
	subs	r6, r6, yl
	sbc	r5, r5, yh
	movs	yh, yh, lsr #1
	mov	yl, yl, rrx
	mov	xl, #0x00100000
	mov	ip, #0x00080000

	@ The actual division loop.
1:	subs	lr, r6, yl
	sbcs	lr, r5, yh
	ittt	cs
	subcs	r6, r6, yl
	movcs	r5, lr
	orrcs	xl, xl, ip
	movs	yh, yh, lsr #1
	mov	yl, yl, rrx
	subs	lr, r6, yl
	sbcs	lr, r5, yh
	ittt	cs
	subcs	r6, r6, yl
	movcs	r5, lr
	orrcs	xl, xl, ip, lsr #1
	movs	yh, yh, lsr #1
	mov	yl, yl, rrx
	subs	lr, r6, yl
	sbcs	lr, r5, yh
	ittt	cs
	subcs	r6, r6, yl
	movcs	r5, lr
	orrcs	xl, xl, ip, lsr #2
	movs	yh, yh, lsr #1
	mov	yl, yl, rrx
	subs	lr, r6, yl
	sbcs	lr, r5, yh
	ittt	cs
	subcs	r6, r6, yl
	movcs	r5, lr
	orrcs	xl, xl, ip, lsr #3

	orrs	lr, r5, r6
	beq	2f
	mov	r5, r5, lsl #4
	orr	r5, r5, r6, lsr #28
	mov	r6, r6, lsl #4
	mov	yh, yh, lsl #3
	orr	yh, yh, yl, lsr #29
	mov	yl, yl, lsl #3
	movs	ip, ip, lsr #4
	bne	1b

	@ We are done with a word of the result.
	@ Loop again for the low word if this pass was for the high word.
	tst	xh, #0x00100000
	bne	3f
	orr	xh, xh, xl
	mov	xl, #0
	mov	ip, #0x80000000
	b	1b
2:
	@ Be sure result starts in the high word.
	tst	xh, #0x00100000
	itt	eq
	orreq	xh, xh, xl
	moveq	xl, #0
3:
	@ Check exponent range for under/overflow.
	subs	ip, r4, #(254 - 1)
	it	hi
	cmphi	ip, #0x700
	bhi	.Lml_u

	@ Round the result, merge final exponent.
	subs	ip, r5, yh
	itt	eq
	subseq	ip, r6, yl
	movseq	ip, xl, lsr #1
	adcs	xl, xl, #0
	adc	xh, xh, r4, lsl #20
	pop {r4, r5, r6, lr}
	bx lr


	@ Division by 0x1p*: shortcut a lot of code.
.Ldv_1:
	and	lr, lr, #0x80000000
	orr	xh, lr, xh, lsr #12
	adds	r4, r4, ip, lsr #1
	ittt	gt
	rsbsgt	r5, r4, ip
	orrgt	xh, xh, r4, lsl #20
	popgt {r4, r5, r6, lr}
	bxgt lr


	orr	xh, xh, #0x00100000
	mov	lr, #0
	subs	r4, r4, #1
	b	.Lml_u

	@ Result mightt need to be denormalized: put remainder bits
	@ in lr for rounding considerations.
.Ldv_u:
	orr	lr, r5, r6
	b	.Lml_u

	@ One or both arguments is either INF, NAN or zero.
.Ldv_s:
	and	r5, ip, yh, lsr #20
	teq	r4, ip
	it	eq
	teqeq	r5, ip
	beq	.Lml_n		@ INF/NAN / INF/NAN -> NAN
	teq	r4, ip
	bne	1f
	orrs	r4, xl, xh, lsl #12
	bne	.Lml_n		@ NAN / <anything> -> NAN
	teq	r5, ip
	bne	.Lml_i		@ INF / <anything> -> INF
	mov	xl, yl
	mov	xh, yh
	b	.Lml_n		@ INF / (INF or NAN) -> NAN
1:	teq	r5, ip
	bne	2f
	orrs	r5, yl, yh, lsl #12
	beq	.Lml_z		@ <anything> / INF -> 0
	mov	xl, yl
	mov	xh, yh
	b	.Lml_n		@ <anything> / NAN -> NAN
2:	@ If both are nonzero, we need to normalize and resume above.
	orrs	r6, xl, xh, lsl #1
	it	ne
	orrsne	r6, yl, yh, lsl #1
	bne	.Lml_d
	@ One or both arguments are 0.
	orrs	r4, xl, xh, lsl #1
	bne	.Lml_i		@ <non_zero> / 0 -> INF
	orrs	r5, yl, yh, lsl #1
	bne	.Lml_z		@ 0 / <non_zero> -> 0
	b	.Lml_n		@ 0 / 0 -> NAN


@ Note: only r0 (return value) and ip are clobbered here.

ARM_FUNC_START2 __gtdf2
ARM_FUNC_START2 __gedf2
	mov	ip, #-1
	b	1f

ARM_FUNC_START2 __ltdf2
ARM_FUNC_START2 __ledf2
	mov	ip, #1
	b	1f

ARM_FUNC_START2 __cmpdf2
ARM_FUNC_START2 __nedf2
ARM_FUNC_START2 __eqdf2
	mov	ip, #1			@ how should we specify unordered here?

1:	str	ip, [sp, #-4]!

	@ Trap any INF/NAN first.
	mov	ip, xh, lsl #1
	mvns	ip, ip, asr #21
	mov	ip, yh, lsl #1
	it	ne
	mvnsne	ip, ip, asr #21
	beq	3f

	@ Test for equality.  Note that 0.0 is equal to -0.0.
2:	add	sp, sp, #4

	orrs	ip, xl, xh, lsl #1	@ if x == 0.0 or -0.0
	ite	eq
	orrseq	ip, yl, yh, lsl #1	@ and y == 0.0 or -0.0
	teqne	xh, yh			@ or xh == yh
	ittt	eq
	teqeq	xl, yl			@ and xl == yl
	moveq	r0, #0			@ then equal.
	bxeq lr

	@ Clear C flag
	cmn	r0, #0

	@ Compare sign, 
	teq	xh, yh

	@ Compare values if same sign
	it	pl
	cmppl	xh, yh
	it	eq
	cmpeq	xl, yl

	@ Result:
	ite	cs
	movcs	r0, yh, asr #31
	mvncc	r0, yh, asr #31
	orr	r0, r0, #1
	bx lr

3:  @ Look for a NAN.

	mov ip, xh, lsl #1
	mvns	ip, ip, asr #21
	bne	4f
	orrs	ip, xl, xh, lsl #12
	bne	5f			@ x is NAN
4:	mov	ip, yh, lsl #1
	mvns	ip, ip, asr #21
	bne	2b
	orrs	ip, yl, yh, lsl #12
	beq	2b			@ y is not NAN

5:	ldr	r0, [sp], #4		@ unordered return code

	bx lr

ARM_FUNC_START2 __aeabi_cdrcmple
	mov	ip, r0
	mov	r0, r2
	mov	r2, ip
	mov	ip, r1
	mov	r1, r3
	mov	r3, ip
	b	6f

ARM_FUNC_START2 __aeabi_cdcmpeq
ARM_FUNC_START2 __aeabi_cdcmple

	@ The status-returning routines are required to preserve all
	@ registers except ip, lr, and cpsr.
6:	push	{r0, lr}

	bl __cmpdf2
	@ Set the Z flag correctly, and the C flag unconditionally.
	cmp	r0, #0
	@ Clear the C flag if the return value was -1, indicating
	@ that the first operand was smaller than the second.
	it	mi
	cmnmi	r0, #0

	pop {r0, lr}
	bx lr

	
ARM_FUNC_START2 __aeabi_dcmpeq
	str lr, [sp, #-8]!        @ sp -= 8

	bl __aeabi_cdcmple
	ite	eq
	moveq	r0, #1	@ Equal to.
	movne	r0, #0	@ Less than, greater than, or unordered.

	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START2 __aeabi_dcmplt
	str lr, [sp, #-8]!        @ sp -= 8

	bl __aeabi_cdcmple
	ite	cc
	movcc	r0, #1	@ Less than.
	movcs	r0, #0	@ Equal to, greater than, or unordered.
	ldr lr, [sp], #8
	bx lr


ARM_FUNC_START2 __aeabi_dcmple
	str lr, [sp, #-8]!        @ sp -= 8

	bl __aeabi_cdcmple
	ite	ls
	movls	r0, #1  @ Less than or equal to.
	movhi	r0, #0	@ Greater than or unordered.
	ldr lr, [sp], #8
	bx lr



ARM_FUNC_START2 __aeabi_dcmpge
	str lr, [sp, #-8]!        @ sp -= 8

	bl __aeabi_cdrcmple
	ite	ls
	movls	r0, #1	@ Operand 2 is less than or equal to operand 1.
	movhi	r0, #0	@ Operand 2 greater than operand 1, or unordered.
	ldr lr, [sp], #8
	bx lr



ARM_FUNC_START2 __aeabi_dcmpgt
	str lr, [sp, #-8]!        @ sp -= 8

	bl __aeabi_cdrcmple
	ite	cc
	movcc	r0, #1	@ Operand 2 is less than operand 1.
	movcs	r0, #0  @ Operand 2 is greater than or equal to operand 1,
			@ or they are unordered.
	ldr lr, [sp], #8
	bx lr



ARM_FUNC_START2 __aeabi_dcmpun
	mov	ip, xh, lsl #1
	mvns	ip, ip, asr #21
	bne	1f
	orrs	ip, xl, xh, lsl #12
	bne	3f			@ x is NAN
1:	mov	ip, yh, lsl #1
	mvns	ip, ip, asr #21
	bne	2f
	orrs	ip, yl, yh, lsl #12
	bne	3f			@ y is NAN
2:	mov	r0, #0			@ arguments are ordered.
	bx lr

3:	mov	r0, #1			@ arguments are unordered.
	bx lr



ARM_FUNC_START2 __aeabi_d2iz
	@ check exponent range.
	mov	r2, xh, lsl #1
	adds	r2, r2, #(1 << 21)
	bcs	2f			@ value is INF or NAN
	bpl	1f			@ value is too small
	mov	r3, #(0xfffffc00 + 31)
	subs	r2, r3, r2, asr #21
	bls	3f			@ value is too large

	@ scale value
	mov	r3, xh, lsl #11
	orr	r3, r3, #0x80000000
	orr	r3, r3, xl, lsr #21
	tst	xh, #0x80000000		@ the sign bit
	lsr  r0, r3, r2
	it	ne
	rsbne	r0, r0, #0
	bx lr

1:	mov	r0, #0
	bx lr

2:	orrs	xl, xl, xh, lsl #12
	bne	4f			@ x is NAN.
3:	ands	r0, xh, #0x80000000	@ the sign bit
	it	eq
	moveq	r0, #0x7fffffff		@ maximum signed positive si
	bx lr

4:	mov	r0, #0			@ How should we convert NAN?
	bx lr



ARM_FUNC_START2 __aeabi_d2uiz
	@ check exponent range.
	movs	r2, xh, lsl #1
	bcs	1f			@ value is negative
	adds	r2, r2, #(1 << 21)
	bcs	2f			@ value is INF or NAN
	bpl	1f			@ value is too small
	mov	r3, #(0xfffffc00 + 31)
	subs	r2, r3, r2, asr #21
	bmi	3f			@ value is too large

	@ scale value
	mov	r3, xh, lsl #11
	orr	r3, r3, #0x80000000
	orr	r3, r3, xl, lsr #21
	lsr  r0, r3, r2
	bx lr

1:	mov	r0, #0
	bx lr

2:	orrs	xl, xl, xh, lsl #12
	bne	4f			@ value is NAN.
3:	mov	r0, #0xffffffff		@ maximum unsigned si
	bx lr

4:	mov	r0, #0			@ How should we convert NAN?
	bx lr



ARM_FUNC_START2 __aeabi_d2f
	@ check exponent range.
	mov	r2, xh, lsl #1
	subs	r3, r2, #((1023 - 127) << 21)
	itt	cs
	subscs	ip, r3, #(1 << 21)
	rsbscs	ip, ip, #(254 << 21)
	bls	2f			@ value is out of range

1:	@ shift and round mantissa
	and	ip, xh, #0x80000000
	mov	r2, xl, lsl #3
	orr	xl, ip, xl, lsr #29
	cmp	r2, #0x80000000
	adc	r0, xl, r3, lsl #2
	it	eq
	biceq	r0, r0, #1
	bx lr

2:	@ either overflow or underflow
	tst	xh, #0x40000000
	bne	3f			@ overflow

	@ check if denormalized value is possible
	adds	r2, r3, #(23 << 21)
	itt	lt
	andlt	r0, xh, #0x80000000	@ too small, return signed 0.
	bxlt lr

	@ denormalize value so we can resume with the code above afterwards.
	orr	xh, xh, #0x00100000
	mov	r2, r2, lsr #21
	rsb	r2, r2, #24
	rsb	ip, r2, #32

	lsls	r3, xl, ip
	
	lsr  xl, xl, r2
	it	ne
	orrne	xl, xl, #1		@ fold r3 for rounding considerations. 
	mov	r3, xh, lsl #11
	mov	r3, r3, lsr #11
	lsl ip, r3, ip
	orr xl, xl, ip
	lsr r3, r3, r2
	mov	r3, r3, lsl #1
	b	1b

3:	@ chech for NAN
	mvns	r3, r2, asr #21
	bne	5f			@ simple overflow
	orrs	r3, xl, xh, lsl #12
	ittt	ne
	movne	r0, #0x7f000000
	orrne	r0, r0, #0x00c00000
	bxne lr			@ return NAN 

5:	@ return INF with sign
	and	r0, xh, #0x80000000
	orr	r0, r0, #0x7f000000
	orr	r0, r0, #0x00800000
	bx lr


ARM_FUNC_START2 __aeabi_d2lz
	push {lr}
	bl	__fixdfdi
	pop {lr}
	bx lr
		
ARM_FUNC_START2 __aeabi_d2ulz
	push {lr}
	bl	__fixunsdfdi
	pop {lr}
	bx lr

ARM_FUNC_START2 __aeabi_f2lz
	push {lr}
	bl	__fixsfdi
	pop {lr}
	bx lr
		
ARM_FUNC_START2 __aeabi_f2ulz	
	push {lr}
	bl	__fixunssfdi
	pop {lr}
	bx lr
		
ARM_FUNC_START2 __aeabi_d2h	
	push {lr}
	bl	__gnu_d2h_ieee
	pop {lr}
	bx lr
		
ARM_FUNC_START2 __aeabi_d2h_alt
	push {lr}
	bl	__gnu_d2h_alternative
	pop {lr}
	bx lr


