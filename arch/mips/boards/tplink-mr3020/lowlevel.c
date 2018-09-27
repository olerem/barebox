#include <pbl.h>

void start_tplink_mr3020(void)
{
	pbl_barebox_uncompress((void*)TEXT_BASE, 0x0, 0xdeadbeef);
}