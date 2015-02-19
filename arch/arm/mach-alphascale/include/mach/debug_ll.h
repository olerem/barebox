/*
 * Copyright (C) 2014, Du Huanpeng <u74147@gmail.com>
 *
 * Under GPLv2
 */

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <asm/io.h>
#include <mach/hardware.h>



/* from Alphascale sysloder */
#define UART4_BASEESS	0x80010000
#define HW_UART4_STAT	(UART4_BASEESS + 0x60)
#define HW_UART4_DATA	(UART4_BASEESS + 0x50)


/*
 * The following code assumes the serial port has already been
 * initialized by the bootloader.  If you didn't setup a port in
 * your bootloader then nothing will appear (which might be desired).
 *
 * This does not append a newline
 */
static inline void PUTC_LL(char c)
{

	while (!(readl(HW_UART4_STAT) & 0x08000000));
	writel(c, HW_UART4_DATA);
	return;
}

#endif
