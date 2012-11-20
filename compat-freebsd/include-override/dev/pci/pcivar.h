
#pragma once

#include <sys/bus.h>

#undef __BUS_ACCESSOR
#define __BUS_ACCESSOR(bus, var, class, ivar, type)

#define pci_write_config pci_write_config_unused_xxx
#define pci_read_config  pci_read_config_unused_xxx
#define pci_msi_count    pci_msi_count_unused_xxx
#define pci_alloc_msi    pci_alloc_msi_unused_xxx
#define pci_find_cap     pci_find_cap_unused_xxx

#include_next <dev/pci/pcivar.h>

#undef  pci_write_config
#undef  pci_read_config
#undef  pci_msi_count
#undef  pci_alloc_msi
#undef  pci_find_cap

uint16_t pci_get_vendor(device_t dev);
uint16_t pci_get_device(device_t dev);
uint16_t pci_get_subvendor(device_t dev);
uint16_t pci_get_subdevice(device_t dev);

uint32_t pci_read_config (device_t dev, int reg, int width);
void     pci_write_config(device_t dev, int reg, uint32_t val, int width);

int      pci_msi_count(device_t dev);
int      pci_alloc_msi(device_t dev, int *count);
int      pci_find_cap(device_t dev, int capability, int *capreg);

/* EOF */
