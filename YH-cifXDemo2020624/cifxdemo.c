#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#include "cifxuser.h"
#include "cifxErrors.h"
#include "cifxdemo.h"


#pragma comment(lib,"ws2_32.lib")		//加载ws2_32.dll
#pragma comment(lib,"Winmm.lib")	//For timeSetEvent


#define PACKET_WAIT_TIMEOUT 20
#define IO_WAIT_TIMEOUT     10
#define HOSTSTATE_TIMEOUT   5000

//所有板卡的句柄
HANDLE hDriver = NULL;

//资源申请响应结果
char ApplyforTesterArropen[2] = { 0 };
//资源申请释放结果
char ApplyforTesterArrclose[2] = { 0 };

//测试仪资源查询响应
char CarAgreement[12] = { 0 };

cifxChannel cifxChannelSate[6] = { NULL };


//发送数据格式
typedef struct request
{
    short nid;
    short type;
    unsigned long dIP;
    unsigned long sIP;
    short flage;
    short length;
    char* data;
}rdata;
rdata sendat;

//接收数据格式
typedef struct netdata
{
    short netid;
    short type;
    IN_ADDR dIP;
    IN_ADDR SIP;
    short flag;
    short length;
    char cmd[1024];
}data;

/*****************************************************************************/
/*! Displays a hex dump on the debug console (16 bytes per line)
*   \param pbData     Pointer to dump data
*   \param ulDataLen  Length of data dump                                    */
/*****************************************************************************/
void DumpData(unsigned char* pbData, unsigned long ulDataLen)
{
  unsigned long ulIdx = 0;
  
  //打印数据长度
  printf("DataLen = %d" ,ulDataLen);

  for(ulIdx = 0; ulIdx < ulDataLen; ++ulIdx)
  {
    if(0 == (ulIdx % 16))
      printf("\r\n");

    printf("%02X ", pbData[ulIdx]);
  }
  printf("\r\n");
}

/*****************************************************************************/
/*! Dumps a rcX packet to debug console
*   \param ptPacket Pointer to packed being dumped                           */
/*****************************************************************************/
void DumpPacket(CIFX_PACKET* ptPacket)
{
  printf("Dest   : 0x%08X      ID   : 0x%08X\r\n", ptPacket->tHeader.ulDest,   ptPacket->tHeader.ulId);
  printf("Src    : 0x%08X      Sta  : 0x%08X\r\n", ptPacket->tHeader.ulSrc,    ptPacket->tHeader.ulState);
  printf("DestID : 0x%08X      Cmd  : 0x%08X\r\n", ptPacket->tHeader.ulDestId, ptPacket->tHeader.ulCmd);
  printf("SrcID  : 0x%08X      Ext  : 0x%08X\r\n", ptPacket->tHeader.ulSrcId,  ptPacket->tHeader.ulExt);
  printf("Len    : 0x%08X      Rout : 0x%08X\r\n", ptPacket->tHeader.ulLen,    ptPacket->tHeader.ulRout);

  printf("Data:");
  DumpData(ptPacket->abData, ptPacket->tHeader.ulLen);
}



//板卡协议字符串对比
typedef struct cifxmore
{
    char*   str;
   // void    (*func)(void);
}CifxAgreement;


CifxAgreement CifxAgreementArr[] = {

    { "EtherNet/IP Scanner"}, //主站
    { "EtherNet/IP Adapter"},//从站

    { "EtherCAT Master"},
    { "EtherCAT Slave"},

    { "PROFIBUS-DP Master"},
    { "PROFIBUS-DP Slave"},

    { "CANopen Master"},
    { "CANopen Slave"},

    { "Open Modbus/TCP"}
};



//测试仪详细资源查询请求
CarMoreDat *QueryTesterMoreResource(int slot)
{
    CarMoreDat carmoreinfo;

    switch (slot)
    {
        carmoreinfo.Request = 0;  //请求成功
        carmoreinfo.Slot = slot;  //槽号
        carmoreinfo.CarType = 0;  //卡类型
        //协议ID
        carmoreinfo.CarState = cifxbd[slot].CIFX_Firmware[0].channel_FirmwareCode;  //协议ID
        //carmoreinfo.Port = 502;
        //carmoreinfo.Addr = inet_addr("192.168.1.10");

    }

    return &carmoreinfo;

}


