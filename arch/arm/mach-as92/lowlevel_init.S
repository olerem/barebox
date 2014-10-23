/*
 * Copyright (C) 2014 Du Huanpeng <u74147@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <sizes.h>
#include <mach/asap-regs.h>
#include <asm/barebox-arm-head.h>


.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
@	bl arm_cpu_lowlevel_init

/*	set SP to SRAM */
	mov	sp, #0x40000000
	add	sp, sp, #0x1000
	sub	sp, sp, #16

	

	mov	r0, #'F'
	bl	putc
	
	ldr	r0, =barebox
	bl	__puts

	mov	r0, #'O'
	bl	putc
	bl	out


/* putc: send char to uart4. */
putc:
	push	{r1}

1:	ldr	r1, reg_base
	ldr	r1, [r1, #96]
	tst	r1, #134217728
	beq	1b

	ldr	r1, reg_base
	str	r0, [r1, #80]
	
	pop	{r1}
	mov	pc, lr

reg_base:
	.word	0x80010000
/* end of putc. */

/* puts: send string to uart4. */
.globl __puts
__puts:
@	push {r1, lr}
	mov	r2, lr

	mov	r1, r0
1:	ldrb	r0, [r1]
	cmp	r0, #0
	beq	88f
	mov	r3, r1
	bl	putc
	mov	r1, r3
	add	r1, r1, #1
	b	1b
88:	@pop {r1, lr}
	mov	pc, r2
	mov	pc, lr
barebox:
	.word	0x61626364
	.word	0x65666768

/* end of __puts. */

out:

	ldr	r0, =(0x22000000-(CONFIG_STACK_SIZE+CONFIG_MALLOC_SIZE))
	ldr	r1, =(CONFIG_STACK_SIZE+CONFIG_MALLOC_SIZE)
	mov	r2, lr
	b	barebox_arm_entry


