
#pragma once

#include <sys/bus.h>

#undef __BUS_ACCESSOR
#define __BUS_ACCESSOR(bus, var, class, ivar, type)

#define pci_write_config pci_write_config_unused_xxx
#define pci_read_config  pci_read_config_unused_xxx

#include_next <dev/pci/pcivar.h>

#undef  pci_write_config
#undef  pci_read_config

uint16_t pci_get_vendor(device_t dev);
uint16_t pci_get_device(device_t dev);
uint16_t pci_get_subvendor(device_t dev);
uint16_t pci_get_subdevice(device_t dev);

uint32_t pci_read_config (device_t dev, int reg, int width);
void     pci_write_config(device_t dev, int reg, uint32_t val, int width);

/* EOF */
