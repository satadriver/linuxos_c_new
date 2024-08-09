#pragma once


#include "def.h"

#define APIC_HPET_BASE  0XFED00000


#pragma pack(1)


typedef struct {
	unsigned char version;
	unsigned char count : 5;
	unsigned char width : 1;
	unsigned char reserved : 1;
	unsigned char compatable : 1;
	unsigned short venderid;
	DWORD tick;
}HPET_GCAP_ID_REG;

#pragma pack()



void enableLocalAPIC();

void enableIOAPIC();

DWORD * getApicBase();

DWORD * getOicBase();

int enableHpet();

DWORD* getHpetBase();

DWORD* getRcbaBase();

int initHpet();

void enableRcba();

void enableFerr();

void enableIMCR();