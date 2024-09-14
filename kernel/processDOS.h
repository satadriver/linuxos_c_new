#pragma once

#include "def.h"
#include "process.h"


#pragma pack(1)

typedef struct  
{
	DWORD address;
	int size;
	DWORD status;
	DWORD pid;
}DOS_PE_CONTROL,*LPDOS_PE_CONTROL;

#pragma pack()


#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport ) int __createDosCodeProc(DWORD addr, char* filename);
#else
extern "C"  __declspec(dllimport) int __createDosCodeProc(DWORD addr, char* filename);
#endif



int __initDosTss(LPPROCESS_INFO tss, int num, DWORD addr, char * filename, char * funcname, DWORD syslevel, DWORD runparam);

int getDosPeAddr(int type,DWORD filedata,int size,int pid);

int relocDos(DWORD loadseg);

DWORD __initDosExe(int type,DWORD filedata, int filesize,int pid);