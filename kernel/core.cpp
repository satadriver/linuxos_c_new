#include "core.h"
#include "def.h"
#include "process.h"
#include "Utils.h"
#include "Pe.h"
#include "malloc.h"
#include "page.h"
#include "video.h"
#include "Utils.h"
#include "Kernel.h"
#include "exception.h"
#include "debugger.h"
#include "keyboard.h"

#include "mouse.h"
#include "v86.h"
#include "servicesProc.h"
#include "task.h"


//TSS32 gInvalidTss;

//TSS32 gTimerTss;

//TSS32 gV86Tss;

DWORD g_sysenterInitFlag = 0;

DWORD* g_sysenterParams = 0;
DWORD g_sysenterParamsLen = 0;

DWORD g_sysenterStack3 = 0;
DWORD g_sysenterEip3 = 0;




void initGdt() {

	DescriptTableReg gdtbase;
	DescriptTableReg* lpgdt = &gdtbase;
	__asm {

		sgdt gdtbase;
	}

	char szout[1024];
	__printf(szout, "gdt base:%x,size:%x\r\n", lpgdt->addr, lpgdt->size);

	__memset((char*)GDT_BASE, 0, sizeof(SegDescriptor) * 8192);
	__memcpy((char*)GDT_BASE, (char*)lpgdt->addr, lpgdt->size + 1);
	lpgdt->addr = GDT_BASE;

	//GDT_BASE = gdtbase.addr;

	SegDescriptor* gdt = (SegDescriptor*)GDT_BASE;
	makeCodeSegDescriptor(0, 0, 32, 0, 1, gdt + 1);
	makeDataSegDescriptor(0, 0, 32, 0, 1, gdt + 2);
	makeCodeSegDescriptor(0, 3, 32, 0, 1, gdt + 3);
	makeDataSegDescriptor(0, 3, 32, 0, 1, gdt + 4);

	makeCallGateDescriptor((DWORD)__kCallGateProc, KERNEL_MODE_CODE, 3, 2, (CallGateDescriptor*)(GDT_BASE + callGateSelector));

	makeLDTDescriptor(LDT_BASE, 3, 0x27, (TssDescriptor*)(GDT_BASE + ldtSelector));

	__memset((char*)LDT_BASE, 0, sizeof(SegDescriptor) * 8192);
	SegDescriptor* ldt = (SegDescriptor*)LDT_BASE;
	//ldt sequence number start form 1 not like gdt starting from 0
	makeCodeSegDescriptor(0, 0, 32, 0, 1, ldt + 0);
	makeDataSegDescriptor(0, 0, 32, 0, 1, ldt + 1);
	makeCodeSegDescriptor(0, 3, 32, 0, 1, ldt + 2);
	makeDataSegDescriptor(0, 3, 32, 0, 1, ldt + 3);

	initKernelTss((TSS*)CURRENT_TASK_TSS_BASE, TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,
		KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor(CURRENT_TASK_TSS_BASE, 3, 0, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTaskSelector));

}





__declspec(naked) void __kCallGateProc() {
	DWORD* params;
	DWORD paramLen;

	//ebp
	//eip3
	//cs3
	//param1
	//param2
	//esp3
	//ss3

	//思考编译器对局部变量的定义过程
	__asm {
		pushad
		pushfd
		mov ebp, esp
		sub esp, 0x1000

		mov eax, [ebp + 36 + 8]
		mov params, eax
		mov eax, [ebp + 36 + 12]
		mov paramLen, eax
	}

	// 	char szout[1024];
	// 	__printf(szout, "__kCallGateProc running,param1:%x,param2:%x\r\n", params,paramLen);
	// 	__drawGraphChars((unsigned char*)szout, 0xff0000);

		//[bits 16] iret ;编译后的机器码为CF 
		//iretd ;编译后的机器码为66 CF
		//C2 RET immed16
		//C3 RET
		//cb retf
		//cf iret
	__asm {
		mov esp, ebp
		popfd
		popad

		retf 0x08		//ca 08 00
	}
	//机器码对应表：
	//https://defuse.ca/online-x86-assembler.htm#disassembly
}




