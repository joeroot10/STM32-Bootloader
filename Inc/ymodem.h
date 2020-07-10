/*****************************************************
**
**
**
**
**
**
**
***********************************************************/



#ifndef         __YMODEM__H_

#define         __YMODEM__H_

#ifdef __cplusplus
 extern "C" {
#endif


/*
YMode
YMode ��ʽ: ͷ[1Byte] ���[1Byte] ���ȡ��[1Byte] Data[?Byte] CRC_H CRC_L
ͷ Key Words
SOH     0x01        Э��ͷ(�̶�128bytes��������,���㲹0 ? 0x1A)
STX     0x02        Э��ͷ(�̶�1024bytes��������,���㲹0 ? 0x1A)
EOT     0x04        �������
ACK     0x06        ������Ӧ
NAK     0X15        ʧ����Ӧ
CAN     0x18        ȡ������
C       0x43        �����ļ�����

YMode��������
���ն�               ���Ͷ�
C(0x43)
                  SOH 00 FF foo.c NUL CRC_H CRC_L   // ���Ͷ˷����ļ����Լ��ļ���С
ACK(0x06)
C(0x43)
                  SOH 01 FE Data[128] CRC_H CRC_L // STX 01 FE Data[1024] CRC_H CRC_L
ACK(0x06)
                  SOH 01 FE Data[128] CRC_H CRC_L // STX 01 FE Data[1024] CRC_H CRC_L
ACK(0x06)
......
                  SOH 01 FE Data[128] CRC_H CRC_L   // ���һ��
ACK(0x06)
                  EOT(0x04)
NAK(0x15)                  
                  EOT(0x04)
ACK(0x06) 
C(0x43)
                  SOH 00 FF NUL CRC_H CRC_L // ȫ0���ݣ���ʾ ���� 
ACK(0x06)                  
*/

#define MODEM_EOT 0x04  // ���ݽ���
#define MODEM_SOH 0x01  //���ݿ���ʼ�ַ�
#define MODEM_STX 0x02  //1028�ֽڿ�ʼ
#define MODEM_EOT 0x04  //�ļ��������
#define MODEM_ACK 0x06  //ȷ��Ӧ��
#define MODEM_NAK 0x15  //���ִ���
#define MODEM_CAN 0x18  //ȡ������
#define MODEM_C 0x43    //��д��ĸ��


#define         _FILE_NAME_SIZE         (96)

#define         _IS_DIGIT(c)            ((c) <= 0x39 && (c) >= 0x30 )


// ���ص���ϢID
//transmitting terminal
#define     _UNKNOWN_MSG             0x01        // δָ֪��
#define     _FILE_INFO_MSG           0x02        // ���ļ���ʼ,��Ϣ���ļ�������С
#define     _FILE_DATA_MSG           0x03
#define     _FILE_SUCCESS_MSG        0x04       // �ɹ�
#define     _TRCAN_MSG               0x05       // �Է�ȡ������
#define     _TIMEOUT_MSG             0x06       // ��ʱ

// ����msg

typedef struct
{
    uint8_t     msgID ;
    union
    {
        // ���ļ���ʼ
        struct 
        {
            char        *mpfileName ;
            uint32_t    filesize ;
        }NewFile ;
        
        // �ļ����� 
        struct 
        {
            uint8_t     mpBuff[1024+128] ;   // ��������
            uint16_t    miLen ;     // ���ݳ���
            uint32_t    mifilesize ; // �ļ��ܳ���
            uint32_t    miRevfilelen ;  // ��ǰ�Ѿ��յ����ݳ���,δ���� �������� miLen
        }FileData ;        
    }MsgData;
}YmodemMSG , *pYmodemMSG ;



static void YModemSendFileInfoMsg( uint8_t *, uint16_t );
static void YModemSendFileDataMsg( uint8_t *, uint16_t );
static void YModemSendFileSuccessMsg(void);
static void YModemSendTimeoutMsg(void);

// 0 :�ɹ� , 1:ʧ��
typedef uint8_t (*_ymodem_cb)( pYmodemMSG );

typedef void (*ymodem_send)(uint8_t*,uint16_t);



typedef struct
{
    char        mpfileName[_FILE_NAME_SIZE];    // �ļ���
    uint32_t    mifilesize ;                // ���ļ���С[�����ն˿�������,SecureCRT����16������ʾ��]
    uint32_t    miRevfileLen ;              // ��ǰ�Ѿ��յ����ļ���С
    uint8_t     miCnt ;                     // ����
    uint8_t     miEOT ;                     // ����
    _ymodem_cb  mpYmodemCB ;                // ͬ������֮
    ymodem_send mpYmodemSendFun ;           // ymodem ���ͽӿ�
}Ymodeminfo , *pYmodeinfo ;






static uint16_t _CRC16(uint8_t *, uint16_t );


uint8_t ymodem_init( _ymodem_cb , ymodem_send );

uint8_t ymodem_start(void);

uint8_t ymodem_rev_data(uint8_t*,uint16_t );


static void ymodem_dcode_timer(void);


// ymode ���ݻ���
#define         _YMODEM_BUFF_SIZE       (512*3)

typedef struct
{
    uint8_t     mpBuff[_YMODEM_BUFF_SIZE];
    uint16_t    miTail ;
    uint16_t    miHead ;
}YmodemRx , *pYmodemRx ;

static void ymodemRx_Init(void);
static void ymodemRx_Push(uint8_t*,uint16_t);
static uint16_t ymodeRx_Dcode(uint8_t*,uint16_t);





// send
uint8_t YModem_C(void);
uint8_t YModem_ACK(void);
uint8_t YModem_NAK(void);
uint8_t YModem_CAN(void);


void YModem_SOH(uint8_t*,uint16_t);
void YModem_STX(uint8_t*,uint16_t); 
void YModem_EOT(uint8_t*,uint16_t);


// �ж��ǲ���ͷ
#define         _YMODE_HEAD(d)          ( (d) == MODEM_EOT || (d) == MODEM_STX || (d) == MODEM_SOH )

#define         _EOT_PACK_LEN                (1) // 1
#define         _SOH_PACK_LEN                (5+128)
#define         _STX_PACK_LEN                (5+1024)



// timer
static void ymodem_start_timeout(void);               // ������ʱ
static void ymodem_revdata_timeout(void);   // �ȴ����ݳ�ʱ


#ifdef __cplusplus
}
#endif


#endif // __YMODEM__H_








