/************************************************
 * 
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
#include "bootloader.h"
#include "usrUsart.h"
#include "usrTimer.h"


static _keyHandler gKeyHandlerlist[5] = {0};
static uint8_t gKeyhandlerInde = 0;

typedef  void (*pFunction)(void);
uint32_t JumpAddress;
pFunction Jump_To_Application;

static uint8_t gInputTimeoutTimerID = 0xFF;
static uint8_t gDcodeRxTimerID = 0xFF ;

void _RunAppAddr(uint32_t ApplicationAddress )
{
    DEL_TIMER(gInputTimeoutTimerID);
    DEL_TIMER(gDcodeRxTimerID);
    if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FF00000/*0x2FFE0000*/ ) == 0x20000000)
    { 
        __disable_irq();       
        JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
        Jump_To_Application = (pFunction) JumpAddress;
       __set_MSP(*(__IO uint32_t*) ApplicationAddress);
       Jump_To_Application();
    }
}


void StartBootLoader(void)
{
    DEL_TIMER(gInputTimeoutTimerID);
    DEL_TIMER(gDcodeRxTimerID);
    BootLoaderMenu();
}

/*
static _keyHandler gKeyHandlerlist[5] = {0};
static uint8_t gKeyhandlerInde = 0;
*/
void BootLoaderMenu(void)
{
    UpdataHeader updatahead ;
    uint8_t patch_flag = 0 , mimage_flag = 0 ;
    gKeyhandlerInde = 0 ;
    memset( gKeyHandlerlist , 0 , sizeof(gKeyHandlerlist) );
    DEL_TIMER(gInputTimeoutTimerID);
    DEL_TIMER(gDcodeRxTimerID);

    //先检测是否有path
    if( inflash_reads( _PATCH_MAGIC_ADDR , (uint8_t*)&updatahead,sizeof(updatahead) ) )
    {
        if( updatahead.file_type == _FILE_LOCAL_PATCH )
        {
            patch_flag = 1 ;
        }
    }
    if( inflash_reads( _IMAGE_MAGIC_ADDR , (uint8_t*)&updatahead,sizeof(updatahead) ) )
    {
        if( updatahead.file_type == _FILE_LOCAL_IMAGE )
        {
            mimage_flag = 1 ;
        }
    }
    // 主界面
    MX_USART1_UART_Send_Bytes( _BOOTLOAD_HEAD_STR , sizeof(_BOOTLOAD_HEAD_STR) );
    rt_kprintf( _DWN_IMAGE_TO_INFLASH , ++gKeyhandlerInde);
    gKeyHandlerlist[gKeyhandlerInde-1] = dwn_image_to_inflash;   // 
    if( mimage_flag )
    {
        rt_kprintf( _UPD_IMAGE_FR_INFLASH , ++gKeyhandlerInde);
        gKeyHandlerlist[gKeyhandlerInde-1] = upd_image_to_inflash;   // 
    }
    rt_kprintf( _DWN_PATCH_TO_EXFLASH , ++gKeyhandlerInde);
    gKeyHandlerlist[gKeyhandlerInde-1] = dwn_patch_to_inflash;   // 
    if( patch_flag )
    {
        rt_kprintf( _UPD_PATCH_TO_EXFLASH , ++gKeyhandlerInde);
        gKeyHandlerlist[gKeyhandlerInde-1] = upd_patch_to_inflash;   // 
    }
    //
    if( _IS_APP( _APP_ADDR) )
    {
        rt_kprintf( _EXECUTE_NEW_PROGRAM , ++gKeyhandlerInde);
        gKeyHandlerlist[gKeyhandlerInde-1] = execute_app;   // 
        // 启动定时器
        gInputTimeoutTimerID = CreateTimer(WaitInputTimeout);
        StartTimer(gInputTimeoutTimerID,3*1000);    // 3s
    }
    MX_USART1_UART_Send_Bytes(_BOOTLOAD_TAIL_STR , sizeof(_BOOTLOAD_TAIL_STR) );
    rt_kprintf(_INPUT_BOOTLOADER_STR,1,gKeyhandlerInde);
    //
    gDcodeRxTimerID = CreateTimer(UsartRxTimer);
    StartTimer(gDcodeRxTimerID,30);    // 3s
    Usart1RXTx_Init();                 // 清除缓冲区
}


