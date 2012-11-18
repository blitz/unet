/* -*- Mode: C -*- */

#include <stdio.h>
#include <iso646.h>

#include <dlfcn.h>


int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: kmod-info module.so\n");
    return 1;
  }

  void *lib = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
  printf("dlopen -> %p\n", lib);
  if (not lib) {
    perror("dlopen");
    return 1;
  }

  return 0;
}

/* EOF */