//打开板卡
char *ApplyforTesterOpen(HANDLE hDriver, int szBoard, uint32_t ulChannel, struct cifx* code)
{
    long   lRet = CIFX_NO_ERROR;
    CHANNEL_INFORMATION tChannelInfo = { 0 };
    uint32_t            ulState = 0;

    switch (szBoard)
    {
        case 0:  
            lRet = xChannelOpen(hDriver, "cifx0", ulChannel, &cifxChannelSate[szBoard].CxifChannel);
            break;

        case 1:  
            lRet = xChannelOpen(hDriver, "cifx1", ulChannel, &cifxChannelSate[szBoard].CxifChannel);
            break;

        case 2:  
            lRet = xChannelOpen(hDriver, "cifx2", ulChannel, &cifxChannelSate[szBoard].CxifChannel);
            break;

        case 3: 
            lRet = xChannelOpen(hDriver, "cifx3", ulChannel, &cifxChannelSate[szBoard].CxifChannel);
            break;

        case 4: 
            lRet = xChannelOpen(hDriver, "cifx4", ulChannel, &cifxChannelSate[szBoard].CxifChannel);
            break;

        case 5:  
            lRet = xChannelOpen(hDriver, "cifx5", ulChannel, &cifxChannelSate[szBoard].CxifChannel);
            break;

        default: printf("ApplyforTester szBoard Err : %d\r\n", szBoard);
            break;
    }

    //打开失败
    if (CIFX_NO_ERROR != lRet)
    {
        printf("%s:Error opening Channel!,&szBoard");
        //申请失败
        ApplyforTesterArropen[0] = 0x81;

    }
    else
    {
        //通道打开成功，读取基本信息
        if (CIFX_NO_ERROR != (lRet = xChannelInfo(cifxChannelSate[szBoard].CxifChannel, sizeof(CHANNEL_INFORMATION), &tChannelInfo)))
        {
            printf("%s:Error querying system information block\r\n", &szBoard);
            //申请失败
            ApplyforTesterArropen[0] = 0x82;
                
        }
        else
        {
            printf("szBoard:    %d\r\n", szBoard);
            printf("Device Number    : %u\r\n", tChannelInfo.ulDeviceNumber);
            printf("Serial Number    : %u\r\n", tChannelInfo.ulSerialNumber);
            printf("Firmware         : %s\r\n", tChannelInfo.abFWName);


            //设置主机准备好向现场总线发送信号应用程序已通道打开准备就绪
            lRet = xChannelHostState(cifxChannelSate[szBoard].CxifChannel, CIFX_HOST_STATE_READY, &ulState, HOSTSTATE_TIMEOUT);
            if (CIFX_NO_ERROR != lRet)
            {
                printf("%s:Error setting host ready!\r\n", &szBoard);
                //申请失败
                ApplyforTesterArropen[0] = 0x83;
               
            }
            else
            {
                //如果通道总线未开启，手动启动总线
                lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_ON, &ulState, 0L);
                if (CIFX_NO_ERROR != lRet)
                {
                    printf("%s:Unable to start the filed bus!\r\n", &szBoard);
                    //申请失败
                    ApplyforTesterArropen[0] = 0x84;                               
                   
                }
                else //申请资源成功
                {
                    ApplyforTesterArropen[0] = 0x02; //成功
                    code[szBoard].CIFX_State = 0x02;  //修改板卡占用状态:本机占用
                   
                }


            }

        }


    }

    ApplyforTesterArropen[1] = szBoard; //板卡号

    return ApplyforTesterArropen;

}



