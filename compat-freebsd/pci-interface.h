/* -*- Mode: C -*- */

#pragma once

/* We don't pull additional headers here, because this file is
   included in the userland and FreeBSD kernel world. */

typedef uint32_t  (*read_cfg) (int reg, int width);
typedef void      (*write_cfg)(int reg, uint32_t val, int width);
typedef void     *(*map_bar)  (int bar, size_t *size);
typedef unsigned  (*msi_count)(void);

void *unet_create_device(read_cfg read_fn, write_cfg write_fn, map_bar bar_map);

struct kobj_class;
struct device;
typedef struct kobj_class driver_t;
typedef struct device *device_t;

const char *device_get_desc  (device_t dev);
int         device_set_driver(device_t dev, driver_t *driver);

extern void *e1000_pci_example;

/* EOF */
