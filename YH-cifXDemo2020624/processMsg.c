#include <pthread.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include "processMsg.h"
#include "cifxdemo.h"
#include "sendMsg.h"
#pragma warning(disable:4996)

#define CLIENTNUM 5
#define CARDNUM 6

#define BUF_SIZE 2048

//用来计数测试例的进度个数
int TestNum = 0;
int flag = 0;           //shared memory flag
u_short length = 0;           //send data length
SOCKET client_s;                 //client socket
CIFXBoard cifxbd;                //create card info struct array

pthread_mutex_t mutex;

LPCTSTR pBuf;    //shared memory pointer
cInfo cinfo;       //client infomation struct array

void checkAge(void* arg)
{
	par *p = (par*)arg;
	fd_set allSocket = p->allSocket;
	while (1)
	{
		Sleep(1000);             //1s
		for (int i = 0; i < CLIENTNUM; i++)
		{
			if (-1 == cinfo[i].age)
			{
				continue;
			}
			else
			{
				//20s
				if (cinfo[i].age < 20)
				{
					cinfo[i].age = cinfo[i].age + 1;
				}
				else
				{
					printf("Client connect timeout\n");
					FD_CLR(cinfo[i].clients, &allSocket);
					printf("Left %d client\n", allSocket.fd_count - 1);
					printf("i = %d,fd = %d\n",i, cinfo[i].clients);
					if (0 == closesocket(cinfo[i].clients))
					{
						cinfo[i].age = -1;
						cinfo[i].clients = -1;
						cinfo[i].client_ip = inet_addr("0.0.0.0");
						memset(&(cinfo[i].occupyInfo), 0xF, 5);
					}
					else
					{
						printf("Close socket error\n");
					}
				}
			}
		}
	}

}

//init client info struct array
void initClientArray()
{
	for (int i = 0; i < CLIENTNUM; i++)
	{
		cinfo[i].clients = -1;
		cinfo[i].client_ip = inet_addr("0.0.0.0");
		cinfo[i].age = -1;
		memset(&(cinfo[i].occupyInfo), 0xF, 5);
	}
}

//init board info struct array
void initBoardArray()
{
	//结构体数组初始化
	for (char i = 0; i < CIFXNum; i++)
	{
		cifxbd[i].ClientID = -1;
		cifxbd[i].TestSta = 0xFF;
		//cifxbd[i].TestCases = 0;
		//cifxbd[i].TestSuiteID = i;
		cifxbd[i].CIFX_BoardID = 0xFF;

		
	}

}

u_short  ConvSlotNum(u_short szBoard)
{
	u_short board;
	//关闭总线
	switch (szBoard)
	{
		case 0x01:  board = 0;
			break;

		case 0x02:  board = 1;
			break;

		case 0x04: board = 2;
			break;

		case 0x08:  board = 3;
			break;

		case 0x10:  board = 4;
			break;

		case 0x20:   board = 5;
			break;

		default: printf("ApplyforTester szBoard Err : %d\r\n", board);
			break;
	}

	return board;
}

//char to int (char > 127 is ok)
int getInt(char chVar)
{
	//将字符串右移7位，观察结果是否为1
	int nVal = chVar >> 7;
	//高位为1
	if (-1 == nVal)
	{
		char cTemp = chVar & 0x7f; //将高位1变为0
		nVal = (int)cTemp + 128;
	}
	else
	{
		nVal = (int)chVar;
	}
	return nVal;
}

int createSharedMem()
{
	HANDLE hMapFile;
	char szName[] = "MySharedMem";
	pthread_t t2;

	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, szName);
	if (NULL == hMapFile)
	{
		printf("Could not create file mapping object \n");
		return 1;
	}
	pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
	if (NULL == pBuf)
	{
		printf("Could not create file mapping object \n");
		CloseHandle(hMapFile);
		return 1;
	}

	//create send message thread
	while (1)
	{
		if (pthread_create(&t2, NULL, sendMessage, NULL) != 0)
		{
			printf("create sendmsg thread failed\n");
		}
		else
		{
			break;
		}
	}
	return 0;
}

