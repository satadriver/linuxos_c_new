#include "process.h"
#include "task.h"
#include "Utils.h"
#include "Kernel.h"
#include "video.h"
#include "Pe.h"
#include "processDOS.h"
#include "file.h"
#include "timer8254.h"
#include "page.h"
#include "def.h"
#include "malloc.h"
#include "core.h"
#include "vectorRoutine.h"

TASK_LIST_ENTRY *gTasksListPtr = 0;


void __terminateTask(int tid, char * filename, char * funcname, DWORD lpparams) {

	removeTaskList(tid);
	__sleep(-1);
}


TASK_LIST_ENTRY* searchTaskList(int tid) {
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid  && list[i].process->status == TASK_RUN && list[i].process->tid == tid) {
			return &list[i];
		}
	}
	return 0;
}


TASK_LIST_ENTRY* addTaskList(int tid) {
	LPPROCESS_INFO base = (LPPROCESS_INFO)TASKS_TSS_BASE;

	//TASK_LIST_ENTRY* tasklist = (TASK_LIST_ENTRY*)gTasksListPtr;

	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid == 0 ) {
			list[i].valid = TRUE;

			list[i].process = base + tid;
			list[i].process->status = TASK_RUN;
			
			addlistTail((LIST_ENTRY*)&gTasksListPtr->list, (LIST_ENTRY*)&list[i].list);
			return &list[i];
		}
	}
	return 0;
}



TASK_LIST_ENTRY* removeTaskList(int tid) {

	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)gTasksListPtr;
	do 
	{
		if (list->valid && list->process && list->process->tid == tid)
		{
			if (gTasksListPtr == list) {
				//gTasksListPtr = (TASK_LIST_ENTRY*)list->list.next;
			}

			removelist((LIST_ENTRY*)&list->list);

			list->process->status = TASK_OVER;
			list->process = 0;

			list->valid = FALSE;

			return list;
		}
		list = (TASK_LIST_ENTRY *)list->list.next;

	} while (list && list != (TASK_LIST_ENTRY *)gTasksListPtr);

	return 0;
}

//CF(bit 0) [Carry flag]   
//若算术操作产生的结果在最高有效位(most-significant bit)发生进位或借位则将其置1，反之清零。
//这个标志指示无符号整型运算的溢出状态，这个标志同样在多倍精度运算(multiple-precision arithmetic)中使用

//SF(bit 7) [Sign flag]   
//该标志被设置为有符号整型的最高有效位。(0指示结果为正，反之则为负) 

//OF(bit 11) [Overflow flag]   
//如果整型结果是较大的正数或较小的负数，并且无法匹配目的操作数时将该位置1，反之清零。这个标志为带符号整型运算指示溢出状态

//OF是有符号数运算结果的标志
//OF标志：这个标志有点复杂，其结果是CF标志和次最高位是否发生进位（如果进位是1，没进位是0）进行异或的结果
//OF只对有符号数运算有意义，CF对无符号数运算有意义
//MOV AX,858F
//SUB AX,7869

// IOPL是I/O保护机制中的关键之一，它位于EFLAGS寄存器的第12、13位。指令in、ins、out、outs、cli、sti只有在CPL<= IOPL时才能执行。
//这些指令被称为I/O敏感指令，如果特权级低的指令视图访问这些I/O敏感指令将会导致常规保护错误(#GP)
//可以改变IOPL的指令只有popfl和iret指令，但只有运行在特权级0的程序才能将其改变


void clearTssBuf(LPPROCESS_INFO tss) {
	__memset((CHAR*)tss, 0, sizeof(PROCESS_INFO));
	tss->status = TASK_SUSPEND;

	tss->tss.iomapEnd = 0xff;
	tss->tss.iomapOffset = 136;

	//tss->tss.trap = 1;
}


int __getFreeTask(LPTASKRESULT ret) {
	if (ret == 0)
	{
		return FALSE;
	}
	ret->lptss = 0;
	ret->number = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0;i < TASK_LIMIT_TOTAL; i++)
	{
		if (tss[i].status == TASK_OVER)
		{
			clearTssBuf(&tss[i]);

			ret->number = i;
			ret->lptss = &tss[i];
			return TRUE;
		}
	}

	return FALSE;
}



TASK_LIST_ENTRY* __findProcessFuncName(char * funcname) {
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	do
	{
		if ( (list->process->status == TASK_RUN) && (__strcmp(list->process->funcname,funcname) == 0))
		{
			return list;
		}
		list = (TASK_LIST_ENTRY *)list->list.next;
		if (list == 0)
		{
			break;
		}
	} while (list != (TASK_LIST_ENTRY *)TASKS_LIST_BASE);

	return FALSE;
}