//关闭板卡
char *ApplyforTesterClose(HANDLE hDriver, int szBoard, uint32_t ulChannel,struct cifx* code)
{
;
    long   lRet = CIFX_NO_ERROR;
    uint32_t            ulState = 0;

     //关闭总线
    switch (szBoard)
    {
        case 0: 
            lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);
            break;

        case 1: 
            lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);
            break;

        case 2:  lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);        
            break;

        case 3:  
            lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);
            break;

        case 4:  
            lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);
            
            break;
        case 5:  
            lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);
            break;

        default: printf("ApplyforTester szBoard Err : %d\r\n", szBoard);
            break;
    }


    if (CIFX_NO_ERROR != lRet)
    {
        printf("%s:Unable to Close the filed bus!\r\n", &szBoard);
        ApplyforTesterArrclose[0] = 0XF1;
    }
    else
    {
        //设置主机未准备好
        lRet = xChannelHostState(cifxChannelSate[szBoard].CxifChannel, CIFX_HOST_STATE_NOT_READY, &ulState, HOSTSTATE_TIMEOUT);
        if (CIFX_NO_ERROR != lRet)
        {
            printf("%s:Unable to Close the filed bus!\r\n", &szBoard);
            ApplyforTesterArrclose[0] = 0XF2;
        }
        else
        {
            //关闭通道
            lRet = xChannelClose(cifxChannelSate[szBoard].CxifChannel);
            if (CIFX_NO_ERROR != lRet)
            {
                printf("%s:Unable to Close the Channel !\r\n", &szBoard);
                ApplyforTesterArrclose[0] = 0XF3;
            }
            else
            {
                ApplyforTesterArrclose[0] = 0X05;
                code[szBoard].CIFX_State = 0x01;  //修改板卡占用状态:空闲可用
            }
        }

    }


    ApplyforTesterArrclose[1] = szBoard; //板卡号

    return ApplyforTesterArrclose;

}

////测试卡信息查询请求
void  QueryTesterResource(struct cifx *code)
{
    int carNum = 0,ArrNum = 0;
    uint8_t str_size = sizeof(CifxAgreementArr) / sizeof(CifxAgreementArr[0]);

    //判断板卡的数量
    while(strstr(code[carNum].CIFX_BoardName,"cifX") != 0 )
    {
        printf("    Firmware : %s : %s\r\n", code[carNum].CIFX_BoardName, cifxbd[carNum].CIFX_Firmware[0].channel_Firmware);
        printf("    carNum  :   %d\r\n", carNum);
        for (uint8_t i = 0; i < str_size; i++)
        {
           //printf("    CifxAgreementArr : % s\r\n", CifxAgreementArr[i].str);          
            if(strstr(code[carNum].CIFX_Firmware[0].channel_Firmware,CifxAgreementArr[i].str))
            {   
                //赋值测试卡协议
                switch (i)
                {
                    case 0: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x03;
                        
                        break;
                    case 1: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x04;
                        break;
                    case 2: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x05;
                        break;
                    case 3: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x06;
                        break;
                    case 4: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x07;
                        break;
                    case 5: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x08;
                        break;
                    case 6: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x09;
                        break;
                    case 7: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x0A;
                        break;
                    case 8: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x01;
                        break;
                    case 9: code[carNum].CIFX_Firmware[0].channel_FirmwareCode = CarAgreement[ArrNum] = 0x02;
                        break;
                    default:
                        CarAgreement[ArrNum] = 0xFF;
                }

                ArrNum++;  //偏移
               
                //检查出来的都是在位的,所以要把 0状态更新成01空闲
                if (code[carNum].CIFX_State == 0)
                {
                    code[carNum].CIFX_State = 0x01;
                  
                }
                //赋值测试卡状态
                CarAgreement[ArrNum++] = code[carNum].CIFX_State;

                break;
            }
        }

       carNum++;
    }

   // return   &CarAgreement[0];

}
/*****************************************************************************/
/*! Function to demonstrate the board/channel enumeration
*   \return CIFX_NO_ERROR on success  
*   
/*****************************************************************************/

