#include <asm/asm.h>

	.section .data
EXPORT(input_data)
        .incbin "images/barebox.z"
EXPORT(input_data_end)
