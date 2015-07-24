#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here
int
e1000_attach(struct pci_func *pcif)
{
	// Enable PCI device
	pci_func_enable(pcif);

	return 0;
}

