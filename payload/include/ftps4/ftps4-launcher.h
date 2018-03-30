#include "ps4.h"
#include "ftps4/defines.h"
#include "ftps4/debug.h"
#include "ftps4/ftps4.h"
#include "ftps4/dump.h"

int run;
void custom_SHUTDOWN(ftps4_client_info_t *client);
char mount_from_path[PATH_MAX]; // Yes, global. Lazy
void custom_MTFR(ftps4_client_info_t *client);
void custom_MTTO(ftps4_client_info_t *client);
void custom_UMT(ftps4_client_info_t *client);
//int get_ip_address(char *ip_address);
//int gogoftps4(struct thread *td);
//int gogoftps4(int ip_address, char msg);
/*
struct FTPS4thread {
    	void *useless;
    	struct proc *td_proc;
};
*/