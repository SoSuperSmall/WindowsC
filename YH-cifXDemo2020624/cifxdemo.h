#ifndef		__CIFXDEMO_H_
#define		__CIFXDEMO_H_

#include <stdio.h>
#include <stdint.h> 

#define CIFXNum 6

extern HANDLE hDriver;
extern char CarAgreement[12];

//ͨ����
typedef struct channel
{
    char channel_Firmware[30]; //�忨��Э��
    char channel_FirmwareCode; //Э�����(�ο�Э���ĵ�)
    char channel_channelID;    //ͨ��ID

}CIFXchannel;


//�忨��
typedef struct cifx
{
    SOCKET   ClientID; //ռ�ÿͻ���FD
    char   CIFX_BoardID; //�忨��ID
    char   CIFX_State;//�忨��״̬,0x00 Ϊ����λ/0x0001Ϊ����/0x0002Ϊռ��/0x0004����
    //char   TestSuiteID; //�����׼�ID
   // char   TestCases; //������
    char   TestSta; //����״̬ 
    char   CIFX_BoardName[10]; //�忨�ı�ʶ�� "cifx0"
    CIFXchannel CIFX_Firmware[CIFXNum];

}CIFXBoard[CIFXNum];



extern CIFXBoard cifxbd;


//���Կ���ϸ��Ϣ��ѯ��Ӧ
typedef struct cifx1
{
    char    Request;   //����ɹ�
    char    Slot;     //����
    char    CarType;  //��������
    char    CarState; //����״̬
    short   CarFirmware; //�忨��Э��ID(Ҳ�ǹ̼�)
    //unsigned long   Addr;  //��ַ��Ϣ
   // short   Port;   //�˿���Ϣ

}CarMoreDat;



//ͨ������ṹ��
typedef struct ChannelSate
{
    HANDLE CxifChannel;

}cifxChannel;



extern  struct ChannelSate cifxChannelSate[6];


//��������ϸ��Դ��ѯ����
CarMoreDat* QueryTesterMoreResource(int slot);
//���Կ���Ϣ��ѯ����
void QueryTesterResource(struct cifx* code);
//ö�ٰ忨����Ϣ
char* EnumBoardDemo(HANDLE hDriver);
//�򿪰忨
char* ApplyforTesterOpen(HANDLE hDriver, int szBoard, uint32_t ulChannel, struct cifx* code);
//�رհ忨
char* ApplyforTesterClose(HANDLE hDriver, int szBoard, uint32_t ulChannel, struct cifx* code);
//���հ忨X������
int ReadDatToBoard(HANDLE hChannel, char* dat, int len, uint32_t ulOffset);
//����ָ����忨X
int WriteDatToBoard(HANDLE hChannel, char* Senddat, int len, uint32_t ulOffset);


#endif // !1



