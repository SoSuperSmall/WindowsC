
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <pthread.h>
#include "processMsg.h"
#include "sendMsg.h"
#pragma warning(disable:4996)

int main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	u_long ul = 1;

	err = WSAStartup(wVersionRequested, &wsaData);
	if (0 != err)
	{
		printf("Socket1.1 init error,exit!\n");
		return 0;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return 0;
	}

	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("invalide socket,exit\n");
		WSACleanup();
		return 0;
	}

	SOCKADDR_IN addr_in;
	memset(&addr_in, 0, sizeof(addr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.S_un.S_addr = INADDR_ANY;
	addr_in.sin_port = htons(6666);
	
	int ret = bind(sock, (SOCKADDR *)&addr_in, sizeof(SOCKADDR));
	if (SOCKET_ERROR == ret)
	{
		printf("bind error\n");
		WSACleanup();
		return 0;
	}

	ret = listen(sock, 8);
	if (SOCKET_ERROR == ret)
	{
		printf("listen failed\n");
		return 0;
	}

	printf("Server start listen...\n");

	//create new thread deal the message
	pthread_t t1;

	//create recv and process message thread
	while (1)
	{
		if (pthread_create(&t1, NULL, recvMessage, (void *)sock) != 0)
		{
			printf("create recvmsg thread failed\n");
		}
		else
		{
			break;
		}
	}

	pthread_join (t1, NULL);
	
	return 0;
}

