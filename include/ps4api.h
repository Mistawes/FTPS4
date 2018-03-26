#define NOTIFYT(type, format, ...)\
	do {\
		char bufferForTheSocket[512];\
		sprintf(bufferForTheSocket, format, ##__VA_ARGS__);\
		sceSysUtilSendSystemNotificationWithText(type, bufferForTheSocket);\
	} while(0)
#define NOTIFY(format, ...) NOTIFYT(222, format, ##__VA_ARGS__)

int createThread(void*(func)(void*), void* args);

void* clientHandler(void* args);

int ps4api(void);

