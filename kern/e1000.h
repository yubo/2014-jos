#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>
#include <kern/e1000_regs.h>

volatile uint32_t *e1000; // MMIO address to access E1000 BAR

#define E1000_TXDESC 64
#define E1000_RCVDESC 64
#define TX_PKT_SIZE 1518
#define RCV_PKT_SIZE 2048

#define E1000_EERD_START 0x01
#define E1000_EERD_DONE  0x10

struct tx_pkt
{
	uint8_t buf[TX_PKT_SIZE];
} __attribute__((packed));

struct rcv_pkt
{
	uint8_t buf[RCV_PKT_SIZE];
} __attribute__((packed));


int e1000_attach(struct pci_func *pcif);
int e1000_transmit(char *data, int len);

#endif	// JOS_KERN_E1000_H
