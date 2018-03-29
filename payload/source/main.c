/* golden */
/* 1/2/2018 */

#include "jkpatch.h"
#include "install.h"

//FTPS4
#include "ftps4/ftps4-launcher.h"

// perfect for putty
void ascii_art(void *_printf) {
	printf("\n\n");
	printf("   _ _                _       _     \n");
	printf("  (_) | ___ __   __ _| |_ ___| |__  \n");
	printf("  | | |/ / '_ \\ / _` | __/ __| '_ \\ \n");
	printf("  | |   <| |_) | (_| | || (__| | | |\n");
	printf(" _/ |_|\\_\\ .__/ \\__,_|\\__\\___|_| |_|\n");
	printf("|__/     |_|                        \n");
	printf("\n\n");
}

void jailbreak(struct thread *td, uint64_t kernbase) {
	void **prison0 =   (void **)(kernbase + __prison0);
	void **rootvnode = (void **)(kernbase + __rootvnode);

	struct ucred* cred;
	struct filedesc* fd;

	fd = td->td_proc->p_fd;
	cred = td->td_proc->p_ucred;

	cred->cr_uid = 0;
	cred->cr_ruid = 0;
	cred->cr_rgid = 0;
	cred->cr_groups[0] = 0;
	cred->cr_prison = *prison0;
	fd->fd_rdir = fd->fd_jdir = *rootvnode;
}

void debug_patches(struct thread *td, uint64_t kernbase) {
	// Debug settings patchs
	*(uint8_t *)(kernbase + 0x1B6D086) |= 0x14;
	*(uint8_t *)(kernbase + 0x1B6D0A9) |= 0x3;
	*(uint8_t *)(kernbase + 0x1B6D0AA) |= 0x1;
	*(uint8_t *)(kernbase + 0x1B6D0C8) |= 0x1;

	// Debug menu full patches
	*(uint32_t *)(kernbase + 0x4D70F7) = 0;
	*(uint32_t *)(kernbase + 0x4D7F81) = 0;

	// Enable mmap of all SELF
	*(uint8_t*)(kernbase + 0x143BF2) = 0x90;
	*(uint8_t*)(kernbase + 0x143BF3) = 0xE9;
	*(uint8_t*)(kernbase + 0x143E0E) = 0x90;
	*(uint8_t*)(kernbase + 0x143E0F) = 0x90;

	// registry patches for extra debug information
	// fucks with the whole system, patches sceRegMgrGetInt
	// 405
	//*(uint32_t *)(kernbase + 0x4CECB7) = 0;
	//*(uint32_t *)(kernbase + 0x4CFB9B) = 0;
	// 455
	//*(uint32_t *)(kernbase + 0x4D70F7) = 0;
	//*(uint32_t *)(kernbase + 0x4D7F81) = 0;

	// flatz RSA check patch
	*(uint32_t *)(kernbase + 0x69F4E0) = 0x90C3C031;

	// flatz enable debug rifs
	*(uint64_t *)(kernbase + 0x62D30D) = 0x3D38EB00000001B8;

	// disable sysdump_perform_dump_on_fatal_trap
	// will continue execution and give more information on crash, such as rip
	*(uint8_t *)(kernbase + 0x736250) = 0xC3;

	// patch vm_map_protect check
	memcpy((void *)(kernbase + 0x396A58), "\x90\x90\x90\x90\x90\x90", 6);

	// patch ASLR, thanks 2much4u
	*(uint16_t *)(kernbase + 0x1BA559) = 0x9090;

	// Disable ptrace check
	//uint8_t* kernel_ptr = (uint8_t*)kernbase;
	//kernel_ptr[0x17D2C1] = 0xEB;
}

void scesbl_patches(struct thread *td, uint64_t kernbase) {
	// JKpatch ucred privs
	//char *td_ucred = (char *)td->td_ucred;

	// escalate ucred privs, needed for access to the filesystem ie* mounting & decrypting files
	void *td_ucred = *(void **)(((char *)td) + 304); // p_ucred == td_ucred

	// signed __int64 __fastcall sceSblACMgrGetDeviceAccessType(__int64 a1, __int64 a2, _DWORD *a3)
	// v6 = *(_QWORD *)(a1 + 0x58);
	*(uint64_t *)(td_ucred + 0x58) = 0x3801000000000013; // gives access to everything

	/*
	signed __int64 __fastcall sceSblACMgrIsSystemUcred(__int64 a1) {
		return (*(_QWORD *)(a1 + 0x60) >> 62) & 1LL;
	}
	*/
	*(uint64_t *)(td_ucred + 0x60) = 0xFFFFFFFFFFFFFFFF;

	/*
	__int64 __fastcall sceSblACMgrHasSceProcessCapability(__int64 a1) {
		return *(_QWORD *)(a1 + 0x68) >> 63;
	}
	*/
	*(uint64_t *)(td_ucred + 0x68) = 0xFFFFFFFFFFFFFFFF;

	// sceSblACMgrIsAllowedSystemLevelDebugging
	*(uint8_t *)(kernbase + 0x36057B) = 0;
}

