
#pragma once

#include <sys/bus.h>

#undef __BUS_ACCESSOR
#define __BUS_ACCESSOR(bus, var, class, ivar, type)

#include_next <dev/pci/pcivar.h>

uint16_t pci_get_vendor(device_t dev);
uint16_t pci_get_device(device_t dev);
uint16_t pci_get_subvendor(device_t dev);
uint16_t pci_get_subdevice(device_t dev);
