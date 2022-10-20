@ libgcc routines for ARM cpu.
@ Division routines, written by Richard Earnshaw, (rearnsha@armltd.co.uk)

/* Copyright (C) 1995-2020 Free Software Foundation, Inc.

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

#define xl r0
#define xh r1
#define yl r2
#define yh r3
#define xxh r0
#define xxl r1
#define yyh r2
#define yyl r3
#define al r0
#define ah r1


.macro ARM_FUNC_START name
	.thumb_func
	.globl \name
	.align 0
\name:
.endm


/* ------------------------------------------------------------------------ */
/*		Bodies of the division and modulo routines.		    */
/* ------------------------------------------------------------------------ */

.macro ARM_DIV_BODY dividend, divisor, result, curbit
	clz	\curbit, \dividend
	clz	\result, \divisor
	sub	\curbit, \result, \curbit
	rsb	\curbit, \curbit, #31
	adr	\result, 1f
	add	\curbit, \result, \curbit, lsl #4
	mov	\result, #0
	mov	pc, \curbit
.p2align 3
1:
	.set	shift, 32
	.rept	32
	.set	shift, shift - 1
	cmp.w	\dividend, \divisor, lsl #shift
	nop.n
	adc.w	\result, \result, \result
	it	cs
	subcs.w	\dividend, \dividend, \divisor, lsl #shift
	.endr
.endm

/* ------------------------------------------------------------------------ */
/*		Start of the Real Functions				    */
/* ------------------------------------------------------------------------ */
ARM_FUNC_START __aeabi_uidiv
	/* Note: if called via udivsi3_skip_div0_test, this will unnecessarily
	   check for division-by-zero a second time.  */
.udivsi3_skip_div0_test:
	subs	r2, r1, #1
	it	eq
	bxeq lr
	bcc	.udiv_Ldiv0
	cmp	r0, r1
	bls	11f
	tst	r1, r2
	beq	12f
	
	ARM_DIV_BODY r0, r1, r2, r3
	
	mov	r0, r2
	bx lr	

11:	ite	eq
	moveq	r0, #1
	movne	r0, #0
	bx lr

12:	clz r2, r1
	rsb r2, r2, #31

	mov	r0, r0, lsr r2
	bx lr

.udiv_Ldiv0:
	cbz r0, 1f
	mov r0, #0xffffffff
1:  b.w __aeabi_idiv0


ARM_FUNC_START __aeabi_uidivmod
	cmp	r1, #0
	beq	.umod_Ldiv0
	stmfd	sp!, { r0, r1, lr }
	bl	.udivsi3_skip_div0_test
	ldmfd	sp!, { r1, r2, lr }
	mul	r3, r2, r0
	sub	r1, r1, r3
	bx lr
	
.umod_Ldiv0:
	cbz r0, 1f
	mov r0, #0xffffffff
1:  b.w __aeabi_idiv0



ARM_FUNC_START __aeabi_idiv
	cmp	r1, #0
	beq	.idiv_Ldiv0
.divsi3_skip_div0_test:
	eor	ip, r0, r1			@ save the sign of the result.
	it	mi
	rsbmi	r1, r1, #0			@ loops below use unsigned.
	subs	r2, r1, #1			@ division by 1 or -1 ?
	beq	10f
	movs	r3, r0
	it	mi
	rsbmi	r3, r0, #0			@ positive dividend value
	cmp	r3, r1
	bls	11f
	tst	r1, r2				@ divisor is power of 2 ?
	beq	12f

	ARM_DIV_BODY r3, r1, r0, r2
	
	cmp	ip, #0
	it	mi
	rsbmi	r0, r0, #0
	bx lr	

10:	teq	ip, r0				@ same sign ?
	it	mi
	rsbmi	r0, r0, #0
	bx lr	

11:	it	lo
	movlo	r0, #0
	itt	eq
	moveq	r0, ip, asr #31
	orreq	r0, r0, #1
	bx lr

12:	clz r2, r1
	rsb r2, r2, #31

	cmp	ip, #0
	mov	r0, r3, lsr r2
	it	mi
	rsbmi	r0, r0, #0
	bx lr

	
.idiv_Ldiv0:
	cbz r0, 1f
	mov r0, #0xffffffff
1:  b.w __aeabi_idiv0


ARM_FUNC_START __aeabi_idivmod
	cmp	r1, #0
	beq	.mod_Ldiv0
	stmfd	sp!, { r0, r1, lr }
	bl	.divsi3_skip_div0_test
	ldmfd	sp!, { r1, r2, lr }
	mul	r3, r2, r0
	sub	r1, r1, r3
	bx lr
	
.mod_Ldiv0:
	cbz r0, 1f
	mov r0, #0xffffffff
1:  b.w __aeabi_idiv0


ARM_FUNC_START __aeabi_idiv0
ARM_FUNC_START __aeabi_ldiv0
	bx lr
	
/* Dword shift operations.  */
/* All the following Dword shift variants rely on the fact that
	shft xxx, Reg
   is in fact done as
	shft xxx, (Reg & 255)
   so for Reg value in (32...63) and (-1...-31) we will get zero (in the
   case of logical shifts) or the sign (for asr).  */

ARM_FUNC_START __aeabi_llsr
	lsrs	al, r2
	movs	r3, ah
	lsrs	ah, r2
	mov	ip, r3
	subs	r2, #32
	lsrs	r3, r2
	orrs	al, r3
	negs	r2, r2
	mov	r3, ip
	lsls	r3, r2
	orrs	al, r3
	bx lr

	
ARM_FUNC_START __aeabi_lasr
	lsrs	al, r2
	movs	r3, ah
	asrs	ah, r2
	subs	r2, #32
	@ If r2 is negative at this point the following step would OR
	@ the sign bit into all of AL.  That's not what we want...
	bmi	1f
	mov	ip, r3
	asrs	r3, r2
	orrs	al, r3
	mov	r3, ip
1:
	negs	r2, r2
	lsls	r3, r2
	orrs	al, r3
	bx lr



ARM_FUNC_START __aeabi_llsl
	lsls	ah, r2
	movs	r3, al
	lsls	al, r2
	mov	ip, r3
	subs	r2, #32
	lsls	r3, r2
	orrs	ah, r3
	negs	r2, r2
	mov	r3, ip
	lsrs	r3, r2
	orrs	ah, r3
	bx lr


