/*******************************************************
**
**
**
**
**
**
**
**
**
**********************************************************/

#include "stdint.h"

#include "ymodem.h"

#include "usrTimer.h"

static YmodemRx gymoderx = {0x00};

static Ymodeminfo gymodeminfo = {0x00};

static uint8_t    gymodedcodebuff[1024+32] = {0x00};

static uint8_t  gWaitYmodeTimerID = 0xFF ;

static uint8_t  gYmodemDcodeTimerID = 0xFF ;

static YmodemMSG gYmodemMSG = { 0x0 };


#define     _WAIT_YMODEM_TIMEOUT(cb,ms)     do{\
                                   DEL_TIMER( gWaitYmodeTimerID );\
                                   gWaitYmodeTimerID = CreateTimer( cb );\
                                   StartTimer( gWaitYmodeTimerID, ms ) ;\
                                    }while(0)

#define     _STOP_YMODEM_TIMEOUT    DEL_TIMER( gWaitYmodeTimerID )   


#define     _YMODEM_TIMEOUT         do{\
                                    _STOP_YMODEM_TIMEOUT ;\
                                    YModem_CAN();\
                                    DEL_TIMER( gYmodemDcodeTimerID );\
                                    YModemSendTimeoutMsg();\
                                    memset( (void*)&gymodeminfo , 0 , sizeof(gymodeminfo) ); \
                                    }while(0)

        
#define     _CAN_YMODEM             do{\
                                    YModem_CAN();\
                                    DEL_TIMER( gYmodemDcodeTimerID ) ;\
                                    DEL_TIMER( gWaitYmodeTimerID ) ;\
                                    memset( (void*)&gymodeminfo , 0 , sizeof(gymodeminfo) ); \
                                    }while(0)

#define     _DONE_YMODEM            do{\
                                    DEL_TIMER( gYmodemDcodeTimerID ) ;\
                                    DEL_TIMER( gWaitYmodeTimerID ) ;\
                                    memset( (void*)&gymodeminfo , 0 , sizeof(gymodeminfo) ); \
                                    }while(0)


// 外部调用
uint8_t ymodem_init( _ymodem_cb nof_cb, ymodem_send send_fun )
{
    if( !nof_cb || !send_fun ) return 1;

    DEL_TIMER( gYmodemDcodeTimerID );
    DEL_TIMER( gWaitYmodeTimerID );
    ymodemRx_Init();
    gymodeminfo.miCnt = 0x00 ;
    gymodeminfo.mifilesize = 0 ;
    gymodeminfo.miRevfileLen = 0 ;
    gymodeminfo.mpYmodemSendFun = send_fun ;
    gymodeminfo.mpYmodemCB = nof_cb ;
    
    return 0 ;
}

// 外部调用
uint8_t ymodem_start(void)
{
    if( SwitchTimer( gWaitYmodeTimerID ) != 1 )
    {
        if( ! YModem_C() )
        {
            gymodeminfo.miCnt = 0x00 ;
            gymodeminfo.mifilesize = 0 ;
            gymodeminfo.miRevfileLen = 0 ;
            gymodeminfo.miEOT = 0x00 ;
            _WAIT_YMODEM_TIMEOUT( ymodem_start_timeout , 300 );
            gYmodemDcodeTimerID = CreateTimer( ymodem_dcode_timer );
            StartTimer( gYmodemDcodeTimerID, 1 );
            return 0 ;
        }
    }
    return 1 ;
}

// 发送C失败
static void ymodem_start_timeout(void)               // 启动超时
{
    if( gymodeminfo.miCnt++ < 100 )
    {
       if( ! YModem_C() )
       {
            _WAIT_YMODEM_TIMEOUT( ymodem_start_timeout , 300 );
       }
       else 
       {
            _YMODEM_TIMEOUT ;
       }
    }
    else
    {
        _YMODEM_TIMEOUT ;
    }
}


static void ymodem_revdata_timeout(void)   // 等待数据超时
{
    _YMODEM_TIMEOUT ;
}

// 外部调用
uint8_t ymodem_rev_data(uint8_t* pinbuff ,uint16_t len )
{
    if( !pinbuff || !len ) return 1 ;
    ymodemRx_Push( pinbuff , len );
    return 0 ;
}



