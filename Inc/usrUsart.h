/*****************************************
 *
 * 
 * 
 * 
 * 
 * 
**************************************************/




#ifndef     __USR_USART__H_

#define     __USR_USART__H_

#ifdef __cplusplus
 extern "C" {
#endif

// BUFF
 
 typedef struct usrUsart
 {
     unsigned char  *mpBuff ;
     unsigned short miHead ;
     unsigned short miTail ;
     unsigned short miSize ;
 }LoopArry , *pLoopArry ;
 
#define     _LOOP_X(x,l,n)          (x) = ( ( (x) + (n) ) % (l) )

static void usrLoopArryInit( pLoopArry , unsigned char *pArry , unsigned short AssrySzie);
void usrLoopArryPush( pLoopArry , unsigned char *pinbuff , unsigned short len );
unsigned short usrLoopArryPop( pLoopArry , unsigned char *poutbuff , unsigned short size );



//
void MX_USART1_UART_Init(void);
void MX_USART1_UART_Send_Bytes(unsigned char*,unsigned short len);
void rt_kprintf(const char *fmt, ...);
void Usart1RXTx_Init(void);



#ifdef __cplusplus
}
#endif



#endif //__USR_USART__H_

