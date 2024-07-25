#pragma once
#include "def.h"




#define SHUTDOWN_SCREEN_DELAY	360


void initTimer();


unsigned char readCmosPort(unsigned char port);

void writeCmosPort(unsigned char port, unsigned char value);

char * dayOfWeek2str(int dw);

unsigned short bcd2asc(char bcd);

unsigned char bcd2binary(char bcd);

void __kPeriodTimerProc();

extern "C" __declspec(dllexport) void __kPeriodTimer();

int __kAddPeriodTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

void __kRemovePeriodTimer(int no);

void initPeriodTimer();


