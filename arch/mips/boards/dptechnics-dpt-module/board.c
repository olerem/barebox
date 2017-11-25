#include <common.h>
#include <init.h>

static int dummy_init(void)
{
	return 0;
}
postcore_initcall(dummy_init);
