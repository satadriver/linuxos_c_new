#pragma once
#pragma once
#include "def.h"
#include "task.h"


#pragma pack(1)

typedef struct {

	DWORD		eip;
	DWORD		cs;
	DWORD		eflags;
}ExceptionStack0;

typedef struct {

	ExceptionStack0 stack0;
	DWORD		esp3;
	DWORD		ss3;
}ExceptionStack3;

typedef struct {

	ExceptionStack3 stack3;
	DWORD		gs;
	DWORD		fs;
	DWORD		ds;
	DWORD		es;
}ExceptionStackV86;

typedef struct {
	DWORD		errcode;
	ExceptionStack0 stack0;
}ExceptionErrorStack0;

typedef struct {
	DWORD		errcode;
	ExceptionStack3 stack3;
}ExceptionErrorStack3;

typedef struct {
	DWORD		errcode;
	ExceptionStackV86 stackv86;

}ExceptionErrorStackV86;










#pragma pack()

#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) void __kException(DWORD);
#else

extern "C" __declspec(dllimport) void __kException(DWORD);
#endif

void  Com1IntProc(LIGHT_ENVIRONMENT* stack);
void  Com2IntProc(LIGHT_ENVIRONMENT* stack);

void  Parallel2IntProc(LIGHT_ENVIRONMENT* stack);
void  Parallel1IntProc(LIGHT_ENVIRONMENT* stack);

void  FloppyDiskIntProc(LIGHT_ENVIRONMENT* stack);
void  SlaveIntProc(LIGHT_ENVIRONMENT* stack);

void  NetcardIntProc(LIGHT_ENVIRONMENT* stack);
void  USBIntProc(LIGHT_ENVIRONMENT* stack);

void  CoprocessorIntProc(LIGHT_ENVIRONMENT* stack);
void  IDEMasterIntProc(LIGHT_ENVIRONMENT* stack);

void  IDESlaveIntProc(LIGHT_ENVIRONMENT* stack);

void Slave1IntProc(LIGHT_ENVIRONMENT* stack);

extern "C" void TimerInterrupt(LIGHT_ENVIRONMENT * stack);

extern "C" void  CmosInterrupt(LIGHT_ENVIRONMENT * stack);


void div0Exception(LIGHT_ENVIRONMENT* stack);

void NmiException(LIGHT_ENVIRONMENT* stack);

void  overflowException(LIGHT_ENVIRONMENT* stack);

void boundCheckException(LIGHT_ENVIRONMENT* stack);

void illegalOperandException(LIGHT_ENVIRONMENT* stack);

void  deviceUnavailableException(LIGHT_ENVIRONMENT* stack);

void  doubleFaultException(LIGHT_ENVIRONMENT* stack);

void  coprocCrossBorderException(LIGHT_ENVIRONMENT* stack);

void  invalidTssException(LIGHT_ENVIRONMENT* stack);

void  segmentInexistException(LIGHT_ENVIRONMENT* stack);

void  stackException(LIGHT_ENVIRONMENT* stack);

void  generalProtectException(LIGHT_ENVIRONMENT* stack);

void  pageException(LIGHT_ENVIRONMENT* stack);

void anonymousException(LIGHT_ENVIRONMENT* stack);

void coprocessorException(LIGHT_ENVIRONMENT* stack);

void  alighCheckException(LIGHT_ENVIRONMENT* stack);

void  machineCheckException(LIGHT_ENVIRONMENT* stack);

void simdException(LIGHT_ENVIRONMENT* stack);

void virtualException(LIGHT_ENVIRONMENT* stack);