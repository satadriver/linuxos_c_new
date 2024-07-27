#include "servicesProc.h"
#include "task.h"
#include "hardware.h"
#include "task.h"
#include "mouse.h"


DWORD __declspec(naked) servicesProc(LIGHT_ENVIRONMENT* stack) {

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
	}

	__asm {
		push edi
		push eax

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		call __kServicesProc
		add esp, 8

		mov stack.eax, eax		//may be error?  warning: "."应用于非 UDT 类型
	}

	__asm {
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

		iretd
	}
}

DWORD __declspec(dllexport) __kServicesProc(DWORD num, DWORD * params) {

	DWORD r = 0;
	switch (num)
	{
		case KBD_OUTPUT:
		{
			break;
		}
		case KBD_INPUT:
		{
			break;
		}
		case MOUSE_OUTPUT:
		{
			r= __kGetMouse((LPMOUSEINFO)params,0);
			break;
		}
		case GRAPH_CHAR_OUTPUT:
		{
			break;
		}
		case RANDOM:
		{
			r = __random((unsigned long)params);
			break;
		}
		case SLEEP:
		{
			sleep(params);

			break;
		}
		case TURNON_SCREEN:
		{
			__turnonScreen();
			break;
		}
		case TURNOFF_SCREEN:
		{
			__turnoffScreen();
			break;
		}
		case CPU_MANUFACTORY:
		{
			r = __cputype(params);
			break;
		}
		case TIMESTAMP:
		{
			r = __timestamp(params);
			break;
		}
		case SWITCH_SCREEN:
		{
			__switchScreen();
			break;
		}
		case CPUINFO:
		{
			r = __cpuinfo(params);
			break;
		}
		case DRAW_MOUSE:
		{
			break;
		}
		case RESTORE_MOUSE:
		{
			break;
		}
		case SET_VIDEOMODE:
		{
			break;
		}

		default: {
			break;
		}
	}
	return r;
}


void sleep(DWORD * params) {
	int interval = 1000 / (OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR);
	DWORD times = params[0] / interval;
	DWORD mod = params[0] % interval;
	if (mod != 0)
	{
		times++;
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	tss->sleep += times;

	for (int i = 0; i < times; i++)
	{
		__asm {
			hlt
		}

		if (tss->sleep)
		{
			tss->sleep--;
		}
	}
}



DWORD __random(DWORD init) {
	if (init == 0) {
		init = *(DWORD*)TIMER0_TICK_COUNT;
	}
	init = (init * 7 ^ 5) % 0xffffffff;
	return init;
}



void __turnoffScreen() {

	outportb(0x3c4, 1);
	int r = inportb(0x3c5);
	if ( (r & 0x20) == 0) {
		outportb(0x3c5, r | 0x20);
	}
}


void __turnonScreen() {

	outportb(0x3c4, 1);
	int r = inportb(0x3c5);
	if (r & 0x20 ) {
		outportb(0x3c5, 0);
	}
}


void __switchScreen() {
	outportb(0x3c4, 1);
	int r = inportb(0x3c5);
	if (r & 0x20) {
		outportb(0x3c5, 0);
	}
	else {
		outportb(0x3c5, 0x20);
	}
}



DWORD	__cputype(unsigned long * params) {

	__asm{
		mov edi,params
		mov eax, 0
		; must use .586 or above
		; dw 0a20fh
		cpuid
		; ebx:edx:ecx = intel or else
		mov ds : [edi] , ebx
		mov ds : [edi + 4] , edx
		mov ds : [edi + 8] , ecx
		mov dword ptr ds : [edi + 12] , 0
	}
}





DWORD __cpuinfo(unsigned long* params) {
	__asm {
		mov edi,params

		mov     eax, 80000000h
		; dw 0a20fh
		cpuid
		cmp     eax, 80000004h
		jb      __cpuinfoEnd

		mov     eax, 80000002h
		; dw 0a20fh
		cpuid
		mov     dword ptr[edi], eax
		mov     dword ptr[edi + 4], ebx
		mov     dword ptr[edi + 8], ecx
		mov     dword ptr[edi + 12], edx

		mov     eax, 80000003h
		; dw 0a20fh
		cpuid
		mov     dword ptr[edi + 16], eax
		mov     dword ptr[edi + 20], ebx
		mov     dword ptr[edi + 24], ecx
		mov     dword ptr[edi + 28], edx

		mov     eax, 80000004h
		; dw 0a20fh
		cpuid
		mov     dword ptr[edi + 32], eax
		mov     dword ptr[edi + 36], ebx
		mov     dword ptr[edi + 40], ecx
		mov     dword ptr[edi + 44], edx

		mov     dword ptr[edi + 48], 0

		__cpuinfoEnd:
	}
}






DWORD __timestamp(unsigned long* params) {

	__asm {
		mov edi,params
		; must use .586 or above
		rdtsc
		; edx:eax = time stamp
		mov ds : [edi] , eax
		mov ds : [edi + 4] , edx
		mov dword ptr ds : [edi + 8] , 0
	}
}