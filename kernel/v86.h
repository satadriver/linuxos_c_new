#pragma once

#include "def.h"


#pragma pack(1)


typedef struct {

	DWORD reax;
	DWORD recx;
	DWORD redx;
	DWORD rebx;
	DWORD resi;
	DWORD redi;
	DWORD rds;
	DWORD res;
	DWORD int_cmd;
	DWORD result;
}V86_INT_PARAMETER;

typedef struct {

	DWORD mode;
	DWORD x;
	DWORD y;
	DWORD bpp;
	DWORD base;
	DWORD size;
	DWORD offset;

}VesaSimpleInfo;



#pragma pack()


void saveScreen();

void restoreScreen();

int getVideoMode(VesaSimpleInfo vsi[64]);

int setGraphMode(int mode);

#ifdef DLL_EXPORT


extern "C" __declspec(dllexport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int rds, int cmd, int res);
#else

extern "C" __declspec(dllimport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi,int rds, int cmd, int res);
#endif