/* -*- Mode: C -*- */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>

#include <dlfcn.h>


typedef unsigned int u_int;
#include "include/sys/kobj.h"

static void *resolve_or_die(void *lib, const char *name)
{
  void *sym = dlsym(lib, name);
  if (not sym) {
    fprintf(stderr, "Symbol '%s' could not be resolved.\n", name);
    exit(1);
  }
  return sym;
}

enum { PROBE = 0, ATTACH, DETACH, SHUTDOWN, SUSPEND, RESUME };

struct methods {
  int method_id;
  const char *name;
  void       *desc;
  int        (*fn)(void *);
} driver_methods[] = {
  { .method_id = PROBE,    .name = "device_probe"    },
  { .method_id = ATTACH,   .name = "device_attach"   },
  { .method_id = DETACH,   .name = "device_detach"   },
  { .method_id = SHUTDOWN, .name = "device_shutdown" },
  { .method_id = SUSPEND,  .name = "device_suspend"  },
  { .method_id = RESUME,   .name = "device_resume"   },
};

#define KNOWN_METHODS (sizeof(driver_methods)/sizeof(driver_methods[0]))

extern void *e1000_pci_example;
extern const char *device_get_desc(void *dev);

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: kmod-info module.so\n");
    return 1;
  }

  void *lib = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
  if (not lib) {
    fprintf(stderr, "dlopen failed\n");
    return 1;
  }

  kobj_class_t *driver_struct = resolve_or_die(lib, "__driver_module");
  printf("Loaded FreeBSD module '%s'.\n", (*driver_struct)->name);

  printf("Resolving device driver methods:\n");
  for (unsigned i = 0; i < KNOWN_METHODS; i++) {
    char name[64];
    assert(driver_methods[i].method_id == i);
    name[63] = 0;
    snprintf(name, sizeof(name)-1, "%s_desc", driver_methods[i].name);
    driver_methods[i].desc = resolve_or_die(lib, name);
    printf("\t%-16s %p\n", driver_methods[i].name, driver_methods[i].desc);
  }

  printf("Parsing device driver kernel object:\n");
  for (kobj_method_t *cur = (*driver_struct)->methods; cur->desc != NULL; cur++) {
    printf("\tmethod %p: ", cur->func);
    unsigned i;
    for (i = 0; i < KNOWN_METHODS; i++) {
      if (driver_methods[i].desc == cur->desc) {
        printf("%s", driver_methods[i].name);
        driver_methods[i].fn = cur->func;
        break;
      }
    }
    printf("%s\n", i == KNOWN_METHODS ? "<unknown>" : "");
  }
  printf("\n");

  if (driver_methods[PROBE].fn) {
    printf("Calling probe...\n");
    int res = driver_methods[PROBE].fn(e1000_pci_example);
    if (res == -20) {
      printf("Driver successfully probed mockup device!\n", res);
      printf("Device detected as '%s'.\n", device_get_desc(e1000_pci_example));
    } else {
      printf("Something went wrong? Return value is %d.\n", res);
    }
  } else {
    printf("No probe method found?\n");
  }

  return 0;
}

/* EOF */
