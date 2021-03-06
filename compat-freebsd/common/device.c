/* -*- Mode: C -*- */

/* Parts of this code are covered by the original FreeBSD license. */

#include <sys/libkern.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/bus_dma.h>
#include <sys/rman.h>
#include <sys/buf_ring.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <stdarg.h>
#include <iso646.h>
#include "../pci-interface.h"

void *calloc(size_t nmemb, size_t size);
void *malloc(size_t size);
void *memalign(size_t alignment, size_t size);

struct device {
  //KOBJ_FIELDS;
  char     desc[128];

  read_cfg  config_read;
  write_cfg config_write;
  map_bar   bar_map;

  void     *softc;
  driver_t *driver;

};

void *unet_create_device(read_cfg read_fn, write_cfg write_fn, map_bar bar_map)
{
  device_t dev = calloc(1, sizeof(struct device));
  strncpy(dev->desc, "<unnamed>", sizeof(dev->desc));

  dev->config_read  = read_fn;
  dev->config_write = write_fn;
  dev->bar_map      = bar_map;

  return dev;
}

uint16_t pci_get_vendor(device_t dev) { return dev->config_read(PCIR_VENDOR, 2); }
uint16_t pci_get_device(device_t dev) { return dev->config_read(PCIR_DEVICE, 2); }

uint16_t pci_get_subvendor(device_t dev) { return dev->config_read(PCIR_SUBVEND_0, 2); }
uint16_t pci_get_subdevice(device_t dev) { return dev->config_read(PCIR_SUBDEV_0,  2); }

int device_get_unit(device_t dev)                 { return 0; }
int resource_disabled(const char *name, int unit) { return 0; }

void device_set_desc_copy(device_t dev, const char *desc)
{
  strncpy(dev->desc, desc, sizeof(dev->desc)-1);
  dev->desc[sizeof(dev->desc)-1] = 0;
}

const char *device_get_desc    (device_t dev) { return dev->desc;  }
void       *device_get_softc   (device_t dev) { return dev->softc; }
const char *device_get_nameunit(device_t dev) { return "user0";    }

int device_set_driver(device_t dev, driver_t *driver)
{
  if (dev->driver == driver) return 0;
  if (dev->driver != 0)      return -1;

  dev->driver = driver;
  printf("Allocating %zu bytes softc for driver '%s'.\n",
         driver->size, driver->name);
  dev->softc  = malloc(driver->size);

  return 0;
}

uint32_t pci_read_config(device_t dev,  int reg, int width)
{
  return dev->config_read(reg, width);
}

void pci_write_config(device_t dev, int reg, uint32_t val, int width)
{
  dev->config_write(reg, val, width);
}

void panic(const char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

        __builtin_trap();
}
int device_printf(device_t dev, const char * fmt, ...)
{
	va_list ap;
	int retval;

        printf("user0: ");
	va_start(ap, fmt);
	retval += vprintf(fmt, ap);
	va_end(ap);
	return (retval);
}


struct resource *bus_alloc_resource(device_t dev, int type, int *rid,
                                    u_long start, u_long end, u_long count,
                                    u_int flags)
{
  if (not ((start == 0) and (end == ~0UL) and (count == 1))) {
    device_printf(dev, "generic bus_alloc_resource is UNIMPLEMENTED.");
    return NULL;
  }

  switch (type) {
  case SYS_RES_IRQ: {
    struct resource *res = calloc(1, sizeof(struct resource));
    /* XXX we leak the resource right now */
    device_printf(dev, "XXX allocating non-working IRQ resource...\n");
    return res;
  }
  case SYS_RES_MEMORY: {
    unsigned bar = (*rid - PCIR_BARS) / 4;
    device_printf(dev, "trying to map BAR%u.\n", bar);

    size_t size;
    void  *m = dev->bar_map(bar, &size);
    if (m == NULL) {
      device_printf(dev, "cannot deal with non-mapable BARs right now.\n");
      return NULL;
    }

    struct resource *res = calloc(1, sizeof(struct resource));
    /* XXX we leak the resource right now */

    //res->r_bustag    = X86_BUS_SPACE_MEM;
    res->r_bustag    = 1;
    res->r_bushandle = (uintptr_t)m;

    return res;
  }
  default:
    return NULL;
  }
}

bus_space_tag_t
rman_get_bustag(struct resource *r)
{
  return (r->r_bustag);
}

bus_space_handle_t
rman_get_bushandle(struct resource *r)
{
  return (r->r_bushandle);
}

int pci_msi_count(device_t dev)
{
  device_printf(dev, "XXX hardcoded 1 MSI\n");
  return 1;
}

int pci_alloc_msi(device_t dev, int *count)
{
  return 0;
}