uint8_t CreateUpImageMagic( uint32_t addr ,uint8_t t ,uint32_t size )
{
    UpdataHeader updatahead ;
    memset( &updatahead , 0 , sizeof(UpdataHeader) );
    memcpy( updatahead.file_magic, _MAGIC_STR , _MAGIC_SIZE );
    updatahead.file_type = t ;
    updatahead.file_size = size ;
    return inflash_writes( addr , 0 , &updatahead , sizeof(updatahead) );
}

uint8_t ClearUpMagic(uint32_t addr )
{
    return inflash_erases( addr , _MAGIC_SIZE );
}



/*
    下载全量升级文件
*/
static void dwn_image_to_inflash(void)
{
    rt_kprintf("===Download Image to the STM32 Internal Flash\r\n");
    dwn_image_inflash();
}
/*
    更新全量升级文件
*/
static void upd_image_to_inflash(void)
{
    UpdataHeader updatahead = {0x00};
    rt_kprintf("===Upload Image From the STM32 Internal Flash\r\n");
    inflash_reads( _IMAGE_MAGIC_ADDR , &updatahead,sizeof(updatahead));
    if( updatahead.file_type != _FILE_LOCAL_IMAGE )
    {
        rt_kprintf("\r\nUpload Image From the STM32 External Flash...Fail\r\n");
    }
    else
    {
        up_image_inflash();
    }
    ClearUpMagic( _IMAGE_MAGIC_ADDR );
    BootLoaderMenu();
}
/*
    下载差分升级包
*/
static void dwn_patch_to_inflash(void)
{
    rt_kprintf("===Upload Path From the STM32 Internal Flash\r\n");
    dwn_patch_inflash();
}
/*
    更新差分升级包
*/
static void upd_patch_to_inflash(void)
{
    uint32_t new_size = 0 ;
    UpdataHeader updatahead = {0x00};
    rt_kprintf("===Bspatch  Patch to the STM32 Internal Flash\r\n");
    
    if( 0 != read_flash(_PATCH_MAGIC_ADDR, 0, 
            (uint8_t *)&updatahead, sizeof(updatahead) ) )
    {
        ClearUpMagic( _PATCH_MAGIC_ADDR ) ;
        rt_kprintf("\r\nBspatch  Patch to the STM32 Internal Flash...Fail\r\n");
        BootLoaderMenu();
        return ;
    }
    //
    new_size = bspatch( _APP_ADDR ,_IMAGE_ADDR ,_PATCH_ADDR ) ;
    ClearUpMagic( _PATCH_MAGIC_ADDR ) ;   
    if( new_size > 0 )
    {        
        CreateUpImageMagic( _IMAGE_MAGIC_ADDR  ,
                    _FILE_MANUAL | _FILE_LOCAL_IMAGE ,
                    new_size);
        
        rt_kprintf("\r\nBspatch  Patch to the STM32 Internal Flash...Success\r\n");
    }
    else
    {
        rt_kprintf("\r\nBspatch  Patch to the STM32 Internal Flash...Fail[%d]\r\n",new_size);
    }
    BootLoaderMenu();
}

static void execute_app(void)
{
    rt_kprintf("\r\nExecute The New Program---%x\r\n" , _APP_ADDR );
    _RunAppAddr( _APP_ADDR );
}


static void WaitInputTimeout(void)
{
    DEL_TIMER( gInputTimeoutTimerID );
    DEL_TIMER( gDcodeRxTimerID );
    rt_kprintf("\r\nTimeout...Execute The New Program---%x\r\n" , _APP_ADDR );
    _RunAppAddr( _APP_ADDR );
}

extern LoopArry gRxLoopBuff ;
static void UsartRxTimer(void)
{
    uint8_t key = 0 ;
    while( usrLoopArryPop( &gRxLoopBuff,&key,sizeof(key)) )
    {
        if( _IS_DIGIT(key) )
        {
            key -= '0';
            if( key > 0 && key < gKeyhandlerInde )
            {
                if( gKeyHandlerlist[key-1] )
                {
                    DEL_TIMER( gInputTimeoutTimerID );
                    DEL_TIMER( gDcodeRxTimerID );
                    gKeyHandlerlist[key-1]();
                }
                return ;
            }
        }
        rt_kprintf(_INPUT_BOOTLOADER_STR,1,gKeyhandlerInde);
    }
    StartTimer(gDcodeRxTimerID,30);    // 3s
}


uint8_t read_flash( uint32_t addr , uint32_t offset , uint8_t *out , uint32_t size )
{
    inflash_reads( addr + offset , out, size);
    return 0;
}


uint8_t write_flash( uint32_t addr , uint32_t offset , uint8_t *in , uint32_t size )
{
    return inflash_writes( addr + offset , in , size );
}