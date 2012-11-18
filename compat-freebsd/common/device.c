/* -*- Mode: C -*- */

#include <sys/libkern.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/bus.h>
#include <dev/pci/pcivar.h>

struct device {
  //KOBJ_FIELDS;

  uint16_t pci_vendor;
  uint16_t pci_device;

  uint16_t pci_subvendor;
  uint16_t pci_subdevice;

  char     desc[128];
};

uint16_t pci_get_vendor(device_t dev) { return dev->pci_vendor; }
uint16_t pci_get_device(device_t dev) { return dev->pci_device; }

uint16_t pci_get_subvendor(device_t dev) { return dev->pci_subvendor; }
uint16_t pci_get_subdevice(device_t dev) { return dev->pci_subdevice; }


void device_set_desc_copy(device_t dev, const char *desc)
{
  strncpy(dev->desc, desc, sizeof(dev->desc)-1);
  dev->desc[sizeof(dev->desc)-1] = 0;
}

const char *device_get_desc(device_t dev)
{
  return dev->desc;
}


// 00:19.0 Ethernet controller [0200]: Intel Corporation 82577LM Gigabit Network Connection [8086:10ea] (rev 06)
//        Subsystem: Lenovo Device [17aa:2153]

static struct device e1000_82577LM = {
  .pci_vendor = 0x8086,
  .pci_device = 0x10ea,
  .pci_subvendor = 0x17aa,
  .pci_subdevice = 0x10ea,
  .desc = "???"
};

device_t e1000_pci_example = &e1000_82577LM;

/* EOF */
