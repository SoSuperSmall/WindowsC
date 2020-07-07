#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#include "cifxuser.h"
#include "cifxErrors.h"
#include "cifxdemo.h"


#pragma comment(lib,"ws2_32.lib")		//����ws2_32.dll
#pragma comment(lib,"Winmm.lib")	//For timeSetEvent


#define PACKET_WAIT_TIMEOUT 20
#define IO_WAIT_TIMEOUT     10
#define HOSTSTATE_TIMEOUT   5000

//���а忨�ľ��
HANDLE hDriver = NULL;

//��Դ������Ӧ���
char ApplyforTesterArropen[2] = { 0 };
//��Դ�����ͷŽ��
char ApplyforTesterArrclose[2] = { 0 };

//��������Դ��ѯ��Ӧ
char CarAgreement[12] = { 0 };

cifxChannel cifxChannelSate[6] = { NULL };


//�������ݸ�ʽ
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

//�������ݸ�ʽ
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
  
  //��ӡ���ݳ���
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



//�忨Э���ַ����Ա�
typedef struct cifxmore
{
    char*   str;
   // void    (*func)(void);
}CifxAgreement;


CifxAgreement CifxAgreementArr[] = {

    { "EtherNet/IP Scanner"}, //��վ
    { "EtherNet/IP Adapter"},//��վ

    { "EtherCAT Master"},
    { "EtherCAT Slave"},

    { "PROFIBUS-DP Master"},
    { "PROFIBUS-DP Slave"},

    { "CANopen Master"},
    { "CANopen Slave"},

    { "Open Modbus/TCP"}
};



//��������ϸ��Դ��ѯ����
CarMoreDat *QueryTesterMoreResource(int slot)
{
    CarMoreDat carmoreinfo;

    switch (slot)
    {
        carmoreinfo.Request = 0;  //����ɹ�
        carmoreinfo.Slot = slot;  //�ۺ�
        carmoreinfo.CarType = 0;  //������
        //Э��ID
        carmoreinfo.CarState = cifxbd[slot].CIFX_Firmware[0].channel_FirmwareCode;  //Э��ID
        //carmoreinfo.Port = 502;
        //carmoreinfo.Addr = inet_addr("192.168.1.10");

    }

    return &carmoreinfo;

}


//�򿪰忨
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

    //��ʧ��
    if (CIFX_NO_ERROR != lRet)
    {
        printf("%s:Error opening Channel!,&szBoard");
        //����ʧ��
        ApplyforTesterArropen[0] = 0x81;

    }
    else
    {
        //ͨ���򿪳ɹ�����ȡ������Ϣ
        if (CIFX_NO_ERROR != (lRet = xChannelInfo(cifxChannelSate[szBoard].CxifChannel, sizeof(CHANNEL_INFORMATION), &tChannelInfo)))
        {
            printf("%s:Error querying system information block\r\n", &szBoard);
            //����ʧ��
            ApplyforTesterArropen[0] = 0x82;
                
        }
        else
        {
            printf("szBoard:    %d\r\n", szBoard);
            printf("Device Number    : %u\r\n", tChannelInfo.ulDeviceNumber);
            printf("Serial Number    : %u\r\n", tChannelInfo.ulSerialNumber);
            printf("Firmware         : %s\r\n", tChannelInfo.abFWName);


            //��������׼�������ֳ����߷����ź�Ӧ�ó�����ͨ����׼������
            lRet = xChannelHostState(cifxChannelSate[szBoard].CxifChannel, CIFX_HOST_STATE_READY, &ulState, HOSTSTATE_TIMEOUT);
            if (CIFX_NO_ERROR != lRet)
            {
                printf("%s:Error setting host ready!\r\n", &szBoard);
                //����ʧ��
                ApplyforTesterArropen[0] = 0x83;
               
            }
            else
            {
                //���ͨ������δ�������ֶ���������
                lRet = xChannelBusState(cifxChannelSate[szBoard].CxifChannel, CIFX_BUS_STATE_ON, &ulState, 0L);
                if (CIFX_NO_ERROR != lRet)
                {
                    printf("%s:Unable to start the filed bus!\r\n", &szBoard);
                    //����ʧ��
                    ApplyforTesterArropen[0] = 0x84;                               
                   
                }
                else //������Դ�ɹ�
                {
                    ApplyforTesterArropen[0] = 0x02; //�ɹ�
                    code[szBoard].CIFX_State = 0x02;  //�޸İ忨ռ��״̬:����ռ��
                   
                }


            }

        }


    }

    ApplyforTesterArropen[1] = szBoard; //�忨��

    return ApplyforTesterArropen;

}



