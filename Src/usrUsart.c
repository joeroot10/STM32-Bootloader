/*****************************
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
*****************************************/
#include "main.h"
#include <string.h>
#include <stdio.h>  
#include <stdarg.h>
#include "usrUsart.h"

#define         _USART_TX_BUFF_SIZE         (128)
static unsigned char gTXBuff[_USART_TX_BUFF_SIZE];
LoopArry    gTxLoopBuff ;
static unsigned char gTXSendIng = 0;

#define         _USART_RX_BUFF_SIZE         (1024+256)
static unsigned char gRXBuff[_USART_RX_BUFF_SIZE];
LoopArry    gRxLoopBuff ;


// BUFF
static void usrLoopArryInit( pLoopArry pLArry, unsigned char *pArry , unsigned short AssrySzie)
{
    if( !pLArry || !pArry || !AssrySzie ) return ;
    pLArry->mpBuff = pArry ;
    pLArry->miHead = 0 ;
    pLArry->miTail = 0 ;
    pLArry->miSize = AssrySzie ;
}
void usrLoopArryPush( pLoopArry pLArry , unsigned char *pinbuff , unsigned short len )
{
    unsigned short i = 0 ;
    if( !pLArry || !pinbuff || !len ) return ;
    for( i = 0 ; i < len ; i++ )
    {
        pLArry->mpBuff[pLArry->miTail] = pinbuff[i];
        _LOOP_X( pLArry->miTail , pLArry->miSize , 1 ) ;
    }
}

unsigned short usrLoopArryPop( pLoopArry pLArry, unsigned char *poutbuff , unsigned short size )
{
    unsigned short ilen = 0 ;
    if( !pLArry || !poutbuff || !size ) return 0;
    while ( pLArry->miHead != pLArry->miTail && ilen < size )
    {
        poutbuff[ilen++] = pLArry->mpBuff[pLArry->miHead];
        _LOOP_X( pLArry->miHead , pLArry->miSize , 1 ) ;
    }
    return ilen ;
}

void Usart1RXTx_Init(void)
{
    usrLoopArryInit(&gTxLoopBuff,gTXBuff,sizeof(gTXBuff));
    usrLoopArryInit(&gRxLoopBuff,gRXBuff,sizeof(gRXBuff));
}
//
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
  /**USART1 GPIO Configuration  
  PA9   ------> USART1_TX
  PA10   ------> USART1_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART1 interrupt Init */
  NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART1_IRQn);

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART1);
  LL_USART_Enable(USART1);
  /* USER CODE BEGIN USART1_Init 2 */

  //LL_USART_EnableIT_IDLE(USART1);
  //if(LL_USART_IsActiveFlag_IDLE(USART1))
  //__HAL_UART_ENABLE_IT(&huart8, UART_IT_IDLE);

  /* USER CODE END USART1_Init 2 */
  // 启动接收中断
    Usart1RXTx_Init();
    // 使能接收中断
    LL_USART_EnableIT_RXNE(USART1);
    gTXSendIng = 0 ;
}

static uint8_t usart_data_r ;
static uint8_t usart_data_s ;
extern uint8_t ymodem_rev_data(uint8_t* pinbuff ,uint16_t len );

void _USART1_IRQ(void)
{
    if(LL_USART_IsActiveFlag_RXNE(USART1))  // rx
    {
        usart_data_r = LL_USART_ReceiveData8(USART1);
        usrLoopArryPush( &gRxLoopBuff,&usart_data_r,sizeof(usart_data_r));
        ymodem_rev_data(&usart_data_r , sizeof(usart_data_r) );
    }
    //
    if(LL_USART_IsActiveFlag_TXE(USART1))   // tX
    {
        if( usrLoopArryPop( &gTxLoopBuff,&usart_data_s,sizeof(usart_data_s)))
        {
            LL_USART_TransmitData8( USART1 , usart_data_s );
        }
        else
        {
            // 禁用TX中断
            LL_USART_DisableIT_TXE(USART1);
            gTXSendIng = 0 ;
        }
        
    }
}



void MX_USART1_UART_Send_Bytes(unsigned char* pinbuff ,unsigned short len)
{
    usrLoopArryPush( &gTxLoopBuff, pinbuff , len );
    if( gTXSendIng ) return ;
    gTXSendIng = 1 ;
    if( usrLoopArryPop( &gTxLoopBuff,&usart_data_s,sizeof(usart_data_s)))
    {
        LL_USART_TransmitData8( USART1 , usart_data_s );
        LL_USART_EnableIT_TXE(USART1);
    }
}


void rt_kprintf(const char *fmt, ...)
{
  char szData[256] = {0x00};
	va_list vfm; 
	int va_len = 0 ;
	va_start(vfm, fmt);
	va_len = vsprintf(szData, fmt, vfm);
	va_end(vfm);
  MX_USART1_UART_Send_Bytes( (uint8_t*)szData , va_len );
}