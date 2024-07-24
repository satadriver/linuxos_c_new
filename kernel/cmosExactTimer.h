#pragma once
#include "def.h"

#define REALTIMER_CALLBACK_MAX	256

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


extern "C" __declspec(dllexport) void __kExactTimerProc();

int __kAddExactTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

void __kRemoveExactTimer(int no);

void initExactTimer();