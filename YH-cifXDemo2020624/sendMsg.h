#ifndef _SENDMSG_H_
#define _SENDMSG_H_

//send struct
typedef struct snedData
{
	short netid;
	short type;
	u_long dIP;
	u_long SIP;
	short flag;
	short length;
	char cmd[0];
}sdata;

void sendMessage(void *args);
int SendData(SOCKET sock, char* data, short len);


#endif // !_SENDMSG_H_