extern "C" __declspec(dllexport) void callgateEntry(DWORD * params, DWORD paramLen) {

	//__drawGraphChars((unsigned char*)"callgateEntry entry\r\n", 0);

	__asm {

		push paramLen
		push params

		_emit 0x9a
		_emit 0
		_emit 0
		_emit 0
		_emit 0
		_emit callGateSelector
		_emit 0

		add esp, 8
	}

	//__drawGraphChars((unsigned char*)"callgateEntry leave\r\n", 0);

// 	CALL_LONG calllong;
// 	calllong.callcode = 0x9a;
// 	calllong.seg = seg;
// 	calllong.offset = (DWORD)__kCallGateProc;
// 	__asm {
// 		lea eax, calllong
// 		jmp eax
// 	}
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




void readmsr(DWORD no, DWORD* lowpart, DWORD* highpart) {
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

void writemsr(DWORD no, DWORD lowpart, DWORD highpart) {
	__asm {
		mov ecx, no

		mov eax, lowpart

		mov edx, highpart

		wrmsr
	}
}

void syscall() {

}

void sysleave() {

}

int sysenterInit(DWORD entryaddr) {
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









void sysenterProc(DWORD* params, DWORD paramslen) {
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
	__printf(szout, "sysenterProc current cs:%x,tss cs:%x,ss:%x,esp:%x\r\n", rcs, tss->cs, rss, resp);

}



//only be invoked in ring3,in ring0 will cause exception 0dh
extern "C" __declspec(dllexport) void sysenterEntry(DWORD * params, DWORD paramslen) {

	//__drawGraphChars((unsigned char*)"sysenterProc entry\r\n", 0);

	g_sysenterParams = params;
	g_sysenterParamsLen = paramslen;

	__asm {

		cmp dword ptr ds : [g_sysenterInitFlag] , 0
		jnz __sysenterInitFlagOK

		lea eax, __sysenterEntry
		push eax
		call sysenterInit
		add esp, 4
		cmp eax, 0
		jnz __sysenterExit
		mov dword ptr ds : [g_sysenterInitFlag] , 1
		jmp __sysenterExit

		__sysenterInitFlagOK :

		mov ax, cs
			test ax, 3
			jz __sysenterExit

			mov ds : [g_sysenterStack3] , esp
			lea eax, __sysenterExit
			mov ds : [g_sysenterEip3] , eax


			_emit 0x0f
			_emit 0x34

			__sysenterEntry :

			mov eax, ds : [g_sysenterParamsLen]
			push eax
			mov eax, ds : [g_sysenterParams]
			push eax
			call sysenterProc
			add esp, 8

			sti

			mov edx, ds: [g_sysenterEip3]
			mov ecx, ds : [g_sysenterStack3]
			_emit 0x0f
			_emit 0x35

			__sysenterExit :
	}

	//__drawGraphChars((unsigned char*)"sysenterProc leave\r\n", 0);
}




void makeDataSegDescriptor(DWORD base, int dpl, int bit, int direction, int w, SegDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->system = 1;
	descriptor->code = 0;
	descriptor->r_w = w;
	descriptor->access = 0;
	if (bit == 16) {
		descriptor->granularity = 0;
		descriptor->db = 0;
	}
	else {
		descriptor->granularity = 1;
		descriptor->db = 1;
	}

	if (direction) {
		descriptor->ext_conform = 1;
	}
	else {
		descriptor->ext_conform = 0;
	}

	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->dpl = dpl;
	descriptor->len = 0xffff;
	descriptor->lenHigh = 0xf;
}

void makeCodeSegDescriptor(DWORD base, int dpl, int bit, int conforming, int r, SegDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->system = 1;
	descriptor->code = 1;
	descriptor->r_w = r;
	descriptor->access = 0;
	if (bit == 16) {
		descriptor->granularity = 0;
		descriptor->db = 0;
	}
	else {
		descriptor->granularity = 1;
		descriptor->db = 1;
	}

	if (conforming) {
		descriptor->ext_conform = 1;
	}
	else {
		descriptor->ext_conform = 0;
	}
	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->dpl = dpl;
	descriptor->len = 0xffff;
	descriptor->lenHigh = 0xf;
}

void makeTssDescriptor(DWORD base, int dpl, int busy, int size, TssDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->dpl = dpl;
	descriptor->system = 0;
	descriptor->type = TSS_DESCRIPTOR;

	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->db = 0;
	descriptor->granularity = 0;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->len = size & 0xffff;
	descriptor->lenHigh = (size >> 16) & 0xf;
}


void makeLDTDescriptor(DWORD base, int dpl, int size, TssDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->dpl = dpl;
	descriptor->system = 0;
	descriptor->type = LDT_DESCRIPTOR;

	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->db = 0;
	descriptor->granularity = 0;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->len = size & 0xffff;
	descriptor->lenHigh = (size >> 16) & 0xf;
}

void makeTaskGateDescriptor(DWORD selector, int dpl, TaskGateDescriptor* descriptor) {

	descriptor->type = TASKGATE_DESCRIPTOR;
	descriptor->system = 0;
	descriptor->dp1 = dpl;
	descriptor->present = 1;
	descriptor->selector = selector;
	descriptor->unused1 = 0;
	descriptor->unused2 = 0;
	descriptor->unused3 = 0;
}



void makeCallGateDescriptor(DWORD base, DWORD selector, int dpl, int paramcnt, CallGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->paramCnt = paramcnt;
	descriptor->system = 0;
	descriptor->type = CALLGATE_DESCRIPTOR;
	descriptor->dpl = dpl;
	descriptor->selector = selector;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseHigh = (base >> 16) & 0xffff;
}

void makeIntGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->system = 0;
	descriptor->type = INTGATE_DESCRIPTOR;
	descriptor->dpl = dpl;
	descriptor->unused = 0;
	descriptor->selector = selector;
	descriptor->baseLow = base & 0xffff;

	descriptor->baseHigh = (base >> 16) & 0xffff;
}

void makeTrapGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->system = 0;
	descriptor->type = TRAPGATE_DESCRIPTOR;
	descriptor->unused = 0;
	descriptor->dpl = dpl;
	descriptor->selector = selector;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseHigh = (base >> 16) & 0xffff;
}







void initKernelTss(TSS* tss, DWORD esp0, DWORD reg_esp, DWORD eip, DWORD cr3, DWORD ldt) {

	__memset((char*)tss, 0, sizeof(TSS));

	tss->iomapEnd = 0xff;
	tss->iomapOffset = OFFSETOF(TSS, iomapOffset) + SIZEOFMEMBER(TSS, intMap);

	__asm {
		//pushfd
		//pop tss.eflags
	}

	tss->ds = KERNEL_MODE_DATA;
	tss->es = KERNEL_MODE_DATA;
	tss->fs = KERNEL_MODE_DATA;
	tss->gs = KERNEL_MODE_DATA;

	tss->ss = KERNEL_MODE_DATA;
	tss->esp = reg_esp;

	tss->esp0 = esp0;
	tss->ss0 = KERNEL_MODE_STACK;

	tss->eip = eip;
	tss->cs = KERNEL_MODE_CODE;

	tss->ldt = ldt;
	tss->cr3 = cr3;
}

void enableGDT() {
	DescriptTableReg gdtbase;

	__asm {

		sgdt gdtbase;
	}

	gdtbase.addr = GDT_BASE;
	__asm {
		//do not use lgdt lpgdt,why?
		lgdt gdtbase

		mov ax, kTssTaskSelector
		ltr ax

		mov ax, ldtSelector
		lldt ax

	}
}


void enableIDT() {
	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;
	char szout[1024];
	__printf(szout, "idt base:%x,size:%x\r\n", idtbase.addr, idtbase.size);
	__asm {
		//不要使用 lidt lpidt,why?
		lidt idtbase
	}
}


void initIDT() {

#ifdef _DEBUG
	SegDescriptor* gdt = (SegDescriptor*)new char[0x10000];
	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)new char[0x10000];
#else

	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)IDT_BASE;