TASK_LIST_ENTRY * __findProcessFileName(char * filename) {
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	do
	{
		if (list->process->status == TASK_RUN && __strcmp(list->process->filename, filename) == 0)
		{
			return list;
		}
		list = (TASK_LIST_ENTRY *)list->list.next;
	} while (list != (TASK_LIST_ENTRY *)TASKS_LIST_BASE);

	return FALSE;
}



TASK_LIST_ENTRY* __findProcessByPid(int pid) {
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	do
	{
		if (list->process->status == TASK_RUN && list->process->pid == pid)
		{
			return list;
		}
		list = (TASK_LIST_ENTRY *)list->list.next;

	} while (list != (TASK_LIST_ENTRY *)TASKS_LIST_BASE);

	return FALSE;
}


TASK_LIST_ENTRY* __findProcessByTid(int tid) {
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	do
	{
		if (list->process->status == TASK_RUN && list->process->tid == tid)
		{
			return list;
		}
		list = (TASK_LIST_ENTRY *)list->list.next;
	} while (list != (TASK_LIST_ENTRY *)TASKS_LIST_BASE);

	return FALSE;
}


int __terminateByFileName(char * filename) {

	TASK_LIST_ENTRY* list = __findProcessFileName(filename);
	if (list)
	{
		list->process->status = TASK_OVER;
		removeTaskList(list->process->pid);
	}

	return FALSE;
}

int __terminateByFuncName(char * funcname) {

	TASK_LIST_ENTRY* list = __findProcessFuncName(funcname);
	if (list)
	{
		list->process->status = TASK_OVER;
		removeTaskList(list->process->pid);
	}

	return FALSE;
}

int __terminatePid(int pid) {

	TASK_LIST_ENTRY* list = __findProcessByPid(pid);
	if (list)
	{
		list->process->status = TASK_OVER;
		removeTaskList(list->process->pid);
	}

	return 0;
}


int __terminateTid(int tid) {

	TASK_LIST_ENTRY* list = __findProcessByTid(tid);
	if (list)
	{
		list->process->status = TASK_OVER;
		removeTaskList(list->process->pid);
	}

	return 0;
}



int __pauseTid(int tid) {

	TASK_LIST_ENTRY* list = __findProcessByTid(tid);
	if (list)
	{
		list->process->status = TASK_SUSPEND;
	}

	return 0;
}


int __resumeTid(int tid) {

	TASK_LIST_ENTRY* list = __findProcessByTid(tid);
	if (list)
	{
		list->process->status = TASK_RUN;
	}

	return 0;
}


int __pausePid(int pid) {

	TASK_LIST_ENTRY* list = __findProcessByPid(pid);
	if (list)
	{
		list->process->status = TASK_SUSPEND;
	}

	return 0;
}


int __resumePid(int pid) {

	TASK_LIST_ENTRY* list = __findProcessByPid(pid);
	if (list)
	{
		list->process->status = TASK_RUN;
	}

	return 0;
}


int __createDosInFileTask(DWORD addr, char* filename) {
	if (__findProcessFileName(filename))
	{
		return 0;
	}
	return __kCreateProcess(addr, 0x1000, filename, filename, DOS_PROCESS_RUNCODE | 3, 0);
}



