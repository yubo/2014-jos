#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here
int
e1000_attach(struct pci_func *pcif)
{
	// Enable PCI device
	pci_func_enable(pcif);

	// Memory map I/O for PCI device
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

	cprintf("E1000 status: %08x You should get 0x80080783\n", e1000[E1000_STATUS]);

	return 0;
}

