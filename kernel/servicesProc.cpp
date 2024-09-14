#include "servicesProc.h"
#include "task.h"
#include "hardware.h"
#include "task.h"
#include "mouse.h"
#include "keyboard.h"
#include "Utils.h"

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
		mov ss,ax
		call __kServicesProc
		add esp, 8

		mov stack.eax, eax		//may be error?  warning: "."Ӧ���ڷ� UDT ����
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
			r = __kGetKbd(0);
			break;
		}
		case KBD_INPUT:
		{
			__kPutKbd((unsigned char)params, 0);
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
			__kDrawMouse();
			break;
		}
		case RESTORE_MOUSE:
		{

			__kRestoreMouse();
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
	int sleeptime = params[0];
	int interval = 1000 / (OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR);
	DWORD times = sleeptime / interval;
	DWORD mod = sleeptime % interval;
	if (mod != 0)
	{
		times++;
	}

	if (times == 0) {
		times = 1;
	}

	LPPROCESS_INFO proc = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	int tid = proc->tid;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO cur_tss = tss + tid;
	
	cur_tss->sleep += times ;
	proc->sleep = cur_tss->sleep;
	while(1)
	{
		__asm {
			hlt
		}

		if (cur_tss->sleep == 0)
		{
			break;
		}
		else {
			continue;
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

//https://www.felixcloutier.com/x86/cpuid
unsigned __int64 __cpuRate() {
	__asm {
		mov eax,0x16
		mov ecx,0
		cpuid
		ret
	}
}

unsigned __int64 __rdtsc() {
	__asm {
		rdtsc
	}
}


DWORD	__cputype(unsigned long * params) {

	__asm{
		mov edi,params
		mov eax, 0
		; must use .586 or above
		; dw 0a20fh
		mov ecx,0
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
		mov		ecx,0	
		cpuid					//dw 0a20fh
		cmp     eax, 80000004h
		jb      __cpuinfoEnd

		mov     eax, 80000002h
		mov		ecx, 0
		cpuid
		mov     dword ptr[edi],     eax
		mov     dword ptr[edi + 4], ebx
		mov     dword ptr[edi + 8], ecx
		mov     dword ptr[edi + 12], edx

		mov     eax, 80000003h
		mov		ecx, 0
		cpuid
		mov     dword ptr[edi + 16], eax
		mov     dword ptr[edi + 20], ebx
		mov     dword ptr[edi + 24], ecx
		mov     dword ptr[edi + 28], edx

		mov     eax, 80000004h
		mov		ecx, 0
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



//MSR ��CPU ��һ��64 λ�Ĵ��������Էֱ�ͨ��RDMSR ��WRMSR ����ָ����ж���д�Ĳ�����ǰ��Ҫ��ECX ��д��MSR �ĵ�ַ��
//MSR ��ָ�����ִ����level 0 ��ʵģʽ�¡�
//RDMSR    ��ģʽ����Ĵ���������RDMSR ָ����᷵����Ӧ��MSR ��64bit ��Ϣ��(EDX��EAX)�Ĵ�����
//WRMSR    дģʽ����Ĵ���������WRMSR ָ���Ҫд�����Ϣ����(EDX��EAX)�У�ִ��дָ��󣬼��ɽ���Ӧ����Ϣ����ECX ָ����MSR ��

//ͨ��DTS��ȡ�¶Ȳ�����ֱ�ӵõ�CPU��ʵ���¶ȣ����������¶ȵĲ
//��һ������Tjmax�����Intel��TCC activation temperature��
//��˼�ǵ�CPU�¶ȴﵽ�򳬹����ֵʱ���ͻᴥ����ص��¶ȿ��Ƶ�·��ϵͳ��ʱ���ȡ��Ҫ�Ķ���������CPU���¶ȣ�����ֱ��������ػ���
//����CPU���¶���Զ���ᳬ�����ֵ�����ֵһ����100���85�棨Ҳ������ֵ�������ھ���Ĵ�������˵����һ���̶���ֵ��
//�ڶ�������DTS��ȡ��CPU�¶����Tjmax��ƫ��ֵ�����ҽ�Toffset����CPU��ʵ���¶Ⱦ��ǣ�currentTemp=Tjmax-Toffset
int __readTemperature(DWORD* tjunction) {
	unsigned int Tjunction = 0;
	DWORD temp = 0;
	__asm {
		mov eax, 0
		cpuid
		cmp eax, 6
		jb _tmpQuit

		mov eax, 6
		cpuid
		test eax, 2
		jz _tmpQuit

		mov ecx, 0x1A2		//eax��16~23λ����Tjmax��ֵ
		rdmsr
		test eax, 0x40000000
		jnz _tmp85
		//mov eax, 100
		mov Tjunction, eax
		jmp _getdts
		_tmp85 :
		//mov eax, 85
		mov Tjunction, eax

			_getdts :
		mov ebx, eax
			and ebx, 0x00ff0000
			shr ebx, 16

			//mov ebx,eax

			mov ecx, 0x19C		//eax��16~22��ע��������7λ��λ����Toffset��ֵ
			rdmsr
			and eax, 0x007f0000
			shr eax, 16
			sub ebx, eax
			mov eax, ebx
			mov temp, eax
			_tmpQuit :
	}

	*tjunction = Tjunction;

	char szout[1024];
	__printf(szout, "tjmax:%x,temprature:%x\r\n", Tjunction, temp);

	return temp;
}