int receive_payload(void **payload, size_t *psize) {
	struct sockaddr_in server;
	server.sin_len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = IN_ADDR_ANY;
	server.sin_port = sceNetHtons(9023);
	memset(server.sin_zero, 0, sizeof(server.sin_zero));

	int servsock = sceNetSocket("jkpatch", AF_INET, SOCK_STREAM, 0);

	sceNetBind(servsock, (struct sockaddr *)&server, sizeof(server));

	sceNetListen(servsock, 128);

	int client = sceNetAccept(servsock, NULL, NULL);
	if (client < 0) {
		return 1;
	}

	void *data = (void *)malloc(4096);
	int recvlen = 0;
	int length = 0;

	while (1) {
		recvlen = sceNetRecv(client, data + length, 4096, 0);
		length += recvlen;

		if (recvlen) {
			void *ndata = (void *)realloc(data, length + 4096);
			if (ndata) {
				data = ndata;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	if (payload) {
		*payload = data;
	} else {
		free(data);
	}

	if (psize) {
		*psize = length;
	}

	sceNetSocketClose(servsock);

	return 0;
}

// FTPS4
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

// End FTPS4

struct jkuap {
	uint64_t sycall;
	void *payload;
	size_t psize;
};

int kload(struct thread *td, struct jkuap *uap) {
	uint64_t kernbase = getkernbase();
	resolve(kernbase);

	// disable write protect
	uint64_t CR0 = __readcr0();
	__writecr0(CR0 & ~CR0_WP);

	// enable uart
	uint8_t *disable_console_output = (uint8_t *)(kernbase + __disable_console_output);
	*disable_console_output = FALSE;

	// real quick jailbreak ;)
	jailbreak(td, kernbase);

	// quick debug patches
	debug_patches(td, kernbase);

	// sceSblMgr patches
	scesbl_patches(td, kernbase);

	// restore CR0
	__writecr0(CR0);

	return 0;
}
/*
int jkpatch(struct thread *td, struct jkuap *uap) {
	uint64_t kernbase = getkernbase();
	resolve(kernbase);

	// disable write protect
	uint64_t CR0 = __readcr0();
	__writecr0(CR0 & ~CR0_WP);

	// enable uart
	uint8_t *disable_console_output = (uint8_t *)(kernbase + __disable_console_output);
	*disable_console_output = FALSE;

	// real quick jailbreak ;)
	jailbreak(td, kernbase);

	// quick debug patches
	debug_patches(td, kernbase);

	// sceSblMgr patches
	scesbl_patches(td, kernbase);

	// restore CR0
	__writecr0(CR0);

	// print some stuff
	ascii_art(printf);
	printf("jkpatch installer loaded\n");
	printf("[jkpatch] kernbase 0x%llX\n", kernbase);

	printf("[jkpatch] loading payload...\n");

	if (!uap->payload) {
		printf("[jkpatch] payload data is NULL!\n");
		return 1;
	}

	// install wizardry
	if (install_payload(td, kernbase, uap->payload, uap->psize)) {
		printf("[jkpatch] install_payload failed!\n");
		return 1;
	}

	printf("[jkpatch] all done! have fun with homebrew!\n");

	return 0;
}
*/

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

//int gogoftps4(void)
void* gogoftps4(void * td)

{
	char ip_address[SCE_NET_CTL_IPV4_ADDR_STR_LEN];
	char msg[64];
	
	initSysUtil();
	notify("Welcome to FTPS4 v"VERSION);

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

	return 0;
}

int _main(struct thread *td)
{

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
	syscall(11, kload);

	int startFTPS4;
	ScePthread thread;
	scePthreadCreate(&thread, NULL, gogoftps4, (void *)&startFTPS4, "ftps4_thread");

	// Do stuff..

	// Wait until the ftps4 thread terminates
	scePthreadJoin(thread, NULL);
	return startFTPS4;

#ifdef DEBUG_SOCKET
	closeDebugSocket();
#endif
	return 0;
}

/*
int _main(void) {
	initKernel();
	initLibc();
	initNetwork();

	// fuck up the updates
	unlink("/update/PS4UPDATE.PUP");
	mkdir("/update/PS4UPDATE.PUP", 777);

	size_t psize = 0;
	void *payload = NULL;
	receive_payload(&payload, &psize);

	syscall(11, jkpatch, payload, psize);

	if (payload) {
		free(payload);
	}

	return 0;
}
*/