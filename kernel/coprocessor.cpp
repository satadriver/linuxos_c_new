#include "coprocessor.h"
#include "process.h"
#include "task.h"
#include "Utils.h"
#include "video.h"
#include "acpi.h"

int gFpuStatus = 0;

//EM位控制浮点指令的执行是用软件模拟，还是由硬件执行。EM=0时，硬件控制浮点指令传送到协处理器；EM=1时，浮点指令由软件模拟。 
//TS位用于加快任务的切换，通过在必要时才进行协处理器切换的方法实现这一目的。每当进行任务切换时，处理器把TS置1。
//TS = 1时，浮点指令将产生设备不可用(DNA)异常。 
//MP位控制WAIT指令在TS = 1时，是否产生DNA异常。MP = 1和TS = 1时，WAIT产生异常；MP = 0时，WAIT指令忽略TS条件，不产生异常。

//Numeric Error（数值错误）是CR0寄存器的第5位（bit 5）。当设置了NE标志时，启用内部的机制来报告x87 FPU错误；
//当清除NE标志时，启用PC风格的x87 FPU错误报告机制。
//当NE标志被清除并且IGNNE#输入被触发时，x87 FPU错误会被忽略。
//当NE标志被清除并且IGNNE#输入未触发时，未屏蔽的x87 FPU错误会导致处理器断言FERR#引脚以生成外部中断，
//并在执行下一个等待的浮点指令或WAIT/FWAIT指令之前立即停止指令执行。

//CR0.NE (bit 5) When set, enables Native Exception handling which will use the FPU exceptions.
//When cleared, an exception is sent via the interrupt controller.Should be on for 486 + , 
//but not on 386s because they lack that bit.

//MMX, 3DNow and the rare EMMI reuse the old FPU registers as vector units, aliasing them into 64 bit data registers. 
//This means that they can be used safely without modifications of the FPU handling. 
//SSE however adds a whole new set of registers, and therefore is disabled by default. 
//To allow SSE instructions, CR4.OSFXSR should be set. 
//Be careful though since writing it on a processor without SSE support causes an exception. 
//When SSE is enabled, FXSAVE and FXRSTOR should be used to store the entire FPU and vector register file. 
//It is good practice to enable the other SSE bit (CR4.OSXMMEXCPT) as well so that SSE exceptions are routed to the #XF handler, 
//instead of your vector unit automatically disabling itself when an exception occurs. 
//The state of the art includes AVX, which adds

//https://www.felixcloutier.com/x86/
//fsave保存mm0-mm7,fstenv不保存mm0-mm7
//MMX: 将8个FPU寄存器重命名为8个64位MMX寄存器，即mm0到mm7。[多媒体]
//SSE: 8个128位寄存器（从xmm0到xmm7）
void initFPU() {
	WORD fpu_status = 0x37f;
	__asm {
		fnclex
		FLDCW[fpu_status]	// writes 0x37f into the control word: the value written by F(N)INIT
	}
}

//CR0.MP (bit 1) This does little else other than saying if an FWAIT opcode is exempted from responding to the TS bit.
//Since FWAIT will force serialisation of exceptions, it should normally be set to the inverse of the EM bit, 
//so that FWAIT will actually cause a FPU state update when FPU instructions are asynchronous, and not when they are emulated.
void enableAVX() {
	__asm {
		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		//mov eax, cr4
		or ax, 1 << 18	; set OSXSAVE
		//mov cr4, eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0

		xgetbv		; Load XCR0 register
		or eax, 7	; Set AVX, SSE, X87 bits
		xsetbv		; Save back to XCR0

		//To enable AVX - 512, set the OPMASK(bit 5), ZMM_Hi256(bit 6), Hi16_ZMM(bit 7) of XCR0.
		//You must ensure that these bits are valid first(see above).
	}
}


//https://blog.csdn.net/qq_43401808/article/details/86677863
void enableSSE() {
	DWORD mxcsr_reg = 0x1fbf;
	__asm {
		; now enable SSEand the like
		mov eax, cr0
		and ax, 0xFFFB		; clear coprocessor emulation CR0.EM
		or ax, 0x2			; set coprocessor monitoring  CR0.MP
		mov cr0, eax

		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		//mov eax, cr4
		or ax, 3 << 9		; set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
		//mov cr4, eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0

		//ldmxcsr mxcsr_reg
		//stmxcsr mxcsr_reg
	}
}