int pci_find_cap(device_t dev, int capability, int *capreg)
{

  u_int32_t status;
  u_int8_t ptr;

  /*
   * Check the CAP_LIST bit of the PCI status register first.
   */
  status = pci_read_config(dev, PCIR_STATUS, 2);
  if (!(status & PCIM_STATUS_CAPPRESENT))
    return (ENXIO);

  /*
   * Determine the start pointer of the capabilities list.
   */
  /* switch (cfg->hdrtype & PCIM_HDRTYPE) { */
  /* case PCIM_HDRTYPE_NORMAL: */
  /* case PCIM_HDRTYPE_BRIDGE: */
  ptr = PCIR_CAP_PTR;
  /* 	break; */
  /* case PCIM_HDRTYPE_CARDBUS: */
  /* 	ptr = PCIR_CAP_PTR_2; */
  /* 	break; */
  /* default: */
  /* 	/\* XXX: panic? *\/ */
  /* 	return (ENXIO);		/\* no extended capabilities support *\/ */
  /* } */
  ptr = pci_read_config(dev, ptr, 1);

  /*
   * Traverse the capabilities list.
   */
  while (ptr != 0) {
    if (pci_read_config(dev, ptr + PCICAP_ID, 1) == capability) {
      if (capreg != NULL) {
        device_printf(dev, "found cap at 0x%x\n", ptr);
        *capreg = ptr;
      }
      return (0);
    }
    ptr = pci_read_config(dev, ptr + PCICAP_NEXTPTR, 1);
  }

  device_printf(dev, "no such cap\n");
  if (capreg != NULL) *capreg = 0;
  return (ENOENT);

}

int usleep(unsigned usec);
void DELAY(int n)
{
  usleep(n);
}

bus_dma_tag_t
bus_get_dma_tag(device_t dev)
{
  return 0;
}

int
bus_dma_tag_create(bus_dma_tag_t parent, bus_size_t alignment,
                   bus_size_t boundary,
                   bus_addr_t lowaddr, bus_addr_t highaddr,
                   bus_dma_filter_t *filtfunc, void *filterarg,
                   bus_size_t maxsize, int nsegments, bus_size_t maxsegsz,
                   int flags, bus_dma_lock_t *lockfn, void *lockarg,
                   bus_dma_tag_t *tag)
{
  /* We remember maxsize as the DMA tag itself instead allocating some
     complex struct. */
  *tag = (bus_dma_tag_t)maxsize;
  return 0;
}

struct bus_dmamap {
  void       *buf;
  bus_size_t  buflen;
};

int
bus_dmamap_create(bus_dma_tag_t dmat, int flags, bus_dmamap_t *mapp)
{
  printf("XXX bus_dmamap_create is a NOP!\n");
  *mapp = NULL;
  return 0;
}

int
bus_dmamem_alloc(bus_dma_tag_t dmat, void **vaddr, int flags,
                 bus_dmamap_t *mapp)
{
  bus_dmamap_t map = calloc(1, sizeof(struct bus_dmamap));
  map->buf    = memalign(4096, (size_t)dmat);
  map->buflen = (size_t)dmat;

  printf("Allocated 0x%zx bytes at %p.\n", map->buflen, map->buf);

  *vaddr = map->buf;
  *mapp  = map;

  return 0;
 }

int
bus_dmamap_load(bus_dma_tag_t dmat, bus_dmamap_t map, void *buf,
                bus_size_t buflen, bus_dmamap_callback_t *callback,
                void *callback_arg, int flags)
{
  /* XXX Make buffer visible to device */

  /* dma_map.vaddr = mmap(0, 1024 * 1024, PROT_READ | PROT_WRITE, */
  /*                          MAP_PRIVATE | MAP_ANONYMOUS, 0, 0); */
  /* dma_map.size = 1024 * 1024; */
  /* dma_map.iova = 0; /\* 1MB starting at 0x0 from device view *\/ */
  /* dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE; */
  /* ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map); */


  bus_dma_segment_t seg = { .ds_addr = (bus_addr_t)map->buf,
                            .ds_len  = map->buflen };
  callback(callback_arg, &seg, 1, 0);
  return 0;
}

struct buf_ring *
buf_ring_alloc(int count, struct malloc_type *t, int flags, struct mtx *sc_lock)
{
  size_t len = sizeof(struct buf_ring) + count*sizeof(void *);
  struct buf_ring *br = malloc(len);

  memset(br, 0, len);

  br->br_prod_size = br->br_cons_size = count;
  br->br_prod_mask = br->br_cons_mask = count-1;
  br->br_prod_head = br->br_cons_head = 0;
  br->br_prod_tail = br->br_cons_tail = 0;

  return br;
}

int
bcmp(const void *b1, const void *b2, size_t len)
{
  return memcmp(b1, b2, len);
}

/* EOF */
