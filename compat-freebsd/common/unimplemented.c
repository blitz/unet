/* -*- Mode: C -*- */

#include <sys/types.h>

#define WEAK __attribute__((weak))

int puts(const char *);
void abort();

#define FN_UNIMPL(name)                                                 \
  void name() WEAK;                                                     \
  void name() { puts("UNIMPLEMENTED: " #name); __unimplemented(); }

void __unimplemented() { abort(); }

#include "unimplemented.inc"

WEAK int ticks;
WEAK int hz;
WEAK int bootverbose = 1;

WEAK uintptr_t *__start_set_pcpu;
WEAK uintptr_t *__stop_set_pcpu;

/* Actually uma_zone_t */
WEAK void *zone_mbuf;
WEAK void *zone_jumbop;
WEAK void *zone_jumbo9;
WEAK void *zone_jumbo16;
WEAK void *zone_pack;
WEAK void *zone_clust;

/* Actually malloc_type */
WEAK void *M_DEVBUF;

/* Unimplemented function pointers */
WEAK void (* vlan_trunk_cap_p)() = __unimplemented;
WEAK void (*tbr_dequeue_ptr)()   = __unimplemented;

/* Taskqueue magic */
WEAK void *taskqueue_fast;

/* Where does this come from? */
WEAK void *sysctl__hw_children;

/* EOF */
