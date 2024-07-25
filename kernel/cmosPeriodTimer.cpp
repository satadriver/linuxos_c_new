
#include "cmosPeriodTimer.h"
#include "cmosAlarm.h"
#include "Utils.h"
#include "video.h"
#include "ListEntry.h"
#include "hardware.h"
#include "servicesProc.h"
#include "cmosExactTimer.h"
#include "timer8254.h"


unsigned char readCmosPort(unsigned char port) {

	outportb(0x70, port);
	return inportb(0x71);

	__asm {
		//in al,70h
		//and al,80h
		//or al,port
	}
}

void writeCmosPort(unsigned char port, unsigned char value) {

	outportb(0x70, port);
	outportb(0x71, value);
	__asm {
		//in al, 70h
		//and al, 80h
		//or al, port
	}
}


unsigned char bcd2binary(char bcd) {
	int low = (bcd & 0xf) ;
	int high = (bcd >> 4)*10;
	return low + high;
}


unsigned short bcd2asc(char bcd) {
	int low = (bcd & 0xf) + 0x30;
	int high = (bcd >> 4) + 0x30;
	return (low << 8) + high;
}

void __kPeriodTimer() {
	char c = readCmosPort(0x32);
	char y = readCmosPort(9);
	char m = readCmosPort(8);
	char d = readCmosPort(7);
	char hour = readCmosPort(4);
	char minute = readCmosPort(2);
	char second = readCmosPort(0);

	int strc = bcd2asc(c);
	int stry = bcd2asc(y);
	int strm = bcd2asc(m);
	int strd = bcd2asc(d);
	int strhour = bcd2asc(hour);
	int strminute = bcd2asc(minute);
	int strsecond = bcd2asc(second);

	char dw = readCmosPort(6);
	char *strdw = dayOfWeek2str(dw);

	char szout[256];
	
	char*  singlefmt = "%s%s/%s/%s [%s] %s:%s:%s";
	char* doublefmt =  "%s%s-%s-%s [%s] %s:%s:%s";
	char*  thirdfmt =  "%s%s\\%s\\%s [%s] %s:%s:%s";
	char * fmt = 0;

	int fontcolor = 0;

	int binsec = bcd2binary(second);
	if ((binsec % 3) == 0)
	{
		fmt = singlefmt;
		fontcolor = CMOS_TIMESTAMP_SINGLE_COLOR;
	}
	else if ((binsec % 3) == 1)
	{
		fmt = doublefmt;
		fontcolor = CMOS_TIMESTAMP_DOUBLE_COLOR;
	}else if ((binsec % 3) == 2)
	{
		fmt = thirdfmt;
		fontcolor = CMOS_TIMESTAMP_THIRD_COLOR;
	}

	__sprintf(szout,fmt , &strc, &stry, &strm, &strd, strdw, &strhour, &strminute, &strsecond);
	__strcpy((char*)CMOS_DATETIME_STRING, szout);

	DWORD* lptickcnt = (DWORD*)CMOS_PERIOD_TICK_COUNT;
	(*lptickcnt)++;

	lptickcnt = (DWORD*)SLEEP_TIMER_RECORD;
	(*lptickcnt)++;
	if (*lptickcnt >= SHUTDOWN_SCREEN_DELAY)
	{
		__asm {
			mov eax, TURNOFF_SCREEN
			int 80h
		}
		*lptickcnt = 0;
	}

	DWORD pos = (gVideoHeight - GRAPHCHAR_HEIGHT) * gVideoWidth * gBytesPerPixel;
	__drawGraphChar((unsigned char*)szout, fontcolor, pos, TASKBARCOLOR);

	__kPeriodTimerProc();
}




char* dayOfWeek2str(int n) {

	char* dayweek[8] = { {"Sunday"},{"Monday"},"Tuesday","Wednesday","Thursday","Friday","Saturday","Other" };

	return dayweek[n];
}


void initTimer() {
	initPeriodTimer();
	initExactTimer();
	init8254Timer();
}


TIMER_PROC_PARAM gPeriodTimer[REALTIMER_CALLBACK_MAX] = { 0 };


void initPeriodTimer() {
	__memset((char*)gPeriodTimer, 0, REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM));
}


int __kAddPeriodTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {
	if (addr == 0 || delay == 0)
	{
		return -1;
	}

	//char szout[1024];
	//__printf(szout, "__kAddCmosTimer addr:%x,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", 
	// addr,delay,param1,param2,param3,param4);

	DWORD* lptickcnt = (DWORD*)CMOS_PERIOD_TICK_COUNT;

	DWORD ticks = delay;		

	int i = 0;
	for (i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (gPeriodTimer[i].func == 0 && gPeriodTimer[i].tickcnt == 0)
		{
			gPeriodTimer[i].func = addr;
			gPeriodTimer[i].ticks = ticks;
			gPeriodTimer[i].tickcnt = *lptickcnt + ticks;
			gPeriodTimer[i].param1 = param1;
			gPeriodTimer[i].param2 = param2;
			gPeriodTimer[i].param3 = param3;
			gPeriodTimer[i].param4 = param4;
			break;
		}
	}

	return i;
}



void __kRemovePeriodTimer(int no) {
	if (no >= 0 && no < REALTIMER_CALLBACK_MAX)
	{
		gPeriodTimer[no].func = 0;
		gPeriodTimer[no].tickcnt = 0;
	}
}



void __kPeriodTimerProc() {

	int result = 0;
	DWORD* lptickcnt = (DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD tickcnt = *lptickcnt;
	//in both c and c++ language,the * priority is lower than ++
	(*lptickcnt)++;

	for (int i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (gPeriodTimer[i].func)
		{
			if (gPeriodTimer[i].tickcnt < *lptickcnt)
			{

				gPeriodTimer[i].tickcnt = *lptickcnt + gPeriodTimer[i].ticks;

				typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
				ptrfunction lpfunction = (ptrfunction)gPeriodTimer[i].func;
				result = lpfunction(gPeriodTimer[i].param1, gPeriodTimer[i].param2, gPeriodTimer[i].param3, gPeriodTimer[i].param4);
			}
		}
	}
}