//select and recv msg
int recvMessage(void *param)
{
	int ret;
	int nready;
	int sock = (int)param;
	pthread_t timer;
	fd_set allSockset;              //all fd set 
	fd_set readset;                 //readable fd set
	fd_set writeset;                //writeable fd set
	par p1;

	FD_ZERO(&allSockset);           //clean fd set
	FD_SET(sock, &allSockset);


	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	//ioctlsocket(sock, FIONBIO, &ul);             //set sock no block

	char *response;
	response = (char *)malloc(1024);
	memset(response, 0, 1024);                    //recv buff set zero

	pthread_mutex_init(&mutex, NULL);          //init the mutex

	initClientArray();
	initBoardArray();
	createSharedMem();

	p1.allSocket = allSockset;                    //set thread param

	//create timer thread
	while (1)
	{
		if (pthread_create(&timer, NULL, checkAge, (void *)&p1) != 0)
		{
			printf("Create timer thread failed\n");
		}
		else
		{
			break;
		}
	}

	//start select listen
	while (1)
	{
		FD_ZERO(&readset);                //clean readable set
		FD_ZERO(&writeset);               //clean writeable set
		readset = allSockset;
		writeset = allSockset;

		nready = select(0, &readset, NULL, NULL, &timeout);
		if (SOCKET_ERROR == nready)
		{
			printf("select error\n");
			return 0;
		}

		if (nready > 0)
		{
			for (int i = 0; i < allSockset.fd_count; i++)
			{
				SOCKET s = allSockset.fd_array[i];
				if (FD_ISSET(s, &readset))
				{
					printf("is exist\n");
					if (s == sock)
					{
						SOCKADDR_IN addrClient;
						int nLen = sizeof(addrClient);
						SOCKET clientSock = accept(s, (SOCKADDR *)&addrClient, &nLen);
						FD_SET(clientSock, &allSockset);
						printf("New client connect\n");
						printf("Has connected %d client\n", allSockset.fd_count - 1);
					}
					else
					{
						int size = sizeof(data);
						ret = recv(s, response, size, 0);
						if (SOCKET_ERROR == ret)
						{
							DWORD err = WSAGetLastError();
							if (WSAECONNRESET == err)
							{
								printf("client is close\n");
							}
							else
							{
								printf("recv() failed\n");
							}
							FD_CLR(s, &allSockset);
							printf("Left %d client\n", allSockset.fd_count - 1);
							break;
						}
						if (0 == ret)
						{
							printf("client quit\n");
							FD_CLR(s, &allSockset);
							printf("Left %d client\n", allSockset.fd_count - 1);
							break;
						}
						data *d1 = NULL;
						d1 = (data *)malloc(sizeof(data));
						memset(d1, 0, sizeof(data));
						memcpy(d1, response, size);

						d1->cmd[ntohs(d1->length) + 1] = '\0';

						//hex print
						/*for (int i = 0; i < 40 + 16; i++)
						{
							printf("%02x\n", getInt(response[i]));
						}*/

						printf("netID = %04x, type = %04x, dIP = %s, sIP = %s, flag = %04x, length = %04x, DATA = %s\n", \
							ntohs(d1->netid), ntohs(d1->type), inet_ntoa(d1->dIP), inet_ntoa(d1->SIP), ntohs(d1->flag), ntohs(d1->length), d1->cmd);

						propar ppar;               //process message param struct
						ppar.d1 = d1;
						ppar.s = s;
						ppar.allSockset = allSockset;

						pthread_t proMsg;
						while (1)
						{
							if (pthread_create(&proMsg, NULL, processMsg, (void *)&ppar) != 0)
							{
								printf("Create process message thread failed\n");
							}
							else
							{
								break;
							}
						}
						//processMsg(s,d1,allSockset);

					}
				}
			}
		}
	}
}

//char to int ("0x.." to int)
int getIntRcv(char *rcv_data, u_short len, data* d1)
{
	rcv_data = (char*)malloc(len * 2 + 2);
	sprintf(rcv_data, "0x");
	for (int i = 0; i < len; i++)
	{
		printf("cmd = %x\n", getInt(d1->cmd[i]));
		sprintf(rcv_data, "%s%02x", rcv_data, getInt(d1->cmd[i]));
	}
	printf("rcv = %s\n", rcv_data);
	int tempInt;
	sscanf(rcv_data, "%x", &tempInt);
	return tempInt;
}

