#ifndef _PROCESSMSG_H_
#define _PROCESSMSG_H_
#include <winsock2.h>


//recv struct
typedef struct recvdata
{
	u_short netid;                   //net id 0x0056
	u_short type;                     //message type
	IN_ADDR dIP;                   //dest IP
	IN_ADDR SIP;                   //source IP
	u_short flag;                    //client flag
	u_short length;                 //data length
	u_char cmd[1024];                  //recv data
}data;

//recv suite struct
typedef struct testsuite
{
	u_short SuitelId;               //test suite ID 
	u_short ItemId;                 //test item ID
	u_short status;                  //test status
	u_short parCnt;                 //param count
	char param[512];                 //param
}suite;

//card detail info struct
typedef struct detailInfo
{
	u_short resstatus;
	u_short slotNum;
	u_short cardType;
	u_short cardstatus;
	u_short protocolID;
	u_short port;
	u_long ipaddr;
	u_int rsvd;
}Info;

typedef struct client_Info
{
	SOCKET clients;                    //client fd
	u_long client_ip;                  //client ip
	int age;                       //client age
	int occupyInfo[5];                  //occupy card infomation
}cInfo[5];                             //five clients


//timer thread param
typedef struct param
{
	fd_set allSocket;
}par;

//process message param struct
typedef struct processMsgparam
{
	SOCKET s;
	fd_set allSockset;
	data *d1;
}propar;

//test status struct
typedef struct testStatus
{
	u_short SubType;
	u_short Resv;
	u_short suitID;
	u_short testCnt;
	char allstatus[512];
}status;

//one param one struct
typedef struct testparam
{
	u_int parId;           
	u_int param;           
}tpar;

typedef struct sendSignalResult
{
	u_short suiteId; //≤‚ ‘Ã◊
	u_short itemId;  //≤‚ ‘¿˝
	u_short tStatus; //≤‚ ‘◊¥Ã¨
	u_short errcode; //¥ÌŒÛ¬Î
}signalRes;

typedef struct testResult
{
	u_short testId;
	u_short result;
}testresult;

typedef struct testItemId
{
	u_short itemId;
}testitemid;

typedef struct sendAllResult
{
	u_short SubType;
	u_short Resv;
	u_short suitID;
	u_short testCnt;
	testresult tres[0];
}allRes;

void processMsg (void *arg);
int recvMessage(void *param);


#endif // !_PROCESSMSG_H_

