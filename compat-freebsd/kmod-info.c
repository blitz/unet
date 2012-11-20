/* -*- Mode: C -*- */

/* I hate glibc */
#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <iso646.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dlfcn.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/vfio.h>

#include "pci-interface.h"

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

int main(int argc, char **argv)
{
  if (argc != 4) {
    fprintf(stderr, "Usage: kmod-info iommu-group pciid module.so\n");
    return 1;
  }

  /****************** OPEN VFIO */
  struct vfio_group_status group_status =
    { .argsz = sizeof(group_status) };
  struct vfio_iommu_type1_dma_map dma_map = { .argsz = sizeof(dma_map) };
  struct vfio_device_info device_info = { .argsz = sizeof(device_info) };

  int container = open("/dev/vfio/vfio", O_RDWR);
  if (container < 0) { perror("open"); return EXIT_FAILURE; }

  int version;
  if ((version = ioctl(container, VFIO_GET_API_VERSION)) != VFIO_API_VERSION) {
    fprintf(stderr, "VFIO API version mismatch. (%d vs %d)\n",
            version, VFIO_API_VERSION);
    return EXIT_FAILURE;
  }

  if (!ioctl(container, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
    fprintf(stderr, "VFIO does not understand TYPE1_IOMMU\n");
    return EXIT_FAILURE;
  }

  int group = open(argv[1], O_RDWR);
  if (group < 0) { perror("open"); return EXIT_FAILURE; }

  ioctl(group, VFIO_GROUP_GET_STATUS, &group_status);
  if (not (group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
    fprintf(stderr, "VFIO group not usable. Did you bind all devices to vfio-pci?\n");
    return EXIT_FAILURE;
  }

  ioctl(group, VFIO_GROUP_SET_CONTAINER, &container);
  ioctl(container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);

  int device = ioctl(group, VFIO_GROUP_GET_DEVICE_FD, argv[2]);
  if (device < 0) { perror("VFIO_GROUP_GET_DEVICE_FD"); return EXIT_FAILURE; }
  ioctl(device, VFIO_DEVICE_GET_INFO, &device_info);
  printf("Device %s has %u region%s and %u IRQ%s\n",
         argv[2],
         device_info.num_regions, device_info.num_regions != 1 ? "s" : "",
         device_info.num_irqs,    device_info.num_irqs    != 1 ? "s" : "");

  uint64_t config_offset = 0;

  for (int i = 0; i < device_info.num_regions; i++) {
    struct vfio_region_info reg = { .argsz = sizeof(reg) };

    reg.index = i;

    ioctl(device, VFIO_DEVICE_GET_REGION_INFO, &reg);

    printf("Region %02u: size %08llx offset %016llx flags%s%s%s\n",
           i, reg.size, reg.offset,
           (reg.flags & VFIO_REGION_INFO_FLAG_MMAP)  ? " MMAP" : "",
           (reg.flags & VFIO_REGION_INFO_FLAG_READ)  ? " READ" : "",
           (reg.flags & VFIO_REGION_INFO_FLAG_WRITE) ? " WRITE" : ""
           );

    if (i == VFIO_PCI_CONFIG_REGION_INDEX)
      config_offset = reg.offset;
  }



  uint32_t vfio_read_cfg(int reg, int width) {
    uint32_t ret = 0;
    int res = pread(device, &ret, width, config_offset + reg);
    assert(res == width);
    return ret;
  }

  void vfio_write_cfg(int reg, uint32_t val, int width) {
    pwrite(device, &val, width, config_offset + reg);
    /* Do nothing for now... */
  }

  void *vfio_map_bar(int bar, size_t *size) {
    struct vfio_region_info reg = { .argsz = sizeof(reg) };
    reg.index = bar;
    ioctl(device, VFIO_DEVICE_GET_REGION_INFO, &reg);

    *size = reg.size;

    if (not (reg.flags & VFIO_REGION_INFO_FLAG_MMAP)) {
      return NULL;
    }

    return mmap(NULL, reg.size, PROT_READ | PROT_WRITE, MAP_SHARED, device,
                reg.offset);
  }


  /****************** LOAD DRIVER */

  void *lib = dlopen(argv[3], RTLD_LAZY | RTLD_LOCAL);
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
    void *device = unet_create_device(vfio_read_cfg, vfio_write_cfg, vfio_map_bar);

    int res = driver_methods[PROBE].fn(device);
    if (res == -20) {
      printf("Driver successfully probed device!\n", res);
      printf("Device detected as '%s'.\n", device_get_desc(device));
      res = device_set_driver(device, *driver_struct);
      printf("Calling attach...\n");
      res = driver_methods[ATTACH].fn(device);
      printf("attach return %d.\n", res);
    } else {
      printf("Something went wrong? Return value is %d.\n", res);
    }
  } else {
    printf("No probe method found?\n");
  }

  return 0;
}

/* EOF */
