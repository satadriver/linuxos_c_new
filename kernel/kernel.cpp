#include "Utils.h"
#include "def.h"
#include "Kernel.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "process.h"
#include "task.h"
#include "Pe.h"
#include "satadriver.h"
#include "fat32/FAT32.h"
#include "fat32/fat32file.h" 
#include "file.h"
#include "NTFS/ntfs.h"
#include "NTFS/ntfsFile.h"
#include "pci.h"
#include "speaker.h"
#include "cmosAlarm.h"
#include "rs232.h"
#include "floppy.h"
#include "malloc.h"
#include "page.h"
#include "processDOS.h"
#include "gdi.h"
#include "coprocessor.h"
#include "Thread.h"
#include "debugger.h"
#include "descriptor.h"
#include "elf.h"
#include "page.h"
#include "device.h"
#include "core.h"
#include "cmosPeriodTimer.h"


//#pragma comment(linker, "/ENTRY:DllMain")
//#pragma comment(linker, "/align:512")
//#pragma comment(linker, "/merge:.data=.text")

DWORD gV86VMIEntry = 0;
DWORD gV86Process = 0;
DWORD gKernel16;
DWORD gKernel32;
DWORD gKernelData;


int __kernelEntry(LPVESAINFORMATION vesa, DWORD fontbase,DWORD v86Proc,DWORD v86Addr ,DWORD kerneldata,DWORD kernel16,DWORD kernel32) {

	int ret = 0;
	
	gV86VMIEntry = v86Proc;
	gV86Process = v86Addr;
	gKernelData = kerneldata;
	gKernel16 = kernel16;
	gKernel32 = kernel32;

	__initVideo(vesa, fontbase);

	char szout[1024];

	initGdt();
	initIDT();

	initDevices();

	initMemory();

	initPage();

	enablePage();

	__initTask();

	initDll();

	initRS232Com1();
	initRS232Com2();

	initEfer();

	initCoprocessor();

	initTimer();

	sysEntryInit((DWORD)sysEntry);

	__asm {
		sti
	}

	__printf(szout, "Hello world of Liunux!\r\n");

#ifndef SINGLE_TASK_TSS
	__createDosInFileTask(gV86VMIEntry, "V86VMIEntry");
#endif

// 	TASKCMDPARAMS cmd;
// 	__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
// 	__kCreateThread((DWORD)__kSpeakerProc, (DWORD)&cmd, "__kSpeakerProc");

	//logFile("__kernelEntry\n");
	
// 	DWORD kernelMain = getAddrFromName(KERNEL_DLL_BASE, "__kKernelMain");
// 	if (kernelMain)
// 	{
// 		TASKCMDPARAMS cmd;
// 		__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
// 		__kCreateThread((unsigned int)kernelMain,(DWORD)&cmd, "__kKernelMain");
// 	}

	initFileSystem();

	initDebugger();

// 	floppyInit();
// 	FloppyReadSector(0, 1, (unsigned char*)FLOPPY_DMA_BUFFER);

// 	ret = loadLibRunFun("c:\\liunux\\main.dll", "__kMainProcess");
 	
	int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
	__printf(szout, "__kMainProcess size:%x\n", imagesize);
	__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize,  "__kExplorer", 3, 0);

	while (1)
	{
		if (__findProcessFuncName("__kExplorer") == FALSE)
		{
			__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kExplorer", 3, 0);
		}

		__asm {
			hlt
		}
	}

	return 0;
}



void __kKernelMain(DWORD retaddr,int pid,char * filename,char * funcname,DWORD param) {

	int ret = 0;

 	char szout[1024];
	__printf(szout, "__kKernelMain task pid:%x,filename:%s,function name:%s\n", pid, filename,funcname);

 	unsigned char sendbuf[1024];
 	//最大不能超过14字节
 	__strcpy((char*)sendbuf, "how are you?");
 	ret = sendCom2Data(sendbuf, __strlen("how are you?"));
 
 	unsigned char recvbuf[1024];
 	int recvlen = getCom2Data(recvbuf);
 	if (recvlen > 0)
 	{
 		*(recvbuf + recvlen) = 0;
 		__printf(szout, "recvbuf data:%s\n", recvbuf);
 	}

	while (1)
	{
		__asm {
			hlt
		}
	}
}



//注意二位数组在内存中的排列和结构
void mytest() {
	char tmp[0x4000] = { 0 };

	PCI_CONFIG_VALUE pci = { 0 };
	PCI_CONFIG_VALUE* lppci = &pci;
	pci.enable = 1;
	pci.bus = 44;
	pci.dev = 14;
	pci.func = 5;
	pci.reg = 22;

	TssDescriptor d;

	TssDescriptor* ld = &d;

	IntTrapGateDescriptor i;

	IntTrapGateDescriptor* li = &i;

	initKernelTss((TSS*)tmp, TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)0x12345678, 3,  0xabcd, (TssDescriptor*)&d);

	makeIntGateDescriptor((DWORD)0x12345678, KERNEL_MODE_CODE, 3, &i);

	return;
}

#ifdef _USRDLL
int __stdcall DllMain( HINSTANCE hInstance,  DWORD fdwReason,  LPVOID lpvReserved) {
	return TRUE;
}
#else
int __stdcall WinMain(  HINSTANCE hInstance,  HINSTANCE hPrevInstance,  LPSTR lpCmdLine,  int nShowCmd )
{
	mytest();
	return TRUE;
}
#endif