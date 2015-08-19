#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

struct e1000_tx_desc tx_desc_array[E1000_TXDESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_pkt_bufs[E1000_TXDESC];

struct e1000_rx_desc rcv_desc_array[E1000_RCVDESC] __attribute__ ((aligned (16)));
struct rcv_pkt rcv_pkt_bufs[E1000_RCVDESC];


#define	defreg(x)	x = (E1000_##x>>2)
enum {
    defreg(CTRL),	defreg(EECD),	defreg(EERD),	defreg(GPRC),
    defreg(GPTC),	defreg(ICR),	defreg(ICS),	defreg(IMC),
    defreg(IMS),	defreg(LEDCTL),	defreg(MANC),	defreg(MDIC),
    defreg(MPC),	defreg(PBA),	defreg(RCTL),	defreg(RDBAH),
    defreg(RDBAL),	defreg(RDH),	defreg(RDLEN),	defreg(RDT),
    defreg(STATUS),	defreg(SWSM),	defreg(TCTL),	defreg(TDBAH),
    defreg(TDBAL),	defreg(TDH),	defreg(TDLEN),	defreg(TDT),
    defreg(TORH),	defreg(TORL),	defreg(TOTH),	defreg(TOTL),
    defreg(TPR),	defreg(TPT),	defreg(TXDCTL),	defreg(WUFC),
    defreg(RA),		defreg(MTA),	defreg(CRCERRS),defreg(VFTA),
    defreg(VET),    defreg(RDTR),   defreg(RADV),   defreg(TADV),
    defreg(ITR),    defreg(TIPG),
};



// LAB 6: Your driver code here
int
e1000_attach(struct pci_func *pcif)
{
	uint32_t i;

	// Enable PCI device
	pci_func_enable(pcif);

	// Memory map I/O for PCI device
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

	assert(e1000[STATUS] == 0x80080783);
	cprintf("E1000 status: %08x You should get 0x80080783\n", e1000[STATUS]);

	// Initialize tx buffer array
	memset(tx_desc_array, 0x0, sizeof(struct e1000_tx_desc) * E1000_TXDESC);
	memset(tx_pkt_bufs, 0x0, sizeof(struct tx_pkt) * E1000_TXDESC);
	for (i = 0; i < E1000_TXDESC; i++) {
		tx_desc_array[i].buffer_addr = PADDR(tx_pkt_bufs[i].buf);
		tx_desc_array[i].upper.fields.status |= E1000_TXD_STAT_DD;
	}

	// Initialize rcv desc buffer array
	memset(rcv_desc_array, 0x0, sizeof(struct e1000_rx_desc) * E1000_RCVDESC);
	memset(rcv_pkt_bufs, 0x0, sizeof(struct rcv_pkt) * E1000_RCVDESC);
	for (i = 0; i < E1000_RCVDESC; i++) {
		rcv_desc_array[i].buffer_addr = PADDR(rcv_pkt_bufs[i].buf);
	}

	/* Transmit initialization */
	// Program the Transmit Descriptor Base Address Registers
	e1000[TDBAL] = PADDR(tx_desc_array);
	e1000[TDBAH] = 0x0;

	// Set the Transmit Descriptor Length Register
	e1000[TDLEN] = sizeof(struct e1000_tx_desc) * E1000_TXDESC;

	// Set the Transmit Descriptor Head and Tail Registers
	e1000[TDH] = 0x0;
	e1000[TDT] = 0x0;

	// Initialize the Transmit Control Register 
	e1000[TCTL] |= E1000_TCTL_EN;
	e1000[TCTL] |= E1000_TCTL_PSP;
	e1000[TCTL] &= ~E1000_TCTL_CT;
	e1000[TCTL] |= (0x10) << 4;
	e1000[TCTL] &= ~E1000_TCTL_COLD;
	e1000[TCTL] |= (0x40) << 12;

	// Program the Transmit IPG Register
	e1000[TIPG] = 0x0;
	e1000[TIPG] |= (0x6) << 20; // IPGR2 
	e1000[TIPG] |= (0x4) << 10; // IPGR1
	e1000[TIPG] |= 0xA; // IPGR


	return 0;
}