//程序启动后枚举出板子上的所有板卡信息
char *EnumBoardDemo(HANDLE hDriver)
{
  uint32_t          ulBoard    = 0;
  BOARD_INFORMATION tBoardInfo = {0};

  printf("---------- Board/Channel enumeration demo ----------\r\n");
  /* Iterate over all boards */
  //枚举出所有的板卡
  while(CIFX_NO_ERROR == xDriverEnumBoards(hDriver, ulBoard, sizeof(tBoardInfo), &tBoardInfo))
  {
    uint32_t            ulChannel    = 0;
    CHANNEL_INFORMATION tChannelInfo = {0};

    //获取板卡ID
    cifxbd[ulBoard].CIFX_BoardID =  tBoardInfo.ulBoardID;
    //拷贝获取到的板卡标识符
    strcpy(cifxbd[ulBoard].CIFX_BoardName,tBoardInfo.abBoardName);
    printf("Found Board %.10s\r\n", cifxbd[ulBoard].CIFX_BoardName);

    //printf("Found Board %.10s\r\n", tBoardInfo.abBoardName);
    if(strlen( (char*)tBoardInfo.abBoardAlias) != 0)
      printf(" Alias        : %.10s\r\n", tBoardInfo.abBoardAlias);

    /*printf(" DeviceNumber : %u\r\n", tBoardInfo.tSystemInfo.ulDeviceNumber);
    printf(" SerialNumber : %u\r\n", tBoardInfo.tSystemInfo.ulSerialNumber);
    printf(" Board ID     : %u\r\n", tBoardInfo.ulBoardID);
    printf(" System Error : 0x%08X\r\n", tBoardInfo.ulSystemError);
    printf(" Channels     : %u\r\n", tBoardInfo.ulChannelCnt);
    printf(" DPM Size     : %u\r\n", tBoardInfo.ulDpmTotalSize);*/

    /* iterate over all channels on the current board */
    //枚举出所有的通道和协议栈等信息
    while(CIFX_NO_ERROR == xDriverEnumChannels(hDriver, ulBoard, ulChannel, sizeof(tChannelInfo), &tChannelInfo))
    {

        //拷贝获取到的板卡协议栈
        strcpy(cifxbd[ulBoard].CIFX_Firmware[ulChannel].channel_Firmware, tChannelInfo.abFWName);
        cifxbd[ulBoard].CIFX_Firmware[ulChannel].channel_channelID = (char)ulChannel;

        printf(" - Channel %u:\r\n", cifxbd[ulBoard].CIFX_Firmware[ulChannel].channel_channelID);
        printf("    Firmware : % s\r\n", cifxbd[ulBoard].CIFX_Firmware[ulChannel].channel_Firmware);
       
        /* printf(" - Channel %u:\r\n", ulChannel);
       printf("    Firmware : %s\r\n", tChannelInfo.abFWName);
       printf("    Version  : %u.%u.%u build %u\r\n", 
             tChannelInfo.usFWMajor,
             tChannelInfo.usFWMinor,
             tChannelInfo.usFWBuild,
             tChannelInfo.usFWRevision);
       printf("    Date     : %02u/%02u/%04u\r\n", 
             tChannelInfo.bFWMonth,
             tChannelInfo.bFWDay,
             tChannelInfo.usFWYear);*/

      ++ulChannel;
    }

    ++ulBoard;
  }
  printf("----------------------------------------------------\r\n");
  
  //查询资源数据赋值
  QueryTesterResource(&cifxbd[0]);
  //打印资源查询结果
  DumpData(CarAgreement,sizeof(CarAgreement));

  return   &CarAgreement[0];
}


//写  指令给板卡X
int WriteDatToBoard(HANDLE hChannel, char* Senddat, int Senddatlen, uint32_t ulOffset)
{
    long   lRet = CIFX_NO_ERROR;

    if (CIFX_NO_ERROR != (lRet = xChannelIOWrite(hChannel, 0, ulOffset, Senddatlen, Senddat, IO_WAIT_TIMEOUT)))
    {
        printf("Error writing to IO Data area!\r\n");

        return lRet;

    }
    printf("WriteDat:    \r\n");
    DumpData(Senddat, Senddatlen);

    return 0;


}

