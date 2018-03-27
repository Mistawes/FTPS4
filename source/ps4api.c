/*
 * Credits: xVortex, BISOON, Mistawes 
 */
#include "ps4.h"
#include "include/ps4api.h"
#include "include/defines.h"
//#include "include/global.h"
//#include "include/kernel.h"
#include "include/sock.h"
#include "include/process.h"
#include "include/commandHandlers.h"

#define MSG_CLIENT_CONNECED "Client [%s] connected" 
#define MSG_CLIENT_DISCONNECED "Client [%s] disconnected"
#define MSG_CLIENT_THREAD_ERROR "Error handling the client"

#define Inline static inline __attribute__((always_inline))
#define	KERN_XFAST_SYSCALL 0x3095D0
#define KERN_PROCESS_ASLR 0x1BA559
#define KERN_PRISON_0 0x10399B0
#define KERN_ROOTVNODE 0x21AFA30
#define KERN_PTRACE_CHECK 0x17D2C1

int createThread(void*(func)(void*), void* args)
{
	ScePthread sceThread;
	return scePthreadCreate(&sceThread, NULL, func, args, "Client Thread") == 0;
}

void* clientHandler(void* args)
{
	struct sockaddr_in client = *(struct sockaddr_in*)args;
	
	int locClientSocketFd = clientSockFd;
	char clientIP[16];//IPv4
	bool gotUnknownCommand = true;
	command_s *localCommands = commands;
	int localCommandsLength = lenOfCommands;
	clientIp(&client.sin_addr, clientIP);
	//NOTIFY(MSG_CLIENT_CONNECED, clientIP);
	for (INFINITE)
	{
			char bufferOfClient[MAX_RECEIVE_LENGTH] = {0};
			int lenOfReceivedData = receiveFromClient(locClientSocketFd, bufferOfClient, MAX_RECEIVE_LENGTH);
			
			if (lenOfReceivedData < 1)//Client Disconnected ?
			{
				//NOTIFY(MSG_CLIENT_DISCONNECED, clientIP);
				scePthreadExit(NULL);
			}

			if (bufferOfClient[0] == 'q')
			{
				quitCommandHandler();
				closeSocket(locClientSocketFd);
				//NOTIFY(MSG_CLIENT_DISCONNECED, clientIP);
				scePthreadExit(NULL);
			}
			for (size_t i = 0; i < localCommandsLength; i++) 
			{
		    	if(localCommands[i].commandChar == bufferOfClient[0] && localCommands[i].minLength <= lenOfReceivedData  && localCommands[i].handler != NULL)
		        {
		            localCommands[i].handler(bufferOfClient, lenOfReceivedData);
					gotUnknownCommand = false;
					break;
		        }
		    }
			if (gotUnknownCommand){
				unknownCommandHandler();
			}
			gotUnknownCommand = true;
	}
	return NULL;
}

void* ps4api(void* td) {
	initSockets();
	initSysUtil();
	struct sockaddr_in clientStruct;
	int clientSocketMonitor = -1;
	for (INFINITE) 
	{
		
		clientSockFd = acceptClient(&clientStruct);
		if (clientSocketMonitor != -1)
		{
			abortSendRecv(clientSocketMonitor);
			closeSocket(clientSocketMonitor);
		}
		clientSocketMonitor = clientSockFd;
		if (!createThread(clientHandler, &clientStruct))
			notify(MSG_CLIENT_THREAD_ERROR);
	}
	closeSockets();
	return 0;
}