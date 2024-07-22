#pragma once
#include "def.h"


#define REALTIMER_CALLBACK_MAX	256

#define SHUTDOWN_SCREEN_DELAY 360

#pragma pack(1)
typedef struct
{
	DWORD func;
	DWORD ticks;
	DWORD tickcnt;
	DWORD param1;
	DWORD param2;
	DWORD param3;
	DWORD param4;
}TIMER_PROC_PARAM;
#pragma pack()


unsigned char readCmosPort(unsigned char port);

void writeCmosPort(unsigned char port, unsigned char value);

char * dayOfWeek2str(int dw);

unsigned short bcd2asc(char bcd);

unsigned char bcd2binary(char bcd);

int __kAddRealTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

void __kRemoveRealTimer(int no);

void initDPC();

extern "C" __declspec(dllexport) void __kCmosTimer();

extern "C" __declspec(dllexport) void __kCmosExactTimerProc();
