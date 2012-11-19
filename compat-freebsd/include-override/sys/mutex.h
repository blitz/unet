#pragma once

#include <sys/_mutex.h>


#define	MTX_DEF		0x00000000	/* DEFAULT (sleep) lock */ 

#define mtx_assert(x, y)
#define mtx_lock(x)
#define mtx_unlock(x)
#define mtx_destroy(x)
#define mtx_init(a, b, c, d)
