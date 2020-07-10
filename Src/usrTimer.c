/**********************************************
**
**
**
**
**
**
**
**
*****************************************************/



#include "stdint.h"
#include "usrTimer.h"


static TIMERCB timecb[SYS_TIMER_MAX] = {0x00};


static MsgList gTimeList = { 0x00 };


void  InitTimer(void)
{
    memset( (void*)timecb , 0 , sizeof(timecb) );
    gTimeList.miHead = gTimeList.miTail = 0x00 ;
}
uint8_t CreateTimer(timer_cb cb )
{
	uint8_t i = 0;
	if(!cb) return 0xFF;
	for(i = 0; i < SYS_TIMER_MAX; i++)
	{
		if(timecb[i].mTimeST == TIMER_IDLE ||
		    timecb[i].mTimeST == TIMER_DEL)
		{
			timecb[i].mTotleSec = 0;
			timecb[i].mCurSec = 0;
			timecb[i].mTimeCB = cb;
			timecb[i].mTimeST = TIMER_CREATE;
			return i;
		}
	}
	return 0xFF;
}

void  StartTimer(uint8_t id, uint32_t msec)
{
	if(id >= SYS_TIMER_MAX || msec == 0) return ;
	timecb[id].mTotleSec = msec;
	timecb[id].mCurSec = 0;
	timecb[id].mTimeST = TIMER_RUNING;
}

void  StopTimer(uint8_t id)
{
	if(id >= SYS_TIMER_MAX) return ;
	timecb[id].mTimeST = TIMER_STOP;
}

void  DeleteTimer(uint8_t id)
{
	if(id >= SYS_TIMER_MAX) return ;
	timecb[id].mTimeST = TIMER_DEL;
}

uint8_t  SwitchTimer(uint8_t id)
{
	if(id >= SYS_TIMER_MAX) return 0;
	return (timecb[id].mTimeST == TIMER_RUNING);
}

void  TimerHandle(void)
{
	uint8_t i = 0;
	timer_cb cb = 0;
	for(i = 0; i < SYS_TIMER_MAX; i++)
	{
		if(timecb[i].mTimeST == TIMER_RUNING)
		{
			if(++timecb[i].mCurSec >= timecb[i].mTotleSec)
			{
				timecb[i].mCurSec = 0x00;
                timecb[i].mTimeST = TIMER_STOP ;
				if(cb = timecb[i].mTimeCB) cb();
			}
		}
	}
}

static void TimerSendMSG(void)
{
    gTimeList.mMSG[gTimeList.miTail].msgID = 0x00 ;
    gTimeList.miTail = (gTimeList.miTail+1)&(_TIME_MSG_SIZE-1);
    if( gTimeList.miTail == gTimeList.miHead )  // 
    {
        gTimeList.miHead = (gTimeList.miHead+1)&(_TIME_MSG_SIZE-1);
    }
}


void HAL_TickHdlr(void)
{
    // 只能发送消息
    TimerSendMSG();
}

void TimeSchedule(void)
{
    if( gTimeList.miTail != gTimeList.miHead )
    {
        gTimeList.miHead = (gTimeList.miHead+1)&(_TIME_MSG_SIZE-1);
        TimerHandle();
    }
}