#ifndef SINGLE_TASK_TSS
extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT* regs) {

	__k8254TimerProc();

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + process->tid);
	if (prev->status == TASK_TERMINATE) {
		prev->status = TASK_OVER;
		if (prev->tid == prev->pid) {
			//__kFreeProcess(prev->pid);
		}
		else {
			//__kFree(prev->espbase);
		}		
	}
	LPPROCESS_INFO next = prev;
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == prev) {
			return FALSE;
		}

		if (next->status == TASK_TERMINATE) {
			next->status = TASK_OVER;
			if (next->tid == next->pid) {
				//__kFreeProcess(next->pid);
			}
			else {
				//__kFree(next->espbase);
			}
		}

		if (next->status == TASK_RUN) {
			if (next->sleep) {
				next->sleep--;
			}
			else {
				break;
			}
		}
	} while (next != prev);
	
	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	process->counter++;
	__memcpy((char*)(tss + prev->tid), (char*)process, sizeof(PROCESS_INFO));
	__memcpy((char*)process, (char*)(next->tid + tss), sizeof(PROCESS_INFO));
	
	//tasktest();

 	char * fenvprev = (char*)FPU_STATUS_BUFFER + (prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions, the save EIP points to the instruction that caused the exception.
	__asm {
		clts			//before all fpu instructions
		fwait
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
		//FNCLEX
	}
	prev->fpu = 1;

	if (next->fpu)
	{
		char * fenvnext = (char*)FPU_STATUS_BUFFER + (next->tid << 9);
		__asm {
			clts
			fwait
			finit
			mov eax, fenvnext
			//frstor [fenv]
			fxrstor[eax]
		}
	}

	return TRUE;
}
#else
extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT * env) {

	__k8254TimerProc();

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + process->tid);
	if (prev->status == TASK_TERMINATE) {
		prev->status = TASK_OVER;
		if (prev->tid == prev->pid) {
			__kFreeProcess(prev->pid);
		}
		else {
			__kFree(prev->espbase);
		}
	}
	LPPROCESS_INFO next = prev;
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == prev) {
			return FALSE;
		}

		if (next->status == TASK_TERMINATE) {
			next->status = TASK_OVER;
			if (next->tid == next->pid) {
				__kFreeProcess(next->pid);
			}
			else {
				__kFree(next->espbase);
			}
		}

		if (next->status == TASK_RUN) {
			if (next->sleep) {
				next->sleep--;
			}
			else {
				break;
			}
		}
	} while (next != prev);

	process->tss.eax = env->eax;
	process->tss.ecx = env->ecx;
	process->tss.edx = env->edx;
	process->tss.ebx = env->ebx;
	process->tss.esp = env->esp;
	process->tss.ebp = env->ebp;
	process->tss.esi = env->esi;
	process->tss.edi = env->edi;
	process->tss.ss = env->ss;
	process->tss.gs = env->gs;
	process->tss.fs = env->fs;
	process->tss.ds = env->ds;
	process->tss.es = env->es;

	process->tss.eip = env->eip;
	process->tss.cs = env->cs;
	process->tss.eflags = env->eflags;

	DWORD dwcr3 = 0;
	__asm {
		mov eax,cr3
		mov dwcr3,eax
	}
	process->tss.cr3 = dwcr3;

	if (env->eflags & 0x20000) {
		process->tss.gs = KERNEL_MODE_DATA;
		process->tss.fs = KERNEL_MODE_DATA;
		process->tss.ds = KERNEL_MODE_DATA;
		process->tss.es = KERNEL_MODE_DATA;
		process->tss.ss = KERNEL_MODE_DATA;
	}

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	process->counter++;
	__memcpy((char*)(tss + prev->tid), (char*)process, sizeof(PROCESS_INFO));
	__memcpy((char*)process, (char*)(next->tid + tss), sizeof(PROCESS_INFO));

	if (process->tss.eflags & 0x20000) {

	}
	else if (process->tss.cs & 3) {
		//env->ss = KERNEL_MODE_STACK;
	}
	else {

	}

	//tasktest();

	char* fenvprev = (char*)FPU_STATUS_BUFFER + (prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions, the save EIP points to the instruction that caused the exception.
	__asm {
		clts			//before all fpu instructions
		fwait
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
		//FNCLEX
	}
	prev->fpu = 1;

	if (next->fpu)
	{
		char* fenvnext = (char*)FPU_STATUS_BUFFER + (next->tid << 9);
		__asm {
			clts
			fwait
			finit
			mov eax, fenvnext
			//frstor [fenv]
			fxrstor[eax]
		}
	}

	env->eax = process->tss.eax;
	env->ecx = process->tss.ecx;
	env->edx = process->tss.edx;
	env->ebx = process->tss.ebx;
	env->esp = process->tss.esp;
	env->ebp = process->tss.ebp;
	env->esi = process->tss.esi;
	env->edi = process->tss.edi;
	env->gs = process->tss.gs;
	env->fs = process->tss.fs;
	env->ds = process->tss.ds;
	env->es = process->tss.es;
	env->ss = process->tss.ss;

	return TRUE;
}
#endif


