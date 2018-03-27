/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 *
 * 
 * Credits: xVortex, BISOON, Mistawes

#include "ps4.h"
#include "ps4api.h"
#include "defines.h"
#include "debug.h"
#include "ftps4.h"
#include "dump.h"

#include "global.h"

#define	KERN_XFAST_SYSCALL 0x3095D0
#define KERN_PROCESS_ASLR 0x1BA559
#define KERN_PRISON_0 0x10399B0
#define KERN_ROOTVNODE 0x21AFA30
#define KERN_PTRACE_CHECK 0x17D2C1

int run;

void custom_SHUTDOWN(ftps4_client_info_t *client) {
	ftps4_ext_client_send_ctrl_msg(client, "200 Shutting down..." FTPS4_EOL);
	run = 0;
}

char mount_from_path[PATH_MAX]; /* Yes, global. Lazy */

void custom_MTFR(ftps4_client_info_t *client)
{
	char from_path[PATH_MAX];
	/* Get the origin filename */
	ftps4_gen_ftp_fullpath(client, from_path, sizeof(from_path));

	/* The file to be renamed is the received path */
	strncpy(mount_from_path, from_path, sizeof(mount_from_path));
	ftps4_ext_client_send_ctrl_msg(client, "350 I need the destination name b0ss." FTPS4_EOL);
}

void custom_MTTO(ftps4_client_info_t *client)
{
	char path_to[PATH_MAX];
	struct iovec iov[8];
	char msg[512];
	char errmsg[255];
	int result;

	/* Get the destination filename */
	ftps4_gen_ftp_fullpath(client, path_to, sizeof(path_to));

	/* Just in case */
	unmount(path_to, 0);

	iov[0].iov_base = "fstype";
	iov[0].iov_len = sizeof("fstype");
	iov[1].iov_base = "nullfs";
	iov[1].iov_len = sizeof("nullfs");
	iov[2].iov_base = "fspath";
	iov[2].iov_len = sizeof("fspath");
	iov[3].iov_base = path_to;
	iov[3].iov_len = strlen(path_to) + 1;
	iov[4].iov_base = "target";
	iov[4].iov_len = sizeof("target");
	iov[5].iov_base = mount_from_path;
	iov[5].iov_len = strlen(mount_from_path) + 1;
	iov[6].iov_base = "errmsg";
	iov[6].iov_len = sizeof("errmsg");
	iov[7].iov_base = errmsg;
	iov[7].iov_len = sizeof(errmsg);
	result = nmount(iov, 8, 0);
	if (result < 0)
	{
		if (strlen(errmsg) > 0)
			snprintf(msg, sizeof(msg), "550 Could not mount (%d): %s." FTPS4_EOL, errno, errmsg);
		else
			snprintf(msg, sizeof(msg), "550 Could not mount (%d)." FTPS4_EOL, errno);
		ftps4_ext_client_send_ctrl_msg(client, msg);
		return;
	}

	ftps4_ext_client_send_ctrl_msg(client, "200 Mount success." FTPS4_EOL);
}

void custom_UMT(ftps4_client_info_t *client)
{
	char msg[512];
	int result;
	char mount_path[PATH_MAX];

	ftps4_gen_ftp_fullpath(client, mount_path, sizeof(mount_path));

	result = unmount(mount_path, 0);
	if (result < 0)
	{
		sprintf(msg, "550 Could not unmount (%d)." FTPS4_EOL, errno);
		ftps4_ext_client_send_ctrl_msg(client, msg);
		return;
	}

	ftps4_ext_client_send_ctrl_msg(client, "200 Unmount success." FTPS4_EOL);
}

unsigned int long long __readmsr(unsigned long __register) {
	unsigned long __edx;
	unsigned long __eax;
	__asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register));
	return (((unsigned int long long)__edx) << 32) | (unsigned int long long)__eax;
}

#define X86_CR0_WP (1 << 16)

static inline __attribute__((always_inline)) uint64_t readCr0(void) {
	uint64_t cr0;
	
	asm volatile (
		"movq %0, %%cr0"
		: "=r" (cr0)
		: : "memory"
 	);
	
	return cr0;
}

static inline __attribute__((always_inline)) void writeCr0(uint64_t cr0) {
	asm volatile (
		"movq %%cr0, %0"
		: : "r" (cr0)
		: "memory"
	);
}

struct auditinfo_addr {
    char useless[184];
};

struct ucred {
	uint32_t useless1;
	uint32_t cr_uid;     // effective user id
	uint32_t cr_ruid;    // real user id
 	uint32_t useless2;
    	uint32_t useless3;
    	uint32_t cr_rgid;    // real group id
    	uint32_t useless4;
    	void *useless5;
    	void *useless6;
    	void *cr_prison;     // jail(2)
    	void *useless7;
    	uint32_t useless8;
    	void *useless9[2];
    	void *useless10;
    	struct auditinfo_addr useless11;
    	uint32_t *cr_groups; // groups
    	uint32_t useless12;
};

struct filedesc {
	void *useless1[3];
    	void *fd_rdir;
    	void *fd_jdir;
};

struct proc {
    	char useless[64];
    	struct ucred *p_ucred;
    	struct filedesc *p_fd;
};

struct thread {
    	void *useless;
    	struct proc *td_proc;
};

#define	KERN_XFAST_SYSCALL	0x3095D0	// 4.55
#define KERN_PRISON_0		0x10399B0
#define KERN_ROOTVNODE		0x21AFA30

