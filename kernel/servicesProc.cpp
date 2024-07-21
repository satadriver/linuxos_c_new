#include "servicesProc.h"
#include "task.h"



DWORD __declspec(naked) servicesProc() {
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

		push edi
		push eax

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX

		call __kServicesProc
		add esp,8
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

DWORD __declspec(dllexport) __kServicesProc(DWORD no, DWORD * params) {


		switch (no)
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
			break;
		}
		case GRAPH_CHAR_OUTPUT:
		{
			break;
		}
		case RANDOM:
		{
			break;
		}
		case SLEEP:
		{
			DWORD times = params[0] / 10;
			DWORD mod = params[0] % 10;
			if (mod == 0 && times == 0)
			{
				times++;
			}
			else if (mod == 0 && times != 0)
			{

			}
			else if (mod != 0 && times == 0)
			{
				times++;
			}
			else if (mod != 0 && times != 0)
			{
				times++;
			}

			if (times > 0)
			{
				LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
				tss->sleep = times - 1;

			}
			__asm {
				hlt;
			}

			// 		for (int i = 0; i < times; i++)
			// 		{
			// 			__asm {
			// 				hlt
			// 			}
			// 		}

			break;
		}
		case TURNON_SCREEN:
		{
			break;
		}
		case TURNOFF_SCREEN:
		{
			break;
		}
		case CPU_MANUFACTORY:
		{
			break;
		}
		case TIMESTAMP:
		{
			break;
		}
		case SWITCH_SCREEN:
		{
			break;
		}
		case CPUINFO:
		{
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


}



DWORD __getRandom() {
	__asm {
		mov eax, 0
		mov al, 0
		out 43h, al
		in al, 40h
		shl eax, 8
		in al, 40h
		shl eax, 8

		mov al, 0
		out 43h, al
		in al, 40h
		shl eax, 8
		in al, 40h
		ret
	}
}



void __turnoffScreen() {
	__asm {
		push edx
		mov dx, 3c4h
		mov al, 1
		out dx, al
		mov dx, 3c5h
		in al, dx
		test al, 20h
		jnz __turnoffScreenEnd
		or al, 20h
		out dx, al
		__turnoffScreenEnd :
		pop edx
			ret
	}
}


void __turnonScreen() {
	__asm {
		push edx
		mov dx, 3c4h
		mov al, 1
		out dx, al
		mov dx, 3c5h
		in al, dx
		test al, 20h
		jz __turnonScreenEnd
		and al, 0
		out dx, al
		__turnonScreenEnd :
		pop edx
			ret
	}
}


void __switchScreen() {
	__asm {
		push edx
		mov dx, 3c4h
		mov al, 1
		out dx, al
		mov dx, 3c5h
		in al, dx
		test al, 20h
		jz _shutdownscreen
		mov al, 0
		out dx, al
		pop edx
		ret

		_shutdownscreen :
		mov al, 20h
			out dx, al
			pop edx
			ret
	}
}



DWORD	_cpumanu() {
	__asm{
		mov eax, 0
		; must use .586 or above
		; dw 0a20fh
		cpuid
		; ebx:edx:ecx = intel or else
		mov ds : [edi] , ebx
		mov ds : [edi + 4] , edx
		mov ds : [edi + 8] , ecx
		mov dword ptr ds : [edi + 12] , 0
		ret
	}
}





DWORD __cpuinfo() {
	__asm {
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
		ret

	}
}






DWORD _cputimestamp() {
	__asm {
		; must use .586 or above
		rdtsc
		; edx:eax = time stamp
		mov ds : [edi] , eax
		mov ds : [edi + 4] , edx
		mov dword ptr ds : [edi + 8] , 0
		ret
	}
}