#endif

	for (int i = 0; i < 256; i++)
	{
		makeTrapGateDescriptor((DWORD)anonymousException, KERNEL_MODE_CODE, 3, descriptor + i);
	}

	makeTrapGateDescriptor((DWORD)div0Exception, KERNEL_MODE_CODE, 3, descriptor + 0);
	makeTrapGateDescriptor((DWORD)__kDebugger, KERNEL_MODE_CODE, 3, descriptor + 1);

	makeTrapGateDescriptor((DWORD)NmiException, KERNEL_MODE_CODE, 3, descriptor + 2);

	makeTrapGateDescriptor((DWORD)__kBreakPoint, KERNEL_MODE_CODE, 3, descriptor + 3);

	makeTrapGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + 4);
	makeTrapGateDescriptor((DWORD)boundCheckException, KERNEL_MODE_CODE, 3, descriptor + 5);
	makeTrapGateDescriptor((DWORD)illegalOperandException, KERNEL_MODE_CODE, 3, descriptor + 6);
	makeTrapGateDescriptor((DWORD)deviceUnavailableException, KERNEL_MODE_CODE, 3, descriptor + 7);
	makeTrapGateDescriptor((DWORD)doubleFaultException, KERNEL_MODE_CODE, 3, descriptor + 8);

	makeTrapGateDescriptor((DWORD)coprocCrossBorderException, KERNEL_MODE_CODE, 3, (descriptor + 9));

	initKernelTss((TSS*)TSSEXCEPTION_TSS_BASE, TSSEXP_STACK0_TOP, TSSEXP_STACK_TOP, (DWORD)invalidTssException, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)TSSEXCEPTION_TSS_BASE, 3, 0, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssExceptSelector));
	makeTaskGateDescriptor((DWORD)kTssExceptSelector, 3, (TaskGateDescriptor*)(descriptor + 10));

	makeTrapGateDescriptor((DWORD)segmentInexistException, KERNEL_MODE_CODE, 3, descriptor + 11);
	makeTrapGateDescriptor((DWORD)stackException, KERNEL_MODE_CODE, 3, descriptor + 12);
	makeTrapGateDescriptor((DWORD)generalProtectException, KERNEL_MODE_CODE, 3, descriptor + 13);
	makeTrapGateDescriptor((DWORD)pageException, KERNEL_MODE_CODE, 3, descriptor + 14);
	makeTrapGateDescriptor((DWORD)anonymousException, KERNEL_MODE_CODE, 3, descriptor + 15);
	makeTrapGateDescriptor((DWORD)coprocessorException, KERNEL_MODE_CODE, 3, descriptor + 16);
	makeTrapGateDescriptor((DWORD)alighCheckException, KERNEL_MODE_CODE, 3, descriptor + 17);
	makeTrapGateDescriptor((DWORD)machineCheckException, KERNEL_MODE_CODE, 3, descriptor + 18);
	makeTrapGateDescriptor((DWORD)simdException, KERNEL_MODE_CODE, 3, descriptor + 19);
	makeTrapGateDescriptor((DWORD)virtualException, KERNEL_MODE_CODE, 3, descriptor + 20);

