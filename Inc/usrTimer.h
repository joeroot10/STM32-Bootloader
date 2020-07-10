/**************************************************
**
**
**
**
**************************************************/



#ifndef         __USR_TIMER__H_

#define         __USR_TIMER__H_

#ifdef __cplusplus
 extern "C" {
#endif


#define		SYS_TIMER_MAX			(8)



typedef enum
{
	TIMER_IDLE = 0,
	TIMER_CREATE,
	TIMER_RUNING,
	TIME_WAIT,
	TIMER_OVER,
	TIMER_STOP,
	TIMER_DEL,
}TIMER_ST;

typedef void  (*timer_cb)(void);

typedef struct
{
	TIMER_ST	    mTimeST;
	uint32_t		mTotleSec;
	uint32_t		mCurSec;
	timer_cb	    mTimeCB;
}TIMERCB;

void  InitTimer(void);
uint8_t CreateTimer(timer_cb);
void  StartTimer(uint8_t id, uint32_t msec);
void  StopTimer(uint8_t id);
void  DeleteTimer(uint8_t id);
uint8_t  SwitchTimer(uint8_t id);
void  TimerHandle(void);
void TimeSchedule(void);
static void TimerSendMSG(void);



#define         _TIME_MSG_SIZE              (16)
typedef struct
{
    uint16_t      msgID ;
    uint32_t      mlParam ;
    uint32_t      mwParam ;
}CellMSG;

typedef struct
{
    CellMSG     mMSG[_TIME_MSG_SIZE];
    uint16_t    miTail;
    uint16_t    miHead ;
}MsgList;













#define     DEL_TIMER(id)    do{DeleteTimer(id);id = 0xFF;}while(0)



#ifdef __cplusplus
}
#endif


#endif //__USR_TIMER__H_








