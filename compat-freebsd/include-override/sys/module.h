/* -*- Mode: C -*- */

#pragma once

#include_next <sys/module.h>

#undef DRIVER_MODULE
#define DRIVER_MODULE(name, busname, driver, devclass, evh, arg) \
  void *__driver_module = &(driver)

/* EOF */