//�رհ忨
char *ApplyforTesterClose(HANDLE hDriver, int szBoard, uint32_t ulChannel,struct cifx* code)
{
;
    long   lRet = CIFX_NO_ERROR;
    uint32_t            ulState = 0;

     //�ر�����
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
        //��������δ׼����
        lRet = xChannelHostState(cifxChannelSate[szBoard].CxifChannel, CIFX_HOST_STATE_NOT_READY, &ulState, HOSTSTATE_TIMEOUT);
        if (CIFX_NO_ERROR != lRet)
        {
            printf("%s:Unable to Close the filed bus!\r\n", &szBoard);
            ApplyforTesterArrclose[0] = 0XF2;
        }
        else
        {
            //�ر�ͨ��
            lRet = xChannelClose(cifxChannelSate[szBoard].CxifChannel);
            if (CIFX_NO_ERROR != lRet)
            {
                printf("%s:Unable to Close the Channel !\r\n", &szBoard);
                ApplyforTesterArrclose[0] = 0XF3;
            }
            else
            {
                ApplyforTesterArrclose[0] = 0X05;
                code[szBoard].CIFX_State = 0x01;  //�޸İ忨ռ��״̬:���п���
            }
        }

    }


    ApplyforTesterArrclose[1] = szBoard; //�忨��

    return ApplyforTesterArrclose;

}

