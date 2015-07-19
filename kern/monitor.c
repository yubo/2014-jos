// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "showmappings", "display the physical page mappings and corresponding permission bits", mon_showmappings },
	{ "mset", "set or clear a flag in a specific page", mon_mset },
	{ "mdump", "dump memory", mon_mdump },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	struct Eipdebuginfo info;
	uint32_t ebp = read_ebp(), eip = 0;
	uint32_t* ebpp;
	cprintf("Stack backtrace:");
	while(ebp){//if ebp is 0, we are back at the first caller
		ebpp = (uint32_t*) ebp;
		eip = *(ebpp+1);
		cprintf("\n  ebp %08x eip %08x args ", ebp, eip);
		
		int argno = 0;
		for(; argno < 5; argno++){
			cprintf("%08x ", *(ebpp+2+argno));
		}
		if(debuginfo_eip(eip, &info) == 0){
			cprintf("\n\t%s:%d: ", info.eip_file, info.eip_line);
			cprintf("%.*s+%d", info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
		}
		ebp = *ebpp;
	}
	cprintf("\n");
	return 0;
}

static uint32_t xtoi(char *buf){
	uint32_t i, n = 0;
	buf += 2;
	while (*buf){
		if (*buf >= 'a'){
			i = *buf - 'a' + 10;
		} else if (*buf >= 'A'){
			i = *buf - 'A' + 10;
		} else if (*buf >= '0'){
			i = *buf - '0';
		} else{
			return 0;
		}
		if (i > 15) return 0;
		n = n*16 + i;
		buf++;
	}
	return n;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t begin, end;
	pte_t *pte;

	if (argc != 3) {
		cprintf("usage: showmappings 0xbegin 0xend\n");
		return 0;
	}
	begin = xtoi(argv[1]);
	end = xtoi(argv[2]);
	cprintf("begin: 0x%x, end: 0x%x\n", begin, end);
	for(; begin <= end; begin += PGSIZE){
		pte = pgdir_walk(kern_pgdir, (void *)begin, 0);
		if(pte && (*pte & PTE_P)){
			cprintf("va:0x%08x pa:0x%08x PTE_P:%x PTE_W:%x PTE_U:%x\n", 
					begin, *pte&0xfffff000, *pte&PTE_P, 
					*pte&PTE_W, *pte&PTE_U);
		}else{
		//	cprintf("va:0x%08x not exist\n");
		}
	}
	return 0;
}

int
mon_mset(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t addr, perm = 0;
	pte_t *pte;

	if (argc != 4) {
		cprintf("usage: mset 0xAddr [0|1 : clear or set] [P|U|W]\n");
		return 0;
	}
	addr = xtoi(argv[1]);
	pte = pgdir_walk(kern_pgdir, (void *)addr, 0);
	if(pte && (*pte & PTE_P)){
		cprintf("va:0x%08x pa:0x%08x PTE_P:%x PTE_W:%x PTE_U:%x\n", 
				addr, *pte&0xfffff000, *pte&PTE_P, 
				*pte&PTE_W, *pte&PTE_U);
		if(argv[3][0] == 'P') perm = PTE_P;
		if(argv[3][0] == 'W') perm = PTE_W;
		if(argv[3][0] == 'U') perm = PTE_U;
		if(argv[2][0] == '0') 
			*pte &= ~perm;
		else
			*pte |=  perm;
		cprintf("va:0x%08x pa:0x%08x PTE_P:%x PTE_W:%x PTE_U:%x\n", 
				addr, *pte&0xfffff000, *pte&PTE_P, 
				*pte&PTE_W, *pte&PTE_U);
	}else{
		cprintf("va:0x%08x not exist\n");
	}
	return 0;
}


int
mon_mdump(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t addr, size, i;
	pte_t *pte;

	if (argc != 3) {
		cprintf("usage: mdump 0xAddr 0xSize\n");
		return 0;
	}
	addr = xtoi(argv[1]);
	size = xtoi(argv[2]);
	for (i = 0; i < size; i += 4){
		if(i%16 == 0)
			cprintf("\nva(0x%08x): ", addr+i);
		cprintf("%08x ", (uint32_t *)(*(char **)addr + i));
	}
	cprintf("\n");
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		//print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
