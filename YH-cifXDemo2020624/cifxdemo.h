#ifndef		__CIFXDEMO_H_
#define		__CIFXDEMO_H_

#include <stdio.h>
#include <stdint.h> 

#define CIFXNum 6

extern HANDLE hDriver;
extern char CarAgreement[12];

//通道级
typedef struct channel
{
    char channel_Firmware[30]; //板卡的协议
    char channel_FirmwareCode; //协议代号(参考协议文档)
    char channel_channelID;    //通道ID

}CIFXchannel;


//板卡级
typedef struct cifx
{
    SOCKET   ClientID; //占用客户端FD
    char   CIFX_BoardID; //板卡的ID
    char   CIFX_State;//板卡的状态,0x00 为不在位/0x0001为空闲/0x0002为占用/0x0004故障
    //char   TestSuiteID; //测试套件ID
   // char   TestCases; //测试例
    char   TestSta; //测试状态 
    char   CIFX_BoardName[10]; //板卡的标识符 "cifx0"
    CIFXchannel CIFX_Firmware[CIFXNum];

}CIFXBoard[CIFXNum];



extern CIFXBoard cifxbd;


//测试卡详细信息查询响应
typedef struct cifx1
{
    char    Request;   //请求成功
    char    Slot;     //卡槽
    char    CarType;  //卡的类型
    char    CarState; //卡的状态
    short   CarFirmware; //板卡的协议ID(也是固件)
    //unsigned long   Addr;  //地址信息
   // short   Port;   //端口信息

}CarMoreDat;



//通道句柄结构体
typedef struct ChannelSate
{
    HANDLE CxifChannel;

}cifxChannel;



extern  struct ChannelSate cifxChannelSate[6];


//测试仪详细资源查询请求
CarMoreDat* QueryTesterMoreResource(int slot);
//测试卡信息查询请求
void QueryTesterResource(struct cifx* code);
//枚举板卡的信息
char* EnumBoardDemo(HANDLE hDriver);
//打开板卡
char* ApplyforTesterOpen(HANDLE hDriver, int szBoard, uint32_t ulChannel, struct cifx* code);
//关闭板卡
char* ApplyforTesterClose(HANDLE hDriver, int szBoard, uint32_t ulChannel, struct cifx* code);
//接收板卡X的数据
int ReadDatToBoard(HANDLE hChannel, char* dat, int len, uint32_t ulOffset);
//发送指令给板卡X
int WriteDatToBoard(HANDLE hChannel, char* Senddat, int len, uint32_t ulOffset);


#endif // !1