//接收板卡X的数据
int ReadDatToBoard(HANDLE hChannel,char *Readat,int Readatlen, uint32_t ulOffset)
{
    long   lRet = CIFX_NO_ERROR;
    //接收数据测试
    if (CIFX_NO_ERROR != (lRet = xChannelIORead(hChannel, 0, ulOffset, Readatlen, Readat, IO_WAIT_TIMEOUT)))
    {
        printf("Error reading IO Data area!\r\n");
        
        return lRet;
    }
    printf("ReadDat:    \r\n");
    DumpData(Readat, Readatlen);

    return 0;
   
}



/*****************************************************************************/
/*! Function to demonstrate system device functionality (Packet Transfer)
*   \return CIFX_NO_ERROR on success                                         */
/*****************************************************************************/
long SysdeviceDemo(HANDLE hDriver, char* szBoard)
{
  long   lRet = CIFX_NO_ERROR;
  HANDLE hSys = NULL;

  printf("---------- System Device handling demo ----------\r\n");

  /* Driver/Toolkit successfully opened */
  //打开通道
  lRet = xSysdeviceOpen(hDriver, szBoard, &hSys);

  if(CIFX_NO_ERROR != lRet)
  {
    printf("Error opening SystemDevice!\r\n");

  } else
  {
    SYSTEM_CHANNEL_SYSTEM_INFO_BLOCK tSysInfo       = {0};
    uint32_t                         ulSendPktCount = 0;
    uint32_t                         ulRecvPktCount = 0;
    CIFX_PACKET                      tSendPkt       = {0};
    CIFX_PACKET                      tRecvPkt       = {0};
   
    /* System channel successfully opened, try to read the System Info Block */
    if( CIFX_NO_ERROR != (lRet = xSysdeviceInfo(hSys, CIFX_INFO_CMD_SYSTEM_INFO_BLOCK, sizeof(tSysInfo), &tSysInfo)))
    {
      printf("Error querying system information block\r\n");
    } else
    {
      printf("System Channel Info Block:\r\n");
      printf("DPM Size         : %u\r\n", tSysInfo.ulDpmTotalSize);
      printf("Device Number    : %u\r\n", tSysInfo.ulDeviceNumber);
      printf("Serial Number    : %u\r\n", tSysInfo.ulSerialNumber);
      printf("Manufacturer     : %u\r\n", tSysInfo.usManufacturer);
      printf("Production Date  : %u\r\n", tSysInfo.usProductionDate);
      printf("Device Class     : %u\r\n", tSysInfo.usDeviceClass);
      printf("HW Revision      : %u\r\n", tSysInfo.bHwRevision);
      printf("HW Compatibility : %u\r\n", tSysInfo.bHwCompatibility);
    }

    /* Do a simple Packet exchange via system channel */
    xSysdeviceGetMBXState(hSys, &ulRecvPktCount, &ulSendPktCount);
    printf("System Mailbox State: MaxSend = %u, Pending Receive = %u\r\n",
           ulSendPktCount, ulRecvPktCount);

    if(CIFX_NO_ERROR != (lRet = xSysdevicePutPacket(hSys, &tSendPkt, PACKET_WAIT_TIMEOUT)))
    {
      printf("Error sending packet to device!\r\n");
    } else
    {
      printf("Send Packet:\r\n");
      DumpPacket(&tSendPkt);

      xSysdeviceGetMBXState(hSys, &ulRecvPktCount, &ulSendPktCount);
      printf("System Mailbox State: MaxSend = %u, Pending Receive = %u\r\n",
            ulSendPktCount, ulRecvPktCount);

      if(CIFX_NO_ERROR != (lRet = xSysdeviceGetPacket(hSys, sizeof(tRecvPkt), &tRecvPkt, PACKET_WAIT_TIMEOUT)) )
      {
        printf("Error getting packet from device!\r\n");
      } else
      {
        printf("Received Packet:\r\n");
        DumpPacket(&tRecvPkt);

        xSysdeviceGetMBXState(hSys, &ulRecvPktCount, &ulSendPktCount);
        printf("System Mailbox State: MaxSend = %u, Pending Receive = %u\r\n",
              ulSendPktCount, ulRecvPktCount);
      }
    }
    xSysdeviceClose(hSys);
  }

  printf(" State = 0x%08X\r\n", lRet);
  printf("----------------------------------------------------\r\n");

  return lRet;
}