//send func
void writeSharedMem (SOCKET s, void *send_Data, u_short len, data *d1)
{
	sdata *sd1 = NULL;     //send data struct
	char *sendbuf = NULL;     //send buff

	sendbuf = (char *)malloc(sizeof(sdata) + len);
	sd1 = (sdata *)malloc(sizeof(sdata) + len);
	sd1->netid = d1->netid;
	sd1->type = d1->type;
	sd1->dIP = inet_addr(inet_ntoa(d1->SIP));
	sd1->SIP = inet_addr(inet_ntoa(d1->dIP));
	sd1->flag = htons(0xFFFF);
	sd1->length = htons(len);
	memcpy(&(sd1->cmd), send_Data, len);                 //copy data to sata->char
	memcpy(sendbuf, sd1, sizeof(sdata) + len);           //copy sdata to sendbuff
	if (0 == flag)
	{
		pthread_mutex_lock(&mutex);
		memcpy(pBuf, sendbuf, sizeof(sdata) + len);
		client_s = s;
		length = sizeof(sdata) + len;
		flag = 1;
	}
	
	//ret = SendData(s, sendbuf, sizeof(sdata) + len);          //send to client
	//return ret;
	//copy data to shared memory
}

//fill all card status array
void fillInfoarray(SOCKET s, SSIZE_T index, char allcard[12])
{
	for (SSIZE_T i = 0; i < CARDNUM; i++)
	{
		if (allcard[index] == getInt(cifxbd[i].CIFX_Firmware[0].channel_FirmwareCode))
		{
			int status = getInt(cifxbd[i].CIFX_State);
			printf("Status %d\r\n",status);
			if (0x00 == status)
			{
				allcard[index + 1] = 0x00;
			}
			else if (0x02 == status)
			{
				//check socket fd
				if (cifxbd[i].ClientID == s)
				{
					allcard[index + 1] = 0x02;              //Self occupation
				}
				else
				{
					allcard[index + 1] = 0x03;             //other client occupation
				}
			}
			else if (0x01 == status)
			{
				allcard[index + 1] = 0x01;                  //card is idle
				printf("index %d\r\n", index+1);
			}
			else if (0x04 == status)
			{
				allcard[index + 1] = 0x04;
			}
			else
			{
				printf("Invaild Status\n");
			}
		}
	}
	
}

//get all card status
void getStatus(char allcard[12], SOCKET s)
{
	for (size_t i = 0; i < 12; i++)
	{
		switch (i)
		{
			case 0:
			{
				allcard[i] = CarAgreement[i];
				fillInfoarray(s, i, allcard);
				break;
			}
			case 2:
			{
				allcard[i] = CarAgreement[i];
				fillInfoarray(s, i, allcard);
				break;
			}
			case 4:
			{
				allcard[i] = CarAgreement[i];
				fillInfoarray(s, i, allcard);
				break;
			}
			case 6:
			{
				allcard[i] = CarAgreement[i];
				fillInfoarray(s, i, allcard);
				break;
			}
			case 8:
			{
				allcard[i] = CarAgreement[i];
				fillInfoarray(s, i, allcard);
				break;
			}
			case 10:
			{
				allcard[i] = CarAgreement[i];
				fillInfoarray(s, i, allcard);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

//traversal the cardinfo struct and find test status
int findStatus(u_short id)
{
	for (SSIZE_T i = 0; i < CARDNUM; i++)
	{
		//获取的测试例
		//if (id == getInt(cifxbd[i].TestCases))
		{

		}
	}


}

//modbus 读取测试例
u_short  modbusTestRead(u_short TID)
{
	u_short ret = 0;
	char	REarr[36] = { 0 };
	char	W_hold[8] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08 };
	char	W_Scoils[10] = { 0X01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01 };


	TestNum++;  //测试例个数递增(目前读取就是四个测试例子)
	switch (TID)
	{

		case 1:
		{
			//先读取
			ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr, sizeof(REarr), 0);
			//正常写多个线圈   
			WriteDatToBoard(cifxChannelSate[1].CxifChannel, &W_Scoils, sizeof(W_Scoils), 0);
		}
			break;
		case 2:
		{
			ret = ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr, sizeof(REarr), 10);
			//正常写保持寄存器  
			ret = WriteDatToBoard(cifxChannelSate[1].CxifChannel, &W_hold, sizeof(W_hold), 10);
		}
			break;
		case 3:
		{
			//正常读多个线圈
			ret = ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr, sizeof(REarr), 0);
		}
			break;
		case 4:
		{
			//正常读保持寄存器
			ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr, sizeof(REarr), 10);
		}
		break;

		default: printf("TID Err\r\n");
			break;
	}

	printf("ret:	%d\r\n",ret);
	//出错了
	if (ret != 0)
	{
		return 0X55;
	}


}
//modbus 写测试例
u_short modbusTestWrite()
{
	u_short ret = 0;
	char	REarr_1[36] = { 0 };
	char W_hold[8] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08 };
	char W_Scoils[10] = { 0X01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01 };

	char W_hold_1[8] = { 0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18 };
	char W_Scoils_1[10] = { 0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00 };

	//先读取
	ret = ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr_1, sizeof(REarr_1), 0);
		
	//正常写多个线圈   
	ret = WriteDatToBoard(cifxChannelSate[1].CxifChannel, &W_Scoils, sizeof(W_Scoils), 0);

	//先读取
	ret = ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr_1, sizeof(REarr_1), 10);
	//异常写多个线圈   
	ret = WriteDatToBoard(cifxChannelSate[1].CxifChannel, &W_Scoils_1, sizeof(W_Scoils_1), 10);


	//先读取
	ret = ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr_1, sizeof(REarr_1), 20);
	//正常写保持寄存器  
	ret = WriteDatToBoard(cifxChannelSate[1].CxifChannel, &W_hold, sizeof(W_hold), 20);
	//再次读取查看数据

	//先读取
	ret = ReadDatToBoard(cifxChannelSate[1].CxifChannel, &REarr_1, sizeof(REarr_1), 28);
	//异常写保持寄存器
	ret = WriteDatToBoard(cifxChannelSate[1].CxifChannel, &W_hold_1, sizeof(W_hold_1), 28);

	printf("ret:	%d\r\n", ret);
	//出错了
	if (ret != 0)
	{
		return 0XFF;
	}


}