inline uint8_t* getKernelBase() {
	uint32_t lo, hi;
	__asm__ __volatile__("rdmsr" : "=a" (lo), "=d" (hi) : "c"(0xC0000082));
	return (uint8_t*)(((uint64_t)lo | ((uint64_t)hi << 32)) - KERN_XFAST_SYSCALL);
}

int kpayload(struct thread *td){

	struct ucred* cred;
	struct filedesc* fd;

	fd = td->td_proc->p_fd;
	cred = td->td_proc->p_ucred;
	
	void* kernel_base = &((uint8_t*)__readmsr(0xC0000082))[-KERN_XFAST_SYSCALL];
	uint8_t* kernel_ptr = (uint8_t*)kernel_base;
	void** got_prison0 =   (void**)&kernel_ptr[KERN_PRISON_0];
	void** got_rootvnode = (void**)&kernel_ptr[KERN_ROOTVNODE];

	cred->cr_uid = 0;
	cred->cr_ruid = 0;
	cred->cr_rgid = 0;
	cred->cr_groups[0] = 0;

	cred->cr_prison = *got_prison0;
	fd->fd_rdir = fd->fd_jdir = *got_rootvnode;

	// escalate ucred privs, needed for access to the filesystem ie* mounting & decrypting files
	void *td_ucred = *(void **)(((char *)td) + 304); // p_ucred == td_ucred
	
	// sceSblACMgrIsSystemUcred
	uint64_t *sonyCred = (uint64_t *)(((char *)td_ucred) + 96);
	*sonyCred = 0xffffffffffffffff;
	
	// sceSblACMgrGetDeviceAccessType
	uint64_t *sceProcType = (uint64_t *)(((char *)td_ucred) + 88);
	*sceProcType = 0x3801000000000013; // Max access
	
	// sceSblACMgrHasSceProcessCapability
	uint64_t *sceProcCap = (uint64_t *)(((char *)td_ucred) + 104);
	*sceProcCap = 0xffffffffffffffff; // Sce Process

	// Disable write protection
	uint64_t cr0 = readCr0();
	writeCr0(cr0 & ~X86_CR0_WP);

	// Enable uart :)
	*(char *)(kernel_base + 0x1997BC8) = 0;

	// Debug settings patchs
	*(char *)(kernel_base + 0x1B6D086) |= 0x14;
	*(char *)(kernel_base + 0x1B6D0A9) |= 3;
	*(char *)(kernel_base + 0x1B6D0AA) |= 1;
	*(char *)(kernel_base + 0x1B6D0C8) |= 1;	

	// Debug menu full patches
	*(uint32_t *)(kernel_base + 0x4D70F7) = 0;
	*(uint32_t *)(kernel_base + 0x4D7F81) = 0;

	// Enable mmap of all SELF
	*(uint8_t*)(kernel_base + 0x143BF2) = 0x90;
	*(uint8_t*)(kernel_base + 0x143BF3) = 0xE9;
	*(uint8_t*)(kernel_base + 0x143E0E) = 0x90;
	*(uint8_t*)(kernel_base + 0x143E0F) = 0x90;

	// Disable ptrace check
	kernel_ptr[KERN_PTRACE_CHECK] = 0xEB;

	// Disable process aslr
	*(uint16_t*)&kernel_ptr[KERN_PROCESS_ASLR] = 0x9090;

	// Restore write protection
	writeCr0(cr0);

	return 0;
}

int get_ip_address(char *ip_address)
{
	int ret;
	SceNetCtlInfo info;

	ret = sceNetCtlInit();
	if (ret < 0)
		goto error;

	ret = sceNetCtlGetInfo(SCE_NET_CTL_INFO_IP_ADDRESS, &info);
	if (ret < 0)
		goto error;

	memcpy(ip_address, info.ip_address, sizeof(info.ip_address));

	sceNetCtlTerm();

	return ret;

error:
	ip_address = NULL;
	return -1;
}

int _main(struct thread *td)
{
	char ip_address[SCE_NET_CTL_IPV4_ADDR_STR_LEN];
	char msg[64];

	run = 1;

	// Init and resolve libraries
	initKernel();
	initLibc();
	initNetwork();
	initPthread();

#ifdef DEBUG_SOCKET
	initDebugSocket();
#endif

	// patch some things in the kernel (sandbox, prison, debug settings etc..)
	syscall(11,kpayload,td);

	initSysUtil();
	notify("Welcome to FTPS4API v"VERSION);

	int startPs4Api;

	ScePthread thread;
	scePthreadCreate(&thread, NULL, ps4api, (void *)&startPs4Api, "ps4api_thread");

	// Start FTPS4
	int ret = get_ip_address(ip_address);
	if (ret < 0)
	{
		notify("Unable to get IP address");
		goto error;
	}

	ftps4_init(ip_address, FTP_PORT);
	ftps4_ext_add_custom_command("SHUTDOWN", custom_SHUTDOWN);
	ftps4_ext_add_custom_command("MTFR", custom_MTFR);
	ftps4_ext_add_custom_command("MTTO", custom_MTTO);
	ftps4_ext_add_custom_command("UMT", custom_UMT);

	sprintf(msg, "PS4 listening on\nIP %s Port %i", ip_address, FTP_PORT);
	notify(msg);

	while (run) {
		sceKernelUsleep(5 * 1000);
	}

	ftps4_fini();

error:
	notify("Bye!");

#ifdef DEBUG_SOCKET
	closeDebugSocket();
#endif

	// Wait until the ps4_api thread terminates
	scePthreadJoin(thread, NULL);
	return startPs4Api;
	return 0;
}