void initCoprocessor() {

	enableIRQ13();

	__asm {
		mov eax, cr0
		or eax, 0x10		//et = 1
		or eax, 0x20		//ne = 1, trap not interrupt
		mov cr0, eax

		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		//or eax,0x40600
		
		//mov cr4,eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0

		clts
		//FNCLEX
		//fwait
		finit
	}

	enableSSE();

	//enableAVX();
}


//EM = 1,all float instruction exception
//MP = 1 && TS =1,fwait exception
//MP=1 or TS = 1,float instruction exception
void __kCoprocessor() {
	
// 	char szout[1024];
// 	__printf(szout, "coprocessor exceiton\n");

	if (gFpuStatus == 0)
	{
		__asm {
			clts
			fnclex
			//fwait
			finit
			//load_mxcsr(0x1f80)
		}

		gFpuStatus = 1;
	}
	else {
		LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		char * fenv = (char*)FPU_STATUS_BUFFER + (tss->tid << 9);
		__asm {
			clts
			fnclex
			//fwait
			finit
			mov eax,fenv
			//frstor [fenv]
			fxrstor [eax]
		}
	}
}



//https://xem.github.io/minix86/manual/intel-x86-and-64-manual-vol3/o_fe12b1e2a880e0ce-213.html


/*
Interrupt 7—Device Not Available Exception (#NM)Exception ClassFault.DescriptionIndicates one of the following things:
The device-not-available exception is generated by either of three conditions:
The processor executed an x87 FPU floating-point instruction while the EM flag in control register CR0 was set
(1). See the paragraph below for the special case of the WAIT/FWAIT instruction.

The processor executed a WAIT/FWAIT instruction while the MP and TS flags of register CR0 were set,
regardless of the setting of the EM flag.

The processor executed an x87 FPU, MMX, or SSE/SSE2/SSE3 instruction (with the exception of MOVNTI,
PAUSE, PREFETCHh, SFENCE, LFENCE, MFENCE, and CLFLUSH) while the TS flag in control register CR0 was set
and the EM flag is clear.The EM flag is set when the processor does not have an internal x87 FPU floating-point unit.

A device-not-available
exception is then generated each time an x87 FPU floating-point instruction is encountered, allowing an exception
handler to call floating-point instruction emulation routines.
The TS flag indicates that a context switch (task switch) has occurred since the last time an x87 floating-point,
MMX, or SSE/SSE2/SSE3 instruction was executed; but that the context of the x87 FPU, XMM, and MXCSR registers
were not saved. When the TS flag is set and the EM flag is clear, the processor generates a device-not-available
exception each time an x87 floating-point, MMX, or SSE/SSE2/SSE3 instruction is encountered (with the exception
of the instructions listed above). The exception handler can then save the context of the x87 FPU, XMM, and MXCSR
registers before it executes the instruction. See Section 2.5, “Control Registers,” for more information about the TS
flag.
The MP flag in control register CR0 is used along with the TS flag to determine if WAIT or FWAIT instructions should
generate a device-not-available exception. It extends the function of the TS flag to the WAIT and FWAIT instruc-
tions, giving the exception handler an opportunity to save the context of the x87 FPU before the WAIT or FWAIT
instruction is executed. The MP flag is provided primarily for use with the Intel 286 and Intel386 DX processors. For
programs running on the Pentium 4, Intel Xeon, P6 family, Pentium, or Intel486 DX processors, or the Intel 487 SX
coprocessors, the MP flag should always be set; for programs running on the Intel486 SX processor, the MP flag
should be clear. Exception Error CodeNone.
Saved Instruction PointerThe saved contents of CS and EIP registers point to the floating-point instruction or the WAIT/FWAIT instruction
that generated the exception.
Program State ChangeA program-state change does not accompany a device-not-available fault, because the instruction that generated
the exception is not executed.
If the EM flag is set, the exception handler can then read the floating-point instruction pointed to by the EIP and
call the appropriate emulation routine.
If the MP and TS flags are set or the TS flag alone is set, the exception handler can save the context of the x87 FPU,
clear the TS flag, and continue execution at the interrupted floating-point or WAIT/FWAIT instruction.

*/