////���Կ���Ϣ��ѯ����
void  QueryTesterResource(struct cifx *code)
{
    int carNum = 0,ArrNum = 0;
    uint8_t str_size = sizeof(CifxAgreementArr) / sizeof(CifxAgreementArr[0]);

    //�жϰ忨������
    while(strstr(code[carNum].CIFX_BoardName,"cifX") != 0 )
    {
        printf("    Firmware : %s : %s\r\n", code[carNum].CIFX_BoardName, cifxbd[carNum].CIFX_Firmware[0].channel_Firmware);
        printf("    carNum  :   %d\r\n", carNum);
        for (uint8_t i = 0; i < str_size; i++)
        {
           //printf("    CifxAgreementArr : % s\r\n", CifxAgreementArr[i].str);          
            if(strstr(code[carNum].CIFX_Firmware[0].channel_Firmware,CifxAgreementArr[i].str))
            {   
                //��ֵ���Կ�Э��
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

                ArrNum++;  //ƫ��
               
                //�������Ķ�����λ��,����Ҫ�� 0״̬���³�01����
                if (code[carNum].CIFX_State == 0)
                {
                    code[carNum].CIFX_State = 0x01;
                  
                }
                //��ֵ���Կ�״̬
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

//����������ö�ٳ������ϵ����а忨��Ϣ
char *EnumBoardDemo(HANDLE hDriver)
{
  uint32_t          ulBoard    = 0;
  BOARD_INFORMATION tBoardInfo = {0};

  printf("---------- Board/Channel enumeration demo ----------\r\n");
  /* Iterate over all boards */
  //ö�ٳ����еİ忨
  while(CIFX_NO_ERROR == xDriverEnumBoards(hDriver, ulBoard, sizeof(tBoardInfo), &tBoardInfo))
  {
    uint32_t            ulChannel    = 0;
    CHANNEL_INFORMATION tChannelInfo = {0};

    //��ȡ�忨ID
    cifxbd[ulBoard].CIFX_BoardID =  tBoardInfo.ulBoardID;
    //������ȡ���İ忨��ʶ��
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
    //ö�ٳ����е�ͨ����Э��ջ����Ϣ
    while(CIFX_NO_ERROR == xDriverEnumChannels(hDriver, ulBoard, ulChannel, sizeof(tChannelInfo), &tChannelInfo))
    {

        //������ȡ���İ忨Э��ջ
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
  
  //��ѯ��Դ���ݸ�ֵ
  QueryTesterResource(&cifxbd[0]);
  //��ӡ��Դ��ѯ���
  DumpData(CarAgreement,sizeof(CarAgreement));

  return   &CarAgreement[0];
}


//д  ָ����忨X
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

//���հ忨X������
int ReadDatToBoard(HANDLE hChannel,char *Readat,int Readatlen, uint32_t ulOffset)
{
    long   lRet = CIFX_NO_ERROR;
    //�������ݲ���
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
  //��ͨ��
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
  //�ȴ򿪴�ͨ��
  lRet = xChannelOpen(hDriver, szBoard, ulChannel, &hChannel);
  //��ʧ��
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

    //����buff��ֵ����
    for (char i = 0; i < 32; i++)
    {
        abSendData[i] = i+1;
    }

    /* Channel successfully opened, so query basic information */
    //ͨ���򿪳ɹ�����ȡ������Ϣ
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
    //��������׼�������ֳ����߷����ź�Ӧ�ó�����ͨ����׼������
    lRet = xChannelHostState(hChannel, CIFX_HOST_STATE_READY, &ulState, HOSTSTATE_TIMEOUT);
    if(CIFX_NO_ERROR != lRet)
    {
      printf("%s:Error setting host ready!\r\n",&szBoard);
    } 
    else  //׼������
    {
      /* Switch on the bus if it is not automatically running (see configuration options) */
        //���ͨ������δ�������ֶ���������
      lRet = xChannelBusState( hChannel, CIFX_BUS_STATE_ON, &ulState, 0L);

      if(CIFX_NO_ERROR != lRet)
      {
        printf("%s:Unable to start the filed bus!\r\n", &szBoard);
      }
      else
      {
        /* Do I/O Data exchange until a key is hit */
          //�ڰ���ĳ����֮ǰ�Ƿ����I/O���ݽ���
        while(!kbhit())
        {
            //�������ݲ���
            if (CIFX_NO_ERROR != (lRet = xChannelIORead(hChannel, 0, 0, sizeof(abRecvData), abRecvData, IO_WAIT_TIMEOUT)))
            {
                printf("Error reading IO Data area!\r\n");
                break;
            }
            //�������ݲ���
            /*if(CIFX_NO_ERROR != (lRet = xChannelIOWrite(hChannel, 0, 0, sizeof(abSendData), abSendData, IO_WAIT_TIMEOUT)))
            {
              printf("Error writing to IO Data area!\r\n");
              break;
            } */
            else
            {
             // printf("IOWrite Data:\r\n");
              //��ӡ���͵����ݵ�debug����
              //DumpData(abSendData, sizeof(abSendData));
              //��ӡ�������ݵ�debug
              printf("%s:\r\n",&szBoard);
              DumpData(abRecvData, sizeof(abRecvData));
              //��������ڴ�
              memset(abRecvData, ulCycle + 1, sizeof(abRecvData));
            }
          
        }
      }
    }

    /* Switch off the bus  */
    //�ر�����
    lRet = xChannelBusState( hChannel, CIFX_BUS_STATE_OFF, &ulState, 0L);

    /* Set Host not ready to stop bus communication */
    xChannelHostState(hChannel, CIFX_HOST_STATE_NOT_READY, &ulState, HOSTSTATE_TIMEOUT);

    /* Close the communication channel */
    //�ر�ͨ��
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




//��������Դ��������
void ApplyforTesterResources(HANDLE hDriver, int arg, int arg2, rdata dat)
{
    char* p;
    if (arg == 0x01) //����忨
    {
        p = ApplyforTesterOpen(hDriver, arg2, 0, &cifxbd[0]);

    }
    else if (arg == 0x04) //�ͷŰ忨
    {
        p = ApplyforTesterClose(hDriver, arg2, 0, &cifxbd[0]);
    }
    else
    {
        //������
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
  //�򿪰忨
  lRet = xDriverOpen(&hDriver);
  //�жϴ򿪳ɹ����
  if(CIFX_NO_ERROR != lRet)
  {
    printf("Error opening driver. lRet=0x%08X\r\n", lRet);
  } 
   else
  {
    //�ռ����а忨����Ϣ 
    //EnumBoardDemo(hDriver);

    ApplyforTesterResources(hDriver,0x01,0x01,sendat);
    ApplyforTesterResources(hDriver, 0x01, 0x02, sendat);

    SendDatTest(cifxChannelSate[0x01].CxifChannel);

    ApplyforTesterResources(hDriver, 0x04, 0x01, sendat);
    ApplyforTesterResources(hDriver, 0x04, 0x02, sendat);
    //�������ϵͳ�豸ͨ�ŵ�ʾ��
    //SysdeviceDemo(hDriver, "cifX0");
   
    //�������ϵ�ͨ���ŵ�ͨ�ŵ�ʾ��
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
