#ifndef __MACH_ARM_HEAD_H
#define __MACH_ARM_HEAD_H

static inline void barebox_arm_head(void)
{

#ifdef CONFIG_THUMB2_BAREBOX
#error Thumb2 is not supported
#endif

	__asm__ __volatile__ (
		"b barebox_arm_reset_vector\n"
		"b .\n"
		"b .\n"
		"b .\n"
		"b .\n"
		".word _barebox_image_size\n"
		"b .\n"
		"b .\n"
		".asciz \"barebox\"\n"
		".word _text\n"
		".word _barebox_image_size\n"
	);
}

#endif /* __ASM_ARM_HEAD_H */