static void ymodem_dcode_timer(void)
{
    uint16_t len = 0 ;     
    while( len = ymodeRx_Dcode( gymodedcodebuff , sizeof(gymodedcodebuff) ) )
    {
        _STOP_YMODEM_TIMEOUT ;
        gymodeminfo.miCnt = 0x00 ;  // 清空计数
        switch( gymodedcodebuff[0] )
        {
            case MODEM_EOT :
                YModem_EOT( 0 , 0 ) ;
                break ;
            case MODEM_SOH :
                YModem_SOH(gymodedcodebuff , len );
                break ;
            case MODEM_STX :
                YModem_STX(gymodedcodebuff , len );
                break ;
            default :   // 不可能到此
                break ;
        }
    }
    //
    StartTimer( gYmodemDcodeTimerID, 5 );
}


static void YModem_Transmission( uint8_t * pinbuff , uint16_t inlen )
{
    if( !pinbuff || !inlen ) return ;
    if( gymodeminfo.mpYmodemCB ) 
    {
        if( pinbuff[1] == 0 )       // 有可能是开始/结束 , 也有可能是数据
        {
            if( pinbuff[3] )        // 有可能是数据 & 开始
            {
                // secureCRT ymode 并没有文件大小信息
                //
                if( /*gymodeminfo.mifilesize == 0*/
                    gymodeminfo.mpfileName[0] == 0x00 )   // 为数据开始,文件名及大小
                {
                    YModemSendFileInfoMsg( pinbuff + 3 , inlen - 5 );
                }
                else                                // 是数据
                {
                   YModemSendFileDataMsg( pinbuff + 3 , inlen -5  );  
                }
            }
            else                    // 有可能是数据 & 结束
            {
                YModemSendFileSuccessMsg();
            }
        }
        else    // 数据
        {
            YModemSendFileDataMsg( pinbuff + 3 , inlen - 5 );         
        }       
    }
    else
    {
        _CAN_YMODEM ;
    }
}


/*
  对于超级终端 YMODE,文件信息是以SOH传输的
  SOH 00 FF [filename 00] [filesize 00] [NUL..] CRC CRC
*/
void YModem_SOH(uint8_t* pinbuff ,uint16_t inlen )
{
    YModem_Transmission( pinbuff , inlen );
}
/*
    对于secureCRT的ymode
    文件信息是以STX传输的 , 也有说文件信息中不包含总字节数
    STX 00 FF [filename 00] [filesize 00] [NUL..] CRC CRC >>>
    SecureCRT 文件长度之后是以空格填充的
*/
void YModem_STX(uint8_t* pinbuff ,uint16_t inlen )
{
    YModem_Transmission( pinbuff , inlen );
}

void YModem_EOT(uint8_t* pinbuff ,uint16_t inlen )
{
    // 第一次返回 nak
    // 第二次返回 ack
    if( gymodeminfo.miEOT == 0 )
    {
        YModem_NAK();
        _WAIT_YMODEM_TIMEOUT( ymodem_revdata_timeout , 1000 );
    }
    else
    {
        YModem_ACK();
        YModem_C();
        _WAIT_YMODEM_TIMEOUT( ymodem_revdata_timeout , 1000 );
    }
    gymodeminfo.miEOT++ ;
}

// 发送新数据信息
static void YModemSendFileInfoMsg( uint8_t *pInbuff , uint16_t inlen )
{
    //uint8_t result = 0 ;
    uint16_t j = 0 ;
    if( !pInbuff || !inlen ) return ;

    //memset( &gYmodemData , 0 , sizeof(gYmodemData) );
    //memset( &gYmodeInfo , 0 , sizeof(gYmodeInfo) );

    memset( &gYmodemMSG , 0 , sizeof(gYmodemMSG) );
    
    // get file name 
    for( j = 0 ; j < _FILE_NAME_SIZE && j < inlen && pInbuff[j] != 0x00 ; j++ )
    {
        gymodeminfo.mpfileName[j] = pInbuff[j] ;
    }
    gymodeminfo.mpfileName[j] = 0x00 ; // 
    while( j < inlen )
    { 
        if( pInbuff[j] == 0x00 ) break ;
        j++ ;
    }
    
    for( gymodeminfo.mifilesize = 0 ,j++ ; j < inlen && 
        _IS_DIGIT( pInbuff[j] ) ; j++ )
    {
        gymodeminfo.mifilesize = gymodeminfo.mifilesize * 10 + ( pInbuff[j] - '0' );
    }
    
    gymodeminfo.miRevfileLen = 0x00 ;

    gymodeminfo.miEOT = 0x00 ;
    
    gYmodemMSG.msgID = _FILE_INFO_MSG;
    gYmodemMSG.MsgData.NewFile.mpfileName = gymodeminfo.mpfileName ;
    gYmodemMSG.MsgData.NewFile.filesize = gymodeminfo.mifilesize ;

    //gYmodeInfo.msgID = _FILE_INFO_MSG ;
    //gYmodeInfo.mfileName = gymodeminfo.mpfileName ;
    //gYmodeInfo.filesize = gymodeminfo.mifilesize  ;
    //
    if( gymodeminfo.mpYmodemCB )
    {
        if( 0 == gymodeminfo.mpYmodemCB( &gYmodemMSG )  )
        {
            // ack & c
            YModem_ACK();
            YModem_C();
            _WAIT_YMODEM_TIMEOUT( ymodem_start_timeout , 300 );
        }
        else
        {
            _CAN_YMODEM; // 取消
        }
    }
    else
    {
        _CAN_YMODEM ;
    }
}