#ifdef TASK_SINGLE_TSS
	makeIntGateDescriptor((DWORD)TimerInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 0);
#else
	initKernelTss((TSS*)TIMER_TSS_BASE, TSSTIMER_STACK0_TOP, TSSTIMER_STACK_TOP, (DWORD)TimerInterrupt, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)TIMER_TSS_BASE, 3, 0, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTimerSelector));
	makeTaskGateDescriptor((DWORD)kTssTimerSelector, 3, (TaskGateDescriptor*)(descriptor + INTR_8259_MASTER + 0));
#endif

	makeIntGateDescriptor((DWORD)__kKeyboardProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 1);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 2);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 3);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 4);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 5);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 6);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 7);
	makeIntGateDescriptor((DWORD)CmosInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 0);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 1);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 2);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 3);
	makeIntGateDescriptor((DWORD)__kMouseProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 4);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 5);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 6);
	makeIntGateDescriptor((DWORD)overflowException, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 7);

	makeTrapGateDescriptor((DWORD)__kServicesProc, KERNEL_MODE_CODE, 3, descriptor + 0x80);

	//initTss(&gV86Tss, TSSV86_STACK0_TOP, (DWORD)v86CallProc, PDE_ENTRY_VALUE, 0);
	//makeTssDescriptor((DWORD)&gV86Tss, 3, 0, sizeof(gV86Tss), (SegDescriptor*)(gdt + 0X50));
	//makeTaskGateDescriptor((DWORD)kTssV86Selector, 3, (TaskGateDescriptor*)(descriptor + 0xff));


}