/*****************************************************************************/
/*! Function to demonstrate communication channel functionality
*   Packet Transfer and I/O Data exchange
*   \return CIFX_NO_ERROR on success                                         */
/*****************************************************************************/
long ChannelDemo(HANDLE hDriver, char* szBoard, uint32_t ulChannel)
{
  HANDLE hChannel = NULL;
  long   lRet     = CIFX_NO_ERROR;

  printf("---------- Communication Channel demo ----------\r\n");
  //先打开打开通道
  lRet = xChannelOpen(hDriver, szBoard, ulChannel, &hChannel);
  //打开失败
  if(CIFX_NO_ERROR != lRet)
  {
    printf("%s:Error opening Channel!,&szBoard");

  }
  else
  {
    CHANNEL_INFORMATION tChannelInfo  = {0};
    CIFX_PACKET         tSendPkt      = {0};
    CIFX_PACKET         tRecvPkt      = {0};
    /* Read and write I/O data (32Bytes). Output data will be incremented each cyle */
    unsigned char       abSendData[32] = {0};
    unsigned char       abRecvData[32] = {0};
    unsigned long       ulCycle        = 0;
    uint32_t            ulState        = 0;

    //发送buff赋值操作
    for (char i = 0; i < 32; i++)
    {
        abSendData[i] = i+1;
    }

    /* Channel successfully opened, so query basic information */
    //通道打开成功，读取基本信息
    if( CIFX_NO_ERROR != (lRet = xChannelInfo(hChannel, sizeof(CHANNEL_INFORMATION), &tChannelInfo)))
    {
      printf("%s:Error querying system information block\r\n",&szBoard);
    } 
    else
    {
      //printf("Communication Channel Info:%s\r\n",szBoard);
      printf("Device Number    : %u\r\n", tChannelInfo.ulDeviceNumber);
      printf("Serial Number    : %u\r\n", tChannelInfo.ulSerialNumber);
      printf("Firmware         : %s\r\n", tChannelInfo.abFWName);
      printf("FW Version       : %u.%u.%u build %u\r\n", 
              tChannelInfo.usFWMajor,
              tChannelInfo.usFWMinor,
              tChannelInfo.usFWBuild,
              tChannelInfo.usFWRevision);
      printf("FW Date          : %02u/%02u/%04u\r\n", 
              tChannelInfo.bFWMonth,
              tChannelInfo.bFWDay,
              tChannelInfo.usFWYear);

      printf("Mailbox Size     : %u\r\n", tChannelInfo.ulMailboxSize);
    }

    /* Set Host Ready to signal the filed bus an application isxChannelOpen ready */
    //设置主机准备好向现场总线发送信号应用程序已通道打开准备就绪
    lRet = xChannelHostState(hChannel, CIFX_HOST_STATE_READY, &ulState, HOSTSTATE_TIMEOUT);
    if(CIFX_NO_ERROR != lRet)
    {
      printf("%s:Error setting host ready!\r\n",&szBoard);
    } 
    else  //准备好了
    {
      /* Switch on the bus if it is not automatically running (see configuration options) */
        //如果通道总线未开启，手动启动总线
      lRet = xChannelBusState( hChannel, CIFX_BUS_STATE_ON, &ulState, 0L);

      if(CIFX_NO_ERROR != lRet)
      {
        printf("%s:Unable to start the filed bus!\r\n", &szBoard);
      }
      else
      {
        /* Do I/O Data exchange until a key is hit */
          //在按下某个键之前是否进行I/O数据交换
        while(!kbhit())
        {
            //接收数据测试
            if (CIFX_NO_ERROR != (lRet = xChannelIORead(hChannel, 0, 0, sizeof(abRecvData), abRecvData, IO_WAIT_TIMEOUT)))
            {
                printf("Error reading IO Data area!\r\n");
                break;
            }
            //发送数据测试
            /*if(CIFX_NO_ERROR != (lRet = xChannelIOWrite(hChannel, 0, 0, sizeof(abSendData), abSendData, IO_WAIT_TIMEOUT)))
            {
              printf("Error writing to IO Data area!\r\n");
              break;
            } */
            else
            {
             // printf("IOWrite Data:\r\n");
              //打印发送的数据到debug界面
              //DumpData(abSendData, sizeof(abSendData));
              //打印接收数据到debug
              printf("%s:\r\n",&szBoard);
              DumpData(abRecvData, sizeof(abRecvData));
              //清除接收内存
              memset(abRecvData, ulCycle + 1, sizeof(abRecvData));
            }
          
        }
      }
    }

    /* Switch off the bus  */
    //关闭总线
    lRet = xChannelBusState( hChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);

    /* Set Host not ready to stop bus communication */
    xChannelHostState(hChannel, CIFX_HOST_STATE_NOT_READY, &ulState, HOSTSTATE_TIMEOUT);

    /* Close the communication channel */
    //关闭通道
    xChannelClose(hChannel);
  }

  if(CIFX_NO_ERROR != lRet)
  {
    char szBuffer[256] = {0};
    xDriverGetErrorDescription(lRet, szBuffer, sizeof(szBuffer));

    printf(" State = 0x%08X <%s>\r\n", lRet, szBuffer);
  } else
  {
    printf(" State = 0x%08X\r\n", lRet);
  }
  printf("----------------------------------------------------\r\n");

  return lRet;
}