void tasktest(TASK_LIST_ENTRY *gTasksListPtr, TASK_LIST_ENTRY*gPrevTasksPtr) {
	static int gTestFlag = 0;
	if (gTestFlag >= 0 && gTestFlag <= -1)
	{
		char szout[1024];
		__printf(szout,
			"saved  cr3:%x,pid:%x,name:%s,level:%u,esp0:%x,ss0:%x,eip:%x,cs:%x,esp3:%x,ss3:%x,eflags:%x,link:%x,\r\n"
			"loaded cr3:%x,pid:%x,name:%s,level:%u,esp0:%x,ss0:%x,eip:%x,cs:%x,esp3:%x,ss3:%x,eflags:%x,link:%x.\r\n\r\n",
			gPrevTasksPtr->process->tss.cr3, gPrevTasksPtr->process->pid, gPrevTasksPtr->process->filename, gPrevTasksPtr->process->level,
			gPrevTasksPtr->process->tss.esp0, gPrevTasksPtr->process->tss.ss0, gPrevTasksPtr->process->tss.eip, gPrevTasksPtr->process->tss.cs,
			gPrevTasksPtr->process->tss.esp, gPrevTasksPtr->process->tss.ss, gPrevTasksPtr->process->tss.eflags, gPrevTasksPtr->process->tss.link,
			gTasksListPtr->process->tss.cr3, gTasksListPtr->process->pid, gTasksListPtr->process->filename, gTasksListPtr->process->level,
			gTasksListPtr->process->tss.esp0, gTasksListPtr->process->tss.ss0, gTasksListPtr->process->tss.eip, gTasksListPtr->process->tss.cs,
			gTasksListPtr->process->tss.esp, gTasksListPtr->process->tss.ss, gTasksListPtr->process->tss.eflags, gTasksListPtr->process->tss.link);
		gTestFlag++;
	}
}


void initTaskSwitchTss() {

	DescriptTableReg idtbase;
	__asm {
		sidt idtbase
	}

	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)idtbase.addr;

	initKernelTss((TSS*)CURRENT_TASK_TSS_BASE, TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,
		KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor(CURRENT_TASK_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTaskSelector));
#ifdef SINGLE_TASK_TSS
	makeIntGateDescriptor((DWORD)TimerInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 0);
#else
	initKernelTss((TSS*)TIMER_TSS_BASE, TSSTIMER_STACK0_TOP, TSSTIMER_STACK_TOP, (DWORD)TimerInterrupt, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)TIMER_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTimerSelector));
	makeTaskGateDescriptor((DWORD)kTssTimerSelector, 3, (TaskGateDescriptor*)(descriptor + INTR_8259_MASTER + 0));
#endif

	__asm
	{
		mov eax, kTssTaskSelector
		ltr ax
		mov ax, ldtSelector
		lldt ax
	}
}

int __initTask() {

	LPPROCESS_INFO tssbase = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		__memset((char*)&tssbase[i], 0, sizeof(PROCESS_INFO));
	}

	//initTaskSwitchTss();
	LPPROCESS_INFO process0 = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	__strcpy(process0->filename, KERNEL_DLL_MODULE_NAME);
	__strcpy(process0->funcname, "__kernelEntry");
	process0->tid = 0;
	process0->pid = 0;
	process0->moduleaddr = (DWORD)KERNEL_DLL_BASE;
	process0->level = 0;
	process0->counter = 0;
	process0->status = TASK_RUN;
	process0->vaddr = KERNEL_DLL_BASE;
	process0->vasize = 0;
	process0->espbase = KERNEL_TASK_STACK_TOP;

	/*
	LPTASKPARAMS params = (LPTASKPARAMS)(process0->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS));
	RETUTN_ADDRESS_0* ret0 = (RETUTN_ADDRESS_0*)((char*)params - sizeof(RETUTN_ADDRESS_0));
	ret0->cs = process0->tss.cs;
	ret0->eip = process0->tss.eip;
	ret0->eflags = process0->tss.eflags;
	process0->tss.esp = (DWORD)ret0;
	process0->tss.ebp = (DWORD)ret0;

	params->terminate = (DWORD)__terminateProcess;
	params->terminate2 = (DWORD)__terminateProcess;
	params->tid = 0;
	__strcpy(params->szFileName, process0->filename);
	params->filename = params->szFileName;
	__strcpy(params->szFuncName, process0->funcname);
	params->funcname = params->szFuncName;
	params->lpcmdparams = &params->cmdparams;
	*/

	__memcpy((char*)TASKS_TSS_BASE, (char*)CURRENT_TASK_TSS_BASE, sizeof(PROCESS_INFO));

	__memset((char*)TASKS_LIST_BASE, 0, TASK_LIMIT_TOTAL * sizeof(TASK_LIST_ENTRY));

	gTasksListPtr = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	initListEntry(&gTasksListPtr->list);
	gTasksListPtr->process = (LPPROCESS_INFO)TASKS_TSS_BASE;
	gTasksListPtr->valid = TRUE;

	//__memset((char*)V86_TASKCONTROL_ADDRESS, 0, LIMIT_V86_PROC_COUNT*12);
	return 0;
}