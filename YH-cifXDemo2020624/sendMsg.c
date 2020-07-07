#include <winsock2.h>
#include <pthread.h>
#include "sendMsg.h"
#pragma warning(disable:4996)
#define BUF_SIZE 2048

extern int flag;
extern short length;
extern SOCKET client_s;
extern pthread_mutex_t mutex;

void sendMessage(void *args)
{
	HANDLE hMapFile;
	LPCTSTR pBuf;
	char szName[] = "MySharedMem";

	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, szName);
	if (!hMapFile || INVALID_HANDLE_VALUE == hMapFile)
	{
		printf("Could not create file mapping object \n");
		return 1;
	}
	
	while (1)
	{
		//printf("waitting send message\n");
		//check shared memory
		//send 
		if (1 == flag)
		{
			pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
			if (!pBuf)
			{
				printf("Could not map view of file \n");
				CloseHandle(hMapFile);
				return 1;
			}
			int ret = SendData(client_s, pBuf, length);
			if (ret < 0)
			{
				printf("Send error\n");
				return 1;
			}
			memset(pBuf, 0, BUF_SIZE);
			flag = 0;
			pthread_mutex_unlock(&mutex);
		}

	}

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
}

int SendData(SOCKET sock, char* data, short len)
{
	int ret;
	ret = send(sock, data, len, 0);
	return ret;
}