//测试仪资源申请请求
void ApplyforTesterResources(HANDLE hDriver, int arg, int arg2, rdata dat)
{
    char* p;
    if (arg == 0x01) //申请板卡
    {
        p = ApplyforTesterOpen(hDriver, arg2, 0, &cifxbd[0]);

    }
    else if (arg == 0x04) //释放板卡
    {
        p = ApplyforTesterClose(hDriver, arg2, 0, &cifxbd[0]);
    }
    else
    {
        //出错了
        printf("ApplyforTesterResources Err\r\n");
    }
}

/*****************************************************************************/
/*! The main function 
*   \return 0 on success                                                     */
/*****************************************************************************/

#if 0
int main(int argc, char* argv[])
{

  long lRet = CIFX_NO_ERROR; 
  DRIVER_INFORMATION tDriverInfo = { 0 };

  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  /* Open the cifX driver */
  //打开板卡
  lRet = xDriverOpen(&hDriver);
  //判断打开成功与否
  if(CIFX_NO_ERROR != lRet)
  {
    printf("Error opening driver. lRet=0x%08X\r\n", lRet);
  } 
   else
  {
    //收集所有板卡的消息 
    //EnumBoardDemo(hDriver);

    ApplyforTesterResources(hDriver,0x01,0x01,sendat);
    ApplyforTesterResources(hDriver, 0x01, 0x02, sendat);

    SendDatTest(cifxChannelSate[0x01].CxifChannel);

    ApplyforTesterResources(hDriver, 0x04, 0x01, sendat);
    ApplyforTesterResources(hDriver, 0x04, 0x02, sendat);
    //如何与板的系统设备通信的示例
    //SysdeviceDemo(hDriver, "cifX0");
   
    //如何与板上的通信信道通信的示例
    /*
    *   cifx0   EtherNet
    *   cifx1   EtherCat
    */
    //ChannelDemo(hDriver, "cifX0", 0);
    //ChannelDemo(hDriver, "cifX1", 0);
    /* Close the cifX driver */
    xDriverClose(hDriver);
  }

	return 0;
}
#endif // !0
