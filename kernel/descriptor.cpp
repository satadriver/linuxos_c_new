#include "descriptor.h"
#include "def.h"
#include "process.h"
#include "Utils.h"
#include "Pe.h"
#include "malloc.h"
#include "page.h"
#include "video.h"
#include "Utils.h"
#include "Kernel.h"


void getGdtIdt() {
	DESCRIPTOR_REG gdt;
	DESCRIPTOR_REG idt;
	__asm {
		lea eax, gdt
		sgdt[eax]

		lea eax, idt
		sidt[eax]
	}

	glpGdt = (LPSEGDESCRIPTOR)gdt.addr;

	glpIdt = (LPSYSDESCRIPTOR)idt.addr;

	int gdtcnt = (gdt.size + 1) >> 3;
	for (int i = 1; i < gdtcnt; i++)
	{
		if (glpGdt[i].attr == 0xe2 || glpGdt[i].attr == 0x82)
		{
			glpLdt = &glpGdt[i];
			initLdt(glpLdt);
			break;
		}
		else if (glpGdt[i].attr == 0xec || glpGdt[i].attr == 0x8c)
		{
			glpCallGate = (LPSYSDESCRIPTOR)&glpGdt[i];
			initCallGate((LPSYSDESCRIPTOR)&glpGdt[i]);
			break;
		}
	}
}

//Extended Feature Enable Register(EFER) is a model - specific register added in the AMD K6 processor, 
//to allow enabling the SYSCALL / SYSRET instruction, and later for entering and exiting long mode.
//This register becomes architectural in AMD64 and has been adopted by Intel.Its MSR number is 0xC0000080.
void initEfer() {
	DWORD highpart, lowpart;
	readmsr(0xC0000080, &lowpart, &highpart);

	//readmsr(0x1f80, &highpart, &lowpart);

	__asm {
		mov eax, [lowpart]
		or eax, 0x4001
		mov[lowpart], eax
	}

	//writemsr(0xC0000080, lowpart, highpart);
}


void initCallGate(LPSYSDESCRIPTOR lpcg) {
	lpcg->attr = 0xec;
	lpcg->paramCnt = 2;
	lpcg->selector = KERNEL_MODE_CODE;

	lpcg->addrHigh = (DWORD)__kCallGateProc >> 16;
	lpcg->addrLow = (DWORD)__kCallGateProc & 0xffff;
}


void initLdt(LPSEGDESCRIPTOR lpldt) {
	//return;

	lpldt->attr = 0xe2;
	lpldt->baseHigh = (unsigned char)(LDT_BASE >> 24);
	lpldt->baseLow = (unsigned short)(LDT_BASE & 0xffff);
	lpldt->baseMid = (unsigned char)(LDT_BASE >> 16);
	lpldt->gd0a_lh = 0;
	lpldt->limitLow = 0x27;

	LPSEGDESCRIPTOR selectors = (LPSEGDESCRIPTOR)LDT_BASE;

	selectors[0].baseHigh = 0;
	selectors[0].baseMid = 0;
	selectors[0].baseLow = 0;
	selectors[0].attr = 0;
	selectors[0].gd0a_lh = 0;
	selectors[0].limitLow = 0;

	selectors[1].baseHigh = 0;
	selectors[1].baseMid = 0;
	selectors[1].baseLow = 0;
	selectors[1].attr = 0x9a;
	selectors[1].gd0a_lh = 0xcf;
	selectors[1].limitLow = 0xffff;

	selectors[2].baseHigh = 0;
	selectors[2].baseMid = 0;
	selectors[2].baseLow = 0;
	selectors[2].attr = 0x92;
	selectors[2].gd0a_lh = 0xcf;
	selectors[2].limitLow = 0xffff;

	selectors[3].baseHigh = 0;
	selectors[3].baseMid = 0;
	selectors[3].baseLow = 0;
	selectors[3].attr = 0xfa;
	selectors[3].gd0a_lh = 0xcf;
	selectors[3].limitLow = 0xffff;

	selectors[4].baseHigh = 0;
	selectors[4].baseMid = 0;
	selectors[4].baseLow = 0;
	selectors[4].attr = 0xf2;
	selectors[4].gd0a_lh = 0xcf;
	selectors[4].limitLow = 0xffff;

	unsigned short ldtno = (unsigned short)((DWORD)lpldt - (DWORD)glpGdt);
	__asm {
		movzx eax, ldtno
		lldt ax
	}
}


