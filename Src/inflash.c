/***************************************
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
***********************************************************/



#include "main.h"
#include "ymodem.h"
#include "bootloader.h"
#include "inflash.h"
#include "bspatch.h"
#include "usrUsart.h"


void inflash_init(void){}



/*
    return 
    0 : ok
    1 : fail
*/
static unsigned char ymode_image_inflash( pYmodemMSG pymodemsg )
{
    if( !pymodemsg ) return 1 ;
    switch ( pymodemsg->msgID )
    {
    case _FILE_INFO_MSG:        // 文件信息
        if( pymodemsg->MsgData.NewFile.filesize > _APP_SIZE ) return 1 ;
        return 0 ;  // Ok
        break;
    case _FILE_DATA_MSG :       // 文件数据
        return inflash_writes( _IMAGE_ADDR+pymodemsg->MsgData.FileData.miRevfilelen ,
								pymodemsg->MsgData.FileData.mpBuff ,
                pymodemsg->MsgData.FileData.miLen );
        break ;
    case _FILE_SUCCESS_MSG :    // 传输成功标识
        CreateUpImageMagic(_IMAGE_MAGIC_ADDR,_FILE_LOCAL_IMAGE,128);
        BootLoaderMenu();  // 重新开始
        return 0 ;
        break ;
    default:
        ClearUpMagic( _IMAGE_MAGIC_ADDR ) ;
        BootLoaderMenu();  // 重新开始
        break;
    }
    return 1 ;
}

/*
// image_magic
#define         _IMAGE_MAGIC_ADDR           ( _APP_ADDR + _APP_SIZE )
#define         _IMAGE_MAGIC_SIZE           (FLASH_PAGE_SIZE)
// image 
#define         _IMAGE_ADDR                 ( _IMAGE_MAGIC_ADDR + _IMAGE_MAGIC_SIZE )
#define         _IMAGE_SIZE                 (_APP_SIZE)

// patch_magic
#define         _PATCH_MAGIC_ADDR            ( _IMAGE_ADDR + _IMAGE_SIZE)
#define         _PATCH_MAGIC_SIZE            (FLASH_PAGE_SIZE)
// PATCH
#define         _PATCH_ADDR                  (_PATCH_MAGIC_ADDR+_PATCH_MAGIC_SIZE)
#define         _PATCH_SIZE                  (_APP_SIZE)
*/

void dwn_image_inflash(void)
{
    ymodem_init( ymode_image_inflash , MX_USART1_UART_Send_Bytes );
    ymodem_start();
}

/*
 Image -> app
*/
static uint8_t inflashbuff[FLASH_PAGE_SIZE] = {0x00};
void up_image_inflash(void)
{
    uint32_t offset = 0;
    while( offset < _APP_SIZE )
    {
        inflash_reads( _IMAGE_ADDR + offset , inflashbuff , sizeof(inflashbuff) );
        inflash_writes( _APP_ADDR + offset ,inflashbuff , sizeof(inflashbuff) );
        offset += sizeof(inflashbuff) ;
        MX_USART1_UART_Send_Bytes(".",1);
    }
    rt_kprintf("...Ok\r\nUpload Image From the STM32 Internal Flash Success\r\n");
}

static unsigned char ymode_patch_inflash( pYmodemMSG pymodemsg )
{
    if( !pymodemsg ) return 1 ;
    switch ( pymodemsg->msgID )
    {
    case _FILE_INFO_MSG:        // 文件信息
        if( pymodemsg->MsgData.NewFile.filesize > _APP_SIZE ) return 1 ;
        return 0 ;  // Ok
        break;
    case _FILE_DATA_MSG :       // 文件数据
        return inflash_writes( _PATCH_ADDR + pymodemsg->MsgData.FileData.miRevfilelen,
								pymodemsg->MsgData.FileData.mpBuff ,
                pymodemsg->MsgData.FileData.miLen );
        break ;
    case _FILE_SUCCESS_MSG :    // 传输成功标识
        CreateUpImageMagic(_PATCH_MAGIC_ADDR,_FILE_LOCAL_IMAGE,128);
        BootLoaderMenu();  // 重新开始
        return 0 ;
        break ;
    default:
        ClearUpMagic( _PATCH_MAGIC_ADDR ) ;
        BootLoaderMenu();  // 重新开始
        break;
    }
    return 1 ;
}

//
void dwn_patch_inflash(void)
{
    ymodem_init( ymode_patch_inflash , MX_USART1_UART_Send_Bytes );
    ymodem_start();
}


void up_patch_inflash(void)
{
    //
}


uint8_t inflash_writes( uint32_t addr , uint8_t *in , uint32_t size)
{
    uint16_t start_page = 0 , end_page = 0 ;
    
    start_page = addr / FLASH_PAGE_SIZE ;
    end_page = ( addr + size) / FLASH_PAGE_SIZE ;
    
    if( addr % FLASH_PAGE_SIZE == 0 )     // 如果刚好在页起始位置,则擦除之
    {
        if( HAL_OK != inflash_erase( addr , FLASH_PAGE_SIZE ) ) return 1 ;   // 停止
    }
    //
    while( start_page < end_page )
    {
        if( HAL_OK != inflash_erase( ++start_page * FLASH_PAGE_SIZE , FLASH_PAGE_SIZE ) ) return 1 ;   // 停止
    }
    //
    return ( HAL_OK == inflash_write( addr , in , size ) ? 0 : 1 );
}

uint8_t inflash_erases(uint32_t addr , uint32_t size)
{
    return ( HAL_OK == inflash_erase( addr , size ) ? 0 : 1 ) ;
}

uint32_t inflash_reads(uint32_t Addr , uint8_t *pOutBuff , uint32_t len)
{
    return inflash_read( Addr , pOutBuff , len );
}


// ymode 以 1024Byte or 128Byte 传输为单位,所以可以一页一页的檫除及写入
static HAL_StatusTypeDef inflash_erase( uint32_t pageAddr ,  uint32_t size )
{
    HAL_StatusTypeDef status ;
		uint32_t PAGEError ;
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_FLASH_Unlock();
    //
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = pageAddr;
    EraseInitStruct.NbPages     = size / FLASH_PAGE_SIZE;
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);// != HAL_OK
    HAL_FLASH_Lock();
    return status;
}


// 4byte or 8 byte 写入
static HAL_StatusTypeDef inflash_write( uint32_t Addr , uint8_t *pInBuff , uint32_t size )
{
    uint32_t Address = Addr ;
    uint32_t i = 0 ;
    HAL_StatusTypeDef status ;
    HAL_FLASH_Unlock();
    for( i = 0 ; i < size ; i += 4 )
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *(uint32_t*)(pInBuff+i)) ;
        if( status != HAL_OK ) break ;
        Address += 4 ;
    }
    HAL_FLASH_Lock();
    return status;
}


static uint32_t inflash_read( uint32_t Addr , uint8_t *pOutBuff , uint32_t len )
{
    memcpy( pOutBuff , (uint8_t*)Addr , len ) ;
    return len ;
}