// 发送文件数据
static void YModemSendFileDataMsg( uint8_t *pinbuff, uint16_t len )
{
    if( !pinbuff || !len ) return ;

    //gYmodemData.msgID = _FILE_DATA_MSG;
    //gYmodemData.miRevfilelen = gymodeminfo.miRevfileLen ;
    //gYmodemData.mifilesize = gymodeminfo.mifilesize;
    //memcpy( gYmodemData.mpBuff , pinbuff , len > 1024 ? 1024 : len );
    //gYmodemData.miLen = len ;

    memset( &gYmodemMSG , 0 , sizeof(gYmodemMSG) );
    
    gYmodemMSG.msgID = _FILE_DATA_MSG;
    memcpy( gYmodemMSG.MsgData.FileData.mpBuff , pinbuff , len > 1024 ? 1024 : len );
    gYmodemMSG.MsgData.FileData.miLen = len ;
    gYmodemMSG.MsgData.FileData.mifilesize = gymodeminfo.mifilesize ;
    gYmodemMSG.MsgData.FileData.miRevfilelen = gymodeminfo.miRevfileLen ;    // 未包括当前的数据

    if( gymodeminfo.mpYmodemCB )
    {
        if( 0 == gymodeminfo.mpYmodemCB( &gYmodemMSG )  )
        {
            YModem_ACK();
            gymodeminfo.miRevfileLen += len ;
            _WAIT_YMODEM_TIMEOUT( ymodem_revdata_timeout , 1000 );
        }
        else        // 
        {
            //_CAN_YMODEM ;
            YModem_NAK();
            _WAIT_YMODEM_TIMEOUT( ymodem_revdata_timeout , 1000 );
        }
    }
    else
    {
        _CAN_YMODEM ;
    }
}
// 发送数据成功
static void YModemSendFileSuccessMsg(void)
{
    // 先返回
    YModem_ACK();
    //
    memset( &gYmodemMSG , 0 , sizeof(gYmodemMSG) );
    
    gYmodemMSG.msgID = _FILE_SUCCESS_MSG;
    gYmodemMSG.MsgData.NewFile.mpfileName = gymodeminfo.mpfileName ;
    gYmodemMSG.MsgData.NewFile.filesize = gymodeminfo.mifilesize ;

    //gYmodeInfo.msgID = _FILE_SUCCESS_MSG ;
    //gYmodeInfo.mfileName = gymodeminfo.mpfileName ;
    //gYmodeInfo.filesize = gymodeminfo.mifilesize  ;

    if( gymodeminfo.mpYmodemCB )
    {
        gymodeminfo.mpYmodemCB( &gYmodemMSG ) ;
    }
    _DONE_YMODEM ;
}

// 发送失败
static void YModemSendTimeoutMsg(void)
{
    uint8_t result = 0 ;
    gYmodemMSG.msgID = _TIMEOUT_MSG ;
    if( gymodeminfo.mpYmodemCB )
    {
        gymodeminfo.mpYmodemCB( &gYmodemMSG ) ;
    }
}



static void ymodemRx_Init(void)
{
    gymoderx.miHead = gymoderx.miTail = 0x00 ;
}
static void ymodemRx_Push(uint8_t* pinbuff ,uint16_t inlen )
{
    uint16_t i = 0 ;
    if( !pinbuff || !inlen ) return ;
    for( i = 0 ; i < inlen ; i++ )
    {
        gymoderx.mpBuff[gymoderx.miTail] = pinbuff[i];
        gymoderx.miTail = ( gymoderx.miTail + 1 )%_YMODEM_BUFF_SIZE;
    }
}

//
static uint16_t ymodeRx_Dcode(uint8_t* poutbuff ,uint16_t outsize )
{
    uint16_t pos = 0 ;
    uint16_t len = 0 ;  
    uint16_t crc = 0x00 ;
    if( gymoderx.miHead == gymoderx.miTail ) return 0 ;

_REYMODEDCODE:    while( gymoderx.miHead != gymoderx.miTail )
    {
        if( _YMODE_HEAD( gymoderx.mpBuff[gymoderx.miHead] ) ) break ;
        gymoderx.miHead = ( gymoderx.miHead + 1 )%_YMODEM_BUFF_SIZE;
    }
    pos = gymoderx.miHead ;
    len = 0 ;    
    while( pos != gymoderx.miTail && len < outsize )
    {
        poutbuff[len++] = gymoderx.mpBuff[pos];
        pos = ( pos + 1 )%_YMODEM_BUFF_SIZE;
        switch( poutbuff[0] )
        {
            
            case MODEM_EOT :
                if( len == _EOT_PACK_LEN )
                {
                    // check crc
                    gymoderx.miHead = pos ;
                    return _EOT_PACK_LEN ;
                }
                break ;
            case MODEM_SOH :
                if( len == _SOH_PACK_LEN )
                {
                    // check crc
                    crc = _CRC16( poutbuff + 3 , 128 );
                    if( crc == ( ( poutbuff[3 + 128] << 8 ) | poutbuff[3+ 128 + 1] ) )
                    {
                        gymoderx.miHead = pos ;
                        return _SOH_PACK_LEN ;
                    }
                    else
                    {
                        gymoderx.miHead = pos ;
                        goto _REYMODEDCODE ;
                    }
                    
                }
                break ;
            case  MODEM_STX :
                if( len == _STX_PACK_LEN )
                {
                    // check crc
                    
                    crc = _CRC16( poutbuff + 3 , 1024 );
                    if( crc == (  ( poutbuff[3 + 1024] << 8 ) | poutbuff[3+ 1024 + 1] ) )
                    {
                        gymoderx.miHead = pos ;
                        return _STX_PACK_LEN ;
                    }
                    else
                    {
                        gymoderx.miHead = pos ;
                        goto _REYMODEDCODE ;
                    }
                    
                }
                break ;
            default :
                gymoderx.miHead = ( gymoderx.miHead + 1 )%_YMODEM_BUFF_SIZE;
                goto _REYMODEDCODE;
                break ;
        }
    }
    //
    if( len >= outsize )
    {
        gymoderx.miHead = ( gymoderx.miHead + 1 )%_YMODEM_BUFF_SIZE;
        goto _REYMODEDCODE;
    }    
    return 0 ;
}



static uint16_t _CRC16(uint8_t * pinbuff , uint16_t len )
{
    uint16_t ckcrc = 0x00 , i = 0 , j = 0  ;
    for( i = 0 ; i < len ; i++ )
    {
        ckcrc = ckcrc ^ (int)pinbuff[i] << 8 ;
        for( j = 8 ; j != 0 ; j-- )
        {
            if( ckcrc & 0x8000 ) ckcrc = ckcrc << 1 ^ 0x1021 ;
            else ckcrc <<= 1 ;
        }
    }
    return ckcrc ;
}



uint8_t YModem_C(void)
{
    uint8_t buff[1] = { MODEM_C };
    if( gymodeminfo.mpYmodemSendFun )
    {
        gymodeminfo.mpYmodemSendFun( buff , 1 );
        return 0 ;
    }
    return 1 ;    
}

uint8_t YModem_ACK(void)
{
    uint8_t buff[1] = { MODEM_ACK };
    if( gymodeminfo.mpYmodemSendFun )
    {
        gymodeminfo.mpYmodemSendFun( buff , 1 );
        return 0 ;
    }
    return 1 ; 
}

uint8_t YModem_NAK(void)
{
    uint8_t buff[1] = { MODEM_NAK };
    if( gymodeminfo.mpYmodemSendFun )
    {
        gymodeminfo.mpYmodemSendFun( buff , 1 );
        return 0 ;
    }
    return 1 ; 
}

uint8_t YModem_CAN(void)
{
    uint8_t buff[1] = { MODEM_CAN };
    if( gymodeminfo.mpYmodemSendFun )
    {
        gymodeminfo.mpYmodemSendFun( buff , 1 );
        return 0 ;
    }
    return 1 ; 
}