//长调用最终调用在哪里.是由调用门(段描述符)来指定的.而不是EIP.EIP是废弃的
extern "C" __declspec(naked) void __kCallGateProc(DWORD  params, DWORD count) {

	__asm {
		mov ebp, esp
	}

	{
		char szout[1024];
		__printf(szout, "__kCallGateProc running,param1:%x,param2:%x\r\n", params, count);
	}

	__asm {
		mov esp, ebp

		retf 0x08		//ca 08 00		在长调用中使用retf，这点需要注意.
	}

	//RET immed16:	C2 
	//RET :		C3 
	//RETF immed16: //CA 
	//RETF :	CB 
	//IRET :	CF 
	//IRET [bits 16]:	CF 
	//IRETD :	66 CF

	//机器码对应表：
	//https://defuse.ca/online-x86-assembler.htm#disassembly
}



extern "C" __declspec(dllexport) void callgateEntry(DWORD  params,DWORD count) {

	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		cli

		push dword ptr count
		push params

		_emit 0x9a

		_emit 0
		_emit 0
		_emit 0
		_emit 0

		_emit callGateSelector
		_emit 0

		add esp,8

		sti

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad
	}

	char szout[1024];
	__printf(szout, "callgateEntry leave\r\n");

#if 0
 	CALL_LONG calllong;
 	calllong.callcode = 0x9a;
 	calllong.seg = seg;
 	calllong.offset = (DWORD)__kCallGateProc;
 	__asm {
 		lea eax, calllong
 		jmp eax
 	}
#endif
}











void readmsr(DWORD no, DWORD *lowpart, DWORD * highpart) {
	__asm {
		xor eax, eax
		xor edx, edx

		mov ecx, no
		rdmsr

		mov ecx, lowpart
		mov[ecx], eax

		mov ecx, highpart
		mov[ecx], edx
	}

	char szout[1024];
	__printf(szout, "msr:%x,high:%x,low:%x\r\n", no, *highpart, *lowpart);

}

void writemsr(DWORD reg, DWORD lowpart, DWORD highpart) {
	__asm {
		mov ecx, reg

		mov eax, lowpart

		mov edx, highpart

		wrmsr
	}
}

void syscall() {

}

void sysleave() {

}



DWORD g_sysEntryInit = 0;

DWORD g_sysEntryStack3 = 0;



int sysEntryInit(DWORD entryaddr) {
	LPTSS tss = (LPTSS)CURRENT_TASK_TSS_BASE;
	if (tss->cs & 3)
	{
		return -1;
	}

	DWORD csseg = KERNEL_MODE_CODE;

	DWORD high = 0;

	writemsr(0x174, csseg, high);

	DWORD esp0 = SYSCALL_STACK0_TOP;

	writemsr(0x175, esp0, high);

	DWORD eip = (DWORD)entryaddr;

	writemsr(0x176, eip, high);

	return 0;
}



void sysEntry(DWORD  params, DWORD size) {
	LPTSS tss = (LPTSS)CURRENT_TASK_TSS_BASE;

	WORD rcs = 0;
	DWORD resp = 0;
	WORD rss = 0;
	__asm {
		mov ax, cs
		mov rcs, ax

		mov ax, ss
		mov rss, ax

		mov resp, esp
	}
	char szout[1024];
	__printf(szout, "sysEntry current cs:%x,tss cs:%x,ss:%x,esp:%x\r\n", rcs, tss->cs, rss, resp);

}

//only be invoked in ring3,in ring0 will cause exception 0dh
extern "C" __declspec(dllexport) void sysEntryProc(DWORD  params, DWORD size) {

	if (g_sysEntryInit == 0) {
		DWORD addr = 0;
		__asm {
			lea eax, __sysEntry
			mov addr, eax
		}
		sysEntryInit((DWORD)addr);
		g_sysEntryInit = TRUE;
	}

	__asm {
		mov ax, cs
		test ax, 3
		jz __sysEntryExit

		mov ds : [g_sysEntryStack3] , esp

		_emit 0x0f
		_emit 0x34
	}

	__sysEntry:
	sysEntry(params, size);

	__asm {
		lea edx, __sysEntryExit
		mov ecx, ds : [g_sysEntryStack3]
		_emit 0x0f
		_emit 0x35

		__sysEntryExit :
	}	


}

