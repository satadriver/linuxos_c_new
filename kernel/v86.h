#pragma once

#include "def.h"


#define VM_OUTPUT_BUSY_CONSTANT 1024



#pragma pack(1)


typedef struct
{
	unsigned char len;
	unsigned char reserved;
	unsigned short seccnt;
	unsigned int segoff;
	unsigned int secnolow;
	unsigned int secnohigh;
}INT13PAT, * LPINT13PAT;


typedef struct
{
	unsigned char bwork;
	unsigned char intno;
	unsigned int reax;		//2
	unsigned int recx;		//6
	unsigned int redx;		//a
	unsigned int rebx;		//e
	unsigned int resi;		//12
	unsigned int redi;		//16
	unsigned short res;		//1a
	unsigned short rds;		//1c
	unsigned int result;	//1e
}V86VMIPARAMS, * LPV86VMIPARAMS;


typedef struct {

	DWORD reax;
	DWORD recx;
	DWORD redx;
	DWORD rebx;
	DWORD resi;
	DWORD redi;
	DWORD rds;
	DWORD res;
	DWORD intnum;
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

int setVideoMode(int mode);

int rejectAtapi(int dev);

extern "C" __declspec(dllexport) int rejectCDROM(int dev);

int v86Int255Read(unsigned int secnum, DWORD secnumHigh, unsigned int seccnt, char* buf, int disk, int secsize);
int v86Int255Write(unsigned int secnum, DWORD secnumhigh, unsigned short seccnt, char* buf, int disk, int sectorsize);


int vm86ReadSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf);
int vm86WriteSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf);

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int getAtapiDev(int disk, int maxno);

extern "C" __declspec(dllexport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int rds, int cmd, int res);
extern "C"  __declspec(dllexport)  int v86Int13Read(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize);
extern "C"  __declspec(dllexport)  int v86Int13Write(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize);
#else
extern "C" __declspec(dllimport) int getAtapiDev(int disk, int maxno);
extern "C" __declspec(dllimport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi,int rds, int cmd, int res);
extern "C"  __declspec(dllimport)  int v86Int13Read(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize);
extern "C"  __declspec(dllimport)  int v86Int13Write(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize);
#endif