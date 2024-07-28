#pragma once

#include "def.h"



int checkAtapiPort(WORD port);

int writeAtapiCMD(unsigned short* cmd);

//int writesector(WORD port, char* buf);
//int readsector(WORD port, char* buf);



extern unsigned char gAtapiCmdOpen[16];
extern unsigned char gAtapiCmdClose[16];

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int atapiCMD(unsigned short* cmd);
extern "C" __declspec(dllexport) int writeAtapiSector(char* buf, unsigned int secnum, unsigned char seccnt);

extern "C" __declspec(dllexport) int readAtapiSector(char* buf, unsigned int secno, unsigned char seccnt);
#else
extern "C" __declspec(dllimport) int atapiCMD(unsigned short* cmd);
extern "C" __declspec(dllimport) int writeAtapiSector(char* buf, unsigned int secnum, unsigned char seccnt);

extern "C" __declspec(dllimport) int readAtapiSector(char* buf, unsigned int secno, unsigned char seccnt);

#endif
