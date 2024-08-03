

#include "apic.h"
#include "hardware.h"
#include "Utils.h"
#include "descriptor.h"




DWORD * gApicBase = 0;
DWORD * gSvrBase = 0;
DWORD * gOicBase = 0;
DWORD * gHpetBase = 0;
DWORD* gRcbaBase = 0;



void enableRcba() {
	*gRcbaBase = *gRcbaBase | 1;
}

void enableFerr() {

	*gOicBase = *gOicBase | 0x200;
}



void enableLocalAPIC() {

	*gApicBase = *gApicBase | 0x0c00;

	*gSvrBase = *gSvrBase | 0x100;
}




void enableIOAPIC() {

	*gOicBase = *gOicBase | 0x100;
	*gOicBase = *gOicBase & 0xffffff00;

	*(DWORD*)0xfec00000 = 0;
	*(DWORD*)0xfec00010 = 0x0f000000;

}

DWORD* getApicBase() {

	DWORD high = 0;
	DWORD low = 0;
	int res = 0;
	readmsr(0x1b,&low,&high);
	gApicBase = (DWORD*)(low & 0xfffff000);
	gSvrBase = (DWORD*)((DWORD)gApicBase + 0xf0);
	return gApicBase;
}

//fec00000
//fed00000
//fee00000
DWORD* getOicBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD rcba = inportd(0xcfc) & 0xffffc000;

	gOicBase = (DWORD*)(rcba + 0x31fe);

	return gOicBase;
}

int enableHpet() {
	*gHpetBase = *gHpetBase | 0x80;
	*gHpetBase = *gHpetBase & 0xfffffffc;
	return 0;
}


DWORD * getHpetBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD addr = inportd(0xcfc) & 0xffffc000;

	gHpetBase = (DWORD*)(addr + 0x3404);

	return gHpetBase;
}


DWORD* getRcbaBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD base = inportd(0xcfc) & 0xffffc000;

	gRcbaBase = (DWORD*)base;

	return gRcbaBase;
}


void enableIMCR() {
	outportb(0x22, 0x70);
	outportb(0x23, 0x01);
}



void initHpet() {
	int res = 0;
	getHpetBase();
	enableHpet();

	long long id = *(long long*)APIC_HPET_BASE;

	HPET_GCAP_ID_REG * gcap = (HPET_GCAP_ID_REG*)&id;

	int cnt = gcap->count;
	*(long long*)(APIC_HPET_BASE + 0x10) = 3;

	*(long long*)(APIC_HPET_BASE + 0x20) = 0;

	*(long long*)(APIC_HPET_BASE + 0xf0) = 0;

	long long* regs = (long long*)(APIC_HPET_BASE + 0x100);

	DWORD tick = gcap->tick;

	long long total = 143182;		// 1431818 = 100ms,0x0429b17f

	for (int i = 0; i < cnt; i++) {
		if (i == 0) {
			regs[i] = 0x40 + 8 + 4 + 2;
		}
		else if (i == 2) {
			regs[i] =0x1000 + 0x40 + 4 + 2;
		}
		else if(i % 2 == 0){
			regs[i] = 0x40 + 2;
		}
		else {
			regs[i] = total;
		}
	}
}