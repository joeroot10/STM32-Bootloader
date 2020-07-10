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
YMode 格式: 头[1Byte] 序号[1Byte] 序号取反[1Byte] Data[?Byte] CRC_H CRC_L
头 Key Words
SOH     0x01        协议头(固定128bytes数据内容,不足补0 ? 0x1A)
STX     0x02        协议头(固定1024bytes数据内容,不足补0 ? 0x1A)
EOT     0x04        传输结束
ACK     0x06        接收响应
NAK     0X15        失败响应
CAN     0x18        取消传输
C       0x43        开启文件传输

YMode传输流程
接收端               发送端
C(0x43)
                  SOH 00 FF foo.c NUL CRC_H CRC_L   // 发送端返回文件名以及文件大小
ACK(0x06)
C(0x43)
                  SOH 01 FE Data[128] CRC_H CRC_L // STX 01 FE Data[1024] CRC_H CRC_L
ACK(0x06)
                  SOH 01 FE Data[128] CRC_H CRC_L // STX 01 FE Data[1024] CRC_H CRC_L
ACK(0x06)
......
                  SOH 01 FE Data[128] CRC_H CRC_L   // 最后一包
ACK(0x06)
                  EOT(0x04)
NAK(0x15)                  
                  EOT(0x04)
ACK(0x06) 
C(0x43)
                  SOH 00 FF NUL CRC_H CRC_L // 全0数据，表示 结束 
ACK(0x06)                  
*/

#define MODEM_EOT 0x04  // 数据结束
#define MODEM_SOH 0x01  //数据块起始字符
#define MODEM_STX 0x02  //1028字节开始
#define MODEM_EOT 0x04  //文件传输结束
#define MODEM_ACK 0x06  //确认应答
#define MODEM_NAK 0x15  //出现错误
#define MODEM_CAN 0x18  //取消传输
#define MODEM_C 0x43    //大写字母Ｃ


#define         _FILE_NAME_SIZE         (96)

#define         _IS_DIGIT(c)            ((c) <= 0x39 && (c) >= 0x30 )


// 返回的消息ID
//transmitting terminal
#define     _UNKNOWN_MSG             0x01        // 未知指令
#define     _FILE_INFO_MSG           0x02        // 新文件开始,消息带文件名及大小
#define     _FILE_DATA_MSG           0x03
#define     _FILE_SUCCESS_MSG        0x04       // 成功
#define     _TRCAN_MSG               0x05       // 对方取消传输
#define     _TIMEOUT_MSG             0x06       // 超时

// 返回msg

typedef struct
{
    uint8_t     msgID ;
    union
    {
        // 新文件开始
        struct 
        {
            char        *mpfileName ;
            uint32_t    filesize ;
        }NewFile ;
        
        // 文件数据 
        struct 
        {
            uint8_t     mpBuff[1024+128] ;   // 数据内容
            uint16_t    miLen ;     // 数据长度
            uint32_t    mifilesize ; // 文件总长度
            uint32_t    miRevfilelen ;  // 当前已经收到数据长度,未包括 本次数据 miLen
        }FileData ;        
    }MsgData;
}YmodemMSG , *pYmodemMSG ;



static void YModemSendFileInfoMsg( uint8_t *, uint16_t );
static void YModemSendFileDataMsg( uint8_t *, uint16_t );
static void YModemSendFileSuccessMsg(void);
static void YModemSendTimeoutMsg(void);

// 0 :成功 , 1:失败
typedef uint8_t (*_ymodem_cb)( pYmodemMSG );

typedef void (*ymodem_send)(uint8_t*,uint16_t);



typedef struct
{
    char        mpfileName[_FILE_NAME_SIZE];    // 文件名
    uint32_t    mifilesize ;                // 总文件大小[超级终端可以正常,SecureCRT是以16进制显示的]
    uint32_t    miRevfileLen ;              // 当前已经收到的文件大小
    uint8_t     miCnt ;                     // 计数
    uint8_t     miEOT ;                     // 计数
    _ymodem_cb  mpYmodemCB ;                // 同步处理之
    ymodem_send mpYmodemSendFun ;           // ymodem 发送接口
}Ymodeminfo , *pYmodeinfo ;






static uint16_t _CRC16(uint8_t *, uint16_t );


uint8_t ymodem_init( _ymodem_cb , ymodem_send );

uint8_t ymodem_start(void);

uint8_t ymodem_rev_data(uint8_t*,uint16_t );


static void ymodem_dcode_timer(void);


// ymode 数据缓冲
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


// 判读是不是头
#define         _YMODE_HEAD(d)          ( (d) == MODEM_EOT || (d) == MODEM_STX || (d) == MODEM_SOH )

#define         _EOT_PACK_LEN                (1) // 1
#define         _SOH_PACK_LEN                (5+128)
#define         _STX_PACK_LEN                (5+1024)



// timer
static void ymodem_start_timeout(void);               // 启动超时
static void ymodem_revdata_timeout(void);   // 等待数据超时


#ifdef __cplusplus
}
#endif


#endif // __YMODEM__H_








