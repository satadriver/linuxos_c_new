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
		return FALSE;
	}

	DWORD csseg = KERNEL_MODE_CODE;

	DWORD high = 0;

	writemsr(0x174, csseg, high);

	DWORD esp0 = SYSCALL_STACK0_TOP;

	writemsr(0x175, esp0, high);

	DWORD eip = (DWORD)entryaddr;

	writemsr(0x176, eip, high);

	return TRUE;
}



int sysEntry(DWORD  params, DWORD size) {
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

	return 0;
}

//only be invoked in ring3,in ring0 will cause exception 0dh
extern "C" __declspec(dllexport) int sysEntryProc(DWORD  params, DWORD size) {

	int res = 0;
	if (g_sysEntryInit == 0) {
		DWORD addr = 0;
		__asm {
			lea eax, __sysEntry
			mov addr, eax
		}
		res = sysEntryInit((DWORD)addr);
		if (res ) {
			g_sysEntryInit = TRUE;
		}
		else {
			return FALSE;
		}
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
	res = sysEntry(params, size);

	__asm {
		lea edx, __sysEntryExit
		mov ecx, ds : [g_sysEntryStack3]
		_emit 0x0f
		_emit 0x35

		__sysEntryExit :
	}	

	return res;

}

