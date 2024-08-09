

#include "apic.h"
#include "hardware.h"
#include "Utils.h"
#include "descriptor.h"

#include "task.h"
#include "cmosAlarm.h"
#include "cmosExactTimer.h"
#include "cmosPeriodTimer.h"
#include "core.h"

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


	//*gApicBase = *gApicBase | 0x0c00;

	*gSvrBase = *gSvrBase | 0x100;
}


void setIoApicID(int id) {
	*(DWORD*)(0xfee00000) = 0;
	*(DWORD*)(0xfee00010) = (id & 0x0f) << 24 ;

}


void setIoRedirect(int id,int idx,int vector,int mode) {
	*(DWORD*)(0xfee00000) = idx ;
	*(DWORD*)(0xfee00010) = ( (id & 0x0f) << 24 ) + vector;

	*(DWORD*)(0xfee00000) = idx + 1;
	*(DWORD*)(0xfee00010) = 0;
}


void enableIOAPIC() {

	*gOicBase = *gOicBase | 0x100;
	*gOicBase = *gOicBase & 0xffffff00;



}

DWORD* getApicBase() {

	DWORD high = 0;
	DWORD low = 0;
	int res = 0;
	readmsr(0x1b,&low,&high);
	low = low | 0x800;
	writemsr(0x1b, low, high);

	gApicBase = (DWORD*)(low & 0xfffff000);
	low = low | 0x800;
	low = low | 0x1000;
	writemsr(0x1b, low, high);
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





int getLVTCount(int n) {
	int cnt = *(long long*)(0xfee00030) >>16;
	return cnt;
}

int getLapicVersion() {
	int ver = *(long long*)(0xfee00030) & 0xff;
	return ver;
}


//cpuid.01h:ebx[31:24]
int getBspID() {
	__asm {
		mov eax,1
		mov ecx,0
		cpuid

		test ebx,0x200000
		jnz _x2apic

		shr ebx,24
		movzx eax,bl

		ret

		_x2apic:
		mov eax,0bh
		mov ecx, 0
		cpuid
		mov eax,edx
		ret

	}
}



int initHpet() {
	int res = 0;
	getHpetBase();
	enableHpet();

	long long id = *(long long*)APIC_HPET_BASE;

	HPET_GCAP_ID_REG * gcap = (HPET_GCAP_ID_REG*)&id;
	if (gcap->tick == 0x0429b17f) {
		int cnt = gcap->count;

		DWORD tick = gcap->tick;

		long long total = 143182;		// 14318179 = 1000ms,0x0429b17f

		*(long long*)(APIC_HPET_BASE + 0x10) = 3;

		*(long long*)(APIC_HPET_BASE + 0x20) = 0;

		*(long long*)(APIC_HPET_BASE + 0xf0) = total;

		long long* regs = (long long*)(APIC_HPET_BASE + 0x100);	

		for (int i = 0; i < cnt; i++) {
			if (i == 0) {
				regs[i] = 0x40 + 8 + 4 + 2 - 2;
			}
			else if (i == 2) {
				regs[i] = 0x1000 + 0x40 + 4 + 2 - 2;
			}
			else if (i % 2 == 0) {
				regs[i] = 0x40 + 2 - 2;
			}
			else {
				regs[i] = total;
			}
			i += 2;
		}

		return TRUE;
	}

	return FALSE;
}



extern "C" void __declspec(naked) HpetInterrupt(LIGHT_ENVIRONMENT * stack) {
	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		char szout[1024];

		int tag = *(int*)(APIC_HPET_BASE + 0x20) ;
		if (tag & 1) {
			__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);
			*(long long*)(APIC_HPET_BASE + 0x108) = 0;
		}
		else if (tag & 2) {
			outportb(0x70, 0x0c);
			int flag = inportb(0x71);
			//IRQF = (PF * PIE) + (AF * AIE) + (UF * UFE), if double interruptions, will not be 1
			if (flag & 0x20) {
				__kAlarmTimerProc();
			}
			else if (flag & 0x40) {
				__kExactTimerProc();
			}
			else if (flag & 0x10) {
				__kPeriodTimer();
			}

			*(long long*)(APIC_HPET_BASE + 0x128) = 0;
		}

		*(long long*)(APIC_HPET_BASE + 0x20) = 0;

		outportb(0x20, 0x20);
		outportb(0xa0, 0xa0);	
	}

	__asm {
#ifdef SINGLE_TASK_TSS
		mov eax, dword ptr ds : [CURRENT_TASK_TSS_BASE + PROCESS_INFO.tss.cr3]
		mov cr3, eax
#endif

		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp
		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad
#ifdef SINGLE_TASK_TSS
		mov esp, dword ptr ss : [esp - 20]
#endif	

		clts
		iretd

		jmp HpetInterrupt
	}
}




extern "C" void __declspec(dllexport) __apInitProc() {
	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;

	DescriptTableReg gdtbase;
	__asm {
		sgdt gdtbase
	}

	gdtbase.addr = GDT_BASE;
	__asm{
		lgdt gdtbase

		; mov ax, kTssTaskSelector
		; ltr ax

		mov ax, ldtSelector
		lldt ax

		lidt idtbase
	}

	char szout[1024];
	__printf(szout, "idt base:%x,size:%x\r\n", idtbase.addr, idtbase.size);

	enablePage();

	__asm {
		hlt
	}
}

/*
短跳转（Short Jmp，只能跳转到256字节的范围内），对应机器码：EB
近跳转（Near Jmp，可跳至同一段范围内的地址），对应机器码：E9
近跳转（Near call，可跳至同一段范围内的地址），对应机器码：E8
远跳转（Far Jmp，可跳至任意地址），对应机器码： EA
远跳转（Far call，可跳至任意地址），对应机器码： 9A
ff 15 call
ff 25 call
*/




