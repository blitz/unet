#pragma once

#include_next <sys/sysctl.h>

#undef SYSCTL_ADD_PROC
#define SYSCTL_ADD_PROC(...)
