#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>
#include <netif/etharp.h>

struct e1000_tx_desc tx_desc_array[E1000_TXDESC] __attribute__ ((aligned (16)));
struct tx_pkt tx_pkt_bufs[E1000_TXDESC];

struct e1000_rx_desc rx_desc_array[E1000_RXDESC] __attribute__ ((aligned (16)));
struct rx_pkt rx_pkt_bufs[E1000_RXDESC];

static void hexdump(const char *prefix, const void *data, int len);

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
	defreg(ITR),    defreg(TIPG),   defreg(RXERRC),
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
		tx_desc_array[i].upper.data |= E1000_TXD_STAT_DD;
	}

	// Initialize rcv desc buffer array
	memset(rx_desc_array, 0x0, sizeof(struct e1000_rx_desc) * E1000_RXDESC);
	memset(rx_pkt_bufs, 0x0, sizeof(struct rx_pkt) * E1000_RXDESC);
	for (i = 0; i < E1000_RXDESC; i++) {
		rx_desc_array[i].buffer_addr = PADDR(rx_pkt_bufs[i].buf);
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

	/* Receive Initialization */
	// Program the Receive Address Registers
	
	e1000[RA] = 0x12005452;
	e1000[RA+1] = (e1000[RA+1] & 0xffff0000) | 0x5634;

	// Program the Receive Descriptor Base Address Registers
	e1000[RDBAL] = PADDR(rx_desc_array);
    e1000[RDBAH] = 0x0;

	// Set the Receive Descriptor Length Register
	e1000[RDLEN] = sizeof(struct e1000_rx_desc) * E1000_RXDESC;

    // Set the Receive Descriptor Head and Tail Registers
	e1000[RDH] = 0x0;
	e1000[RDT] = E1000_RXDESC - 1;
	e1000[MTA] = 0x0;

	// Initialize the Receive Control Register
	e1000[RCTL] = E1000_RCTL_BAM | 
		E1000_RCTL_SZ_2048 | 
		E1000_RCTL_SECRC | 
		E1000_RCTL_EN;

	return 0;
}


static void
hexdump(const char *prefix, const void *data, int len)
{
	int i;
	char buf[80];
	char *end = buf + sizeof(buf);
	char *out = NULL;
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			out = buf + snprintf(buf, end - buf,
					     "%s%04x   ", prefix, i);
		out += snprintf(out, end - out, "%02x", ((uint8_t*)data)[i]);
		if (i % 16 == 15 || i == len - 1)
			cprintf("%.*s\n", out - buf, buf);
		if (i % 2 == 1)
			*(out++) = ' ';
		if (i % 16 == 7)
			*(out++) = ' ';
	}
}

int
e1000_transmit(char *data, int len)
{
	if (len > TX_PKT_SIZE) {
		return -E_PKT_TOO_LONG;
	}

	uint32_t tdt = e1000[TDT];

	// Check if next tx desc is free
	if (tx_desc_array[tdt].upper.data & E1000_TXD_STAT_DD) {
		memmove(tx_pkt_bufs[tdt].buf, data, len);
		tx_desc_array[tdt].lower.flags.length = len;

		tx_desc_array[tdt].upper.data &= ~E1000_TXD_STAT_DD;
		tx_desc_array[tdt].lower.data |= E1000_TXD_CMD_EOP;
		tx_desc_array[tdt].lower.data |= E1000_TXD_CMD_RS;

		//hexdump("tx dump:", data, len);
		e1000[TDT] = (tdt + 1) % E1000_TXDESC;
	}
	else { // tx queue is full!
		return -E_TX_FULL;
	}

	return 0;
}

int
e1000_receive(char *data)
{
	uint32_t rdt, len;
	rdt = (e1000[RDT] + 1) % E1000_RXDESC;
	
	if (rx_desc_array[rdt].status & E1000_RXD_STAT_DD) {
		if (!(rx_desc_array[rdt].status & E1000_RXD_STAT_EOP)) {
			panic("Don't allow jumbo frames!\n");
		}
		len = rx_desc_array[rdt].length;
		
		memmove(data, rx_pkt_bufs[rdt].buf, len);
		//hexdump("rx dump:", data, len);

		rx_desc_array[rdt].status &= ~E1000_RXD_STAT_DD;
		rx_desc_array[rdt].status &= ~E1000_RXD_STAT_EOP;
		e1000[RDT] = rdt;

		return len;
	}

	return -E_RCV_EMPTY;
}