//modbus主板板卡协议分析
void  paraDatAnalysis(tpar* tp,signalRes* sr,u_short count, u_short TID)
{


	for (SSIZE_T i = 0; i < count; i++)
	{
		int id = ntohl(tp[i].parId);          //param id
		int par = ntohl(tp[i].param);  //本地字节序转换
		printf("ID:	%d\r\n",id);
		
		switch (id)
		{
			//slaveID
			case 0x00000000:
			{

			}
			break;
			//FunCode
			case 0x00000001:
			{

				printf("Funcation M：%d\r\n",par);
				//写入多个保持寄存器
				if(par == 0x16)
				{
					u_short tem1 = modbusTestWrite();
					printf("tem1 :%d\r\n",tem1);
					sr->errcode = htons(tem1);

				}

				else if (par == 0x03)   //读取多个保持寄存器功能码
				{
					u_short tem = 0;
					tem = modbusTestRead(TID);
					printf("tem :%d\r\n", tem);
					sr->errcode = htons(tem);
					//测试到读取就是已经结束了
					if (TestNum >= 3  || TID == 4)
					{
						TestNum = 0;
						sr->tStatus = htons(0x10);  //测试结束
					}
						
					else
					{
						sr->tStatus = htons(0x01);  //测试中
					}

				}

			}
			break;
			//start Address
			case 0x00000002:
			{
			}
			break;
			//number
			case 0x00000003:
			{

			}
			break;
			//data
			case 0x00000004:
			{

			}
			break;

		}
	}

}


char* parr;
//u_short send_data11;
void processMsg(void *arg)
{
	u_short send_data = 0;
	uint8_t Notfnd = 0;

	//sdata *sd1 = NULL;     //sned data struct
	propar *prop = (propar *)arg;
	data *d1 = prop->d1;
	SOCKET s = prop->s;
	fd_set allSockset = prop->allSockset;
	char *rcv_data = NULL;
	char allcard[12] = { 0 };        //all card info and status

	u_short netID = ntohs(d1->netid);
	u_short MsgType = ntohs(d1->type);
	u_short len = ntohs(d1->length);
	
	
	//check protocol number
	if (0x56 != netID)
	{
		printf("Is Not our protocol!\n");
		return;
	}

	//judge  message type
	switch (MsgType)
	{
		//request connect
		case 1:
		{
			//u_short send_data;
			u_short sign = 0;          //can connect?
			printf("request connect\n");
			int tempInt = getIntRcv(rcv_data, len, d1);
			if (0x0000 == tempInt)
			{
				u_long cip = inet_addr(inet_ntoa(d1->SIP));
				for (int i = 0; i < CLIENTNUM; i++)
				{
					if (cinfo[i].client_ip == cip)
					{
						//if (FD_ISSET(s, &allSockset))
						//{
						//	FD_CLR(s, &allSockset);
						//	printf("Left %d client\n", allSockset.fd_count - 1);
						//}
						sign = 1;           //the client can connect
						cinfo[i].clients = s;
						send_data = htons(0x0001);
						goto send;
					}
				}

				for (int i = 0; i < CLIENTNUM; i++)
				{
					if (-1 == cinfo[i].clients)
					{
						sign = 1;                    //have space
						cinfo[i].clients = s;
						cinfo[i].client_ip = cip;
						cinfo[i].age = 0;
						break;
					}
					
					
				}
				
			send:
				//no space
				if (0 == sign)
				{
					printf("no space to connect new client\n");
					send_data = htons(0x0000);
					FD_CLR(s, &allSockset);
					printf("Left %d client\n", allSockset.fd_count - 1);
				}
				else
				{
					send_data = htons(0x0001);
				}
				u_short dlen = sizeof(send_data);
				writeSharedMem(s, &send_data, dlen, d1);

			}
			free(rcv_data);
			rcv_data = NULL;
			break;
		}
		//query the card, query all & one
		case 2:
		{
			printf("query card\n");
			int tempInt = getIntRcv(rcv_data, len, d1);
			//all
			if (0xff00 == tempInt)
			{
				printf("get all\n");
				
				
				EnumBoardDemo(hDriver);
				//serch card info status
				getStatus(allcard, s);

				for (size_t i = 0; i < 12; i++)
				{
					if (i < 6)
					{
						allcard[i] = 0x01;
					}
					else
					{
						allcard[i] = 0x00;
					}
				}
				writeSharedMem(s, &allcard, sizeof(allcard), d1);
			}
			free(rcv_data);
			rcv_data = NULL;
			break;
		}
		//get detail info  详细
		case 258:
		{
			//one
			int tempInt = getIntRcv(rcv_data, len, d1);
			if (!(0xFF00 & tempInt))
			{
				printf("get one card info\n");
				u_short Temslot = 0x00FF & tempInt;              //get slot number
				u_short slot = ConvSlotNum(Temslot); //协议槽号转换

				Info *info = NULL;
				info = (Info*)malloc(sizeof(Info));

				info->resstatus = htons(0x00);         //request success
				info->slotNum = htons(slot);                 //slot number
				info->cardType = htons(slot);              //card type
				info->protocolID = htons(getInt(cifxbd[slot].CIFX_Firmware[0].channel_FirmwareCode));             //card protocol ID
				int status = getInt(cifxbd[slot].CIFX_State);              //char(status) to int
				if (0x00 == status)
				{
					info->cardstatus = htons(0x00);
				}
				else if (0x01 == status)
				{
					info->cardstatus = htons(0x01);
				}
				else if (0x02 == status)
				{
					if (cifxbd[slot].ClientID == s)
					{
						info->cardstatus = 0x02;
					}
					else
					{
						info->cardstatus = 0x03;
					}
				}
				else if (0x04 == status)
				{
					info->cardstatus = 0x04;
				}
				else
				{
					printf("Invalid status\n");
				}
				
				info->ipaddr = inet_addr("192.168.1.5");             //card ip addr
				info->port = htons(0x6666);                          //card port
				info->rsvd = htonl(0xFFFFFFFF);

				writeSharedMem(s, info, sizeof(Info), d1);
				free(info);
				info = NULL;
			}
			free(rcv_data);
			rcv_data = NULL;
			break;
		}
		//apply/release for resources
		case 3:
		{
			printf("apply or release resources\n");
			//u_short send_data;
			int tempInt = getIntRcv(rcv_data, len, d1);
			//apply  申请
			if (0x0100 == (tempInt & 0xFF00))
			{
				printf("apply resource\n");
				u_short slot = tempInt & 0x00FF;        //card type
				u_short cardtype = ConvSlotNum(slot); //协议槽号转换
				printf("slot:%d", slot);
				printf("cardtype:%d", cardtype);
				for (SSIZE_T i = 0; i < CARDNUM; i++)
				{
					if (cardtype == getInt(cifxbd[i].CIFX_BoardID))
					{
						if (0x01 == getInt(cifxbd[i].CIFX_State)) //空闲
						{														
							//占用操作
							//char *parr = ApplyforTesterOpen(hDriver, cardtype, 0, &cifxbd[i])
							parr = ApplyforTesterOpen(hDriver, cardtype, 0, &cifxbd[i]);
							//赋值占用成功
							if (parr[0] == 0x02)
							{
								//查找本身客户端ID
								for (SSIZE_T i = 0; i < CLIENTNUM; i++)
								{
									//对比套接字找到自己客户端的结构体
									if (s == cinfo[i].clients)
									{
										//赋值占用信息
										cinfo[i].occupyInfo[cardtype] = cardtype;

										break;
									}
								}

								send_data = send_data | 0x0200;//set result
							}
							//底层板卡打开失败 错误码4
							//if (parr[0] == 0x84)	
							else
							{
								send_data = send_data | 0X8500;
								printf("send_data  0x84:%d",send_data);
							}

							cifxbd[i].ClientID = s; //赋值ID           
						}
						else if (0x02 == getInt(cifxbd[i].CIFX_State))
						{
							//判断被谁占用
							if (s == cifxbd[i].ClientID)
							{
								send_data = send_data | 0x0200;               //set result
							}
							else
							{
								send_data = send_data | 0x8100;                 //error code 1,other client occupy the board
							}
						}
						else if (0x04 == getInt(cifxbd[i].CIFX_State))
						{
							send_data = send_data | 0x8200;                    //error code 2,board fault
						}
						else
						{
							send_data = send_data | 0x8300;                     //error code 3, other error
						}
						Notfnd = 0;
						break;
					}
					else
					{
						Notfnd = 1;
						
						
					}
				}

				if (Notfnd == 1)
				{
					send_data = send_data | 0x8400;                        //error code 4, no this card error
				}

				send_data = send_data | slot ;    //set cardtype
				send_data = send_data & 0;
				send_data = 0x0201;
				u_short lenz = sizeof(send_data);
				u_short tempdat = htons(send_data); //转换网络字节序
				writeSharedMem(s, &tempdat, lenz, d1);
			}
			//release  释放
			else if (0x0400 == (tempInt & 0xFF00))
			{
				printf("release resource\n");
				u_short slot = tempInt & 0x00FF;        //card type is slot number
				u_short cardtype = ConvSlotNum(slot); //协议槽号转换
				u_short send_data = 0;
				for (SSIZE_T i = 0; i < CARDNUM; i++)
				{
					if (cardtype == getInt(cifxbd[i].CIFX_BoardID))
					{
						if (s == cifxbd[i].ClientID)
						{
							//释放板卡
							char *c = ApplyforTesterClose(hDriver, cardtype, 0, &cifxbd[i]);
							if (c[0] == 0X05)
							{
								//查找本身客户端ID
								for (SSIZE_T i = 0; i < CLIENTNUM; i++)
								{
									//对比套接字找到自己客户端的结构体
									if (s == cinfo[i].clients)
									{
										//赋值占用信息
										cinfo[i].occupyInfo[cardtype] = 0xF;
									}
								}
								send_data = send_data | 0x0500; //set result
							}
							//板卡释放错误  错误码 5
							else
							{
								send_data = send_data | 0xF500;		//error code 5, Close err;
							}
            
						}
						else
						{
							send_data = send_data | 0xF100;                   //error code 1,not occupy this card
						}

						Notfnd = 0;
						break;
					}
					else
					{
						Notfnd = 1;
					
					}
				}
				if (Notfnd == 1)
				{
					send_data = send_data | 0xF400;                     //error code 4, no this card type
				}
				
				send_data = send_data |slot ;    //set cardtype

				send_data = send_data & 0;
				send_data = 0x0501;
				u_short lenz = sizeof(send_data);
				u_short tempdat = htons(send_data); //转换网络字节序

				writeSharedMem(s, &tempdat, lenz, d1);
			}

			break;
		}
		//Get test suite (Not use)
		case 4:
		{
			printf("Get test suite\n");

			break;
		}
		//stop test (Not use)
		case 5:
		{
			printf("Stop test\n");
			int tempInt = getIntRcv(rcv_data, len, d1);
			if (0x0001 == tempInt)
			{
				//close test and send the status

			}
			break;
		}
		//response hand shake
		case 16:
		{
			printf("response hand shake\n");
			char *tempdata = (char *)malloc(2);
			memcpy(tempdata, &(d1->cmd), 2);
			int tempInt = getIntRcv(rcv_data, 2, d1);
			//set client age = 0
			for (int i = 0; i < CLIENTNUM; i++)
			{
				if (s == cinfo[i].clients)
				{
					cinfo[i].age = 0;
				}
			}
			printf("%d\n", tempInt);
			if (0x0101 == tempInt)
			{
				printf("get card status\n");
				getStatus(allcard, s);

				writeSharedMem(s, &allcard, sizeof(allcard), d1);
			}
			else if (0x0102 == tempInt)
			{
				printf("get test status\n");
				if (len > 512)
				{
					printf("Too many test item, no enough space\n");
					return;
				}
				else
				{
					rcv_data = (char *)malloc(len);
					memcpy(rcv_data, &(d1->cmd), len);
					status *ts = NULL;
					ts = (status *)malloc(sizeof(status));
					memcpy(ts, rcv_data, len);
					u_short count = ntohs(ts->testCnt);              //test item count
					u_short itemlength = count * 2;                  //item id byte
					u_short lenz = sizeof(allRes) + sizeof(testresult) * count;

					allRes *ar = (allRes *)malloc(lenz);

					testitemid *tid = (testitemid *)malloc(sizeof(testitemid) * count);            //recv testId struct array
					memcpy(tid, &(ts->allstatus), itemlength);

					ar->SubType = ts->SubType;
					ar->Resv = ts->Resv;
					ar->suitID = ts->suitID;
					ar->testCnt = ts->testCnt;
					
					for (SSIZE_T i = 0; i < count; i++)
					{
						ar->tres[i].testId = tid[i].itemId;
						u_short id = ntohs(tid[i].itemId);
						if (0 == id)
						{
							printf("invalid test id\n");
							ar->tres[i].result = htons(0x1010);              //error code 0x1010 ,no this test id
							continue;
						}
						int res = findStatus(id);
						switch (res)
						{
							//test success over
							case 0:
							{
								ar->tres[i].result = htons(0x0000);
								break;
							}
							//test not run
							case 1:
							{
								ar->tres[i].result = htons(0x1001);
								break;
							}
							//test card error
							case 2:
							{
								ar->tres[i].result = htons(0x1002);
								break;
							}
							//timeout error
							case 3:
							{
								ar->tres[i].result = htons(0x1004);
								break;
							}
							//run error
							case 4:
							{
								ar->tres[i].result = htons(0x1008);
								break;
							}
							//other error
							default:
							{
								ar->tres[i].result = htons(0x1111);
								break;
							}
						}
					}

					writeSharedMem(s, ar, lenz, d1);
					

					free(ts);
					free(tid);
					free(ar);
					ts = NULL;
					tid = NULL;
					ar = NULL;
				}
			}
			free(rcv_data);
			free(tempdata);
			rcv_data = NULL;
			tempdata = NULL;
			break;
		}
		//Get test case
		case 512:
		{
			printf("start test\n");
			if (len > 512)
			{
				printf("recv buff space is not enough\n");
			}
			else
			{
				rcv_data = (char *)malloc(len);
				memcpy(rcv_data, &(d1->cmd), len);            //copy data to char array
				suite * s1 = NULL;
				s1 = (suite*)malloc(sizeof(suite));
				memcpy(s1, rcv_data, len);                       //copy char array to testdata struct
				u_short count = ntohs(s1->parCnt);
				u_short TID = ntohs(s1->ItemId);   //测试例
				u_short SID = ntohs(s1->SuitelId); //测试套
				u_short parlength = count * 8;

				tpar *tp = NULL;
				tp = (tpar *)malloc(sizeof(tpar) * count);                   //struct array
				memcpy(tp, &(s1->param), parlength);

				signalRes* sr = (signalRes *)malloc(sizeof(signalRes));

				sr->itemId = s1->ItemId;
				sr->suiteId = s1->SuitelId;

				printf("SID: %d\r\n",SID);
				printf("TID: %d\r\n" ,TID);
				switch (SID)
				{
					//modbus
					//case 0: 
						break;
					case 1: paraDatAnalysis(tp, sr, count,TID);
						break;
					case 2:
						break;
					case 3:
						break;
					case 4:
						break;
					case 5:
						break;

					default: printf("paraDatAnalysis Err\r\n");
						break;
				}
				
				//写到共享内存里
				writeSharedMem(s, sr, sizeof(signalRes), d1);

				free(s1);
				free(tp);
				tp = NULL;
				s1 = NULL;
			}
			free(rcv_data);
			rcv_data = NULL;
			break;
		}
		default:
		{
			printf("unknown message type\n");
			break;
		}
	}

	free(d1);
	d1 = NULL;
}

