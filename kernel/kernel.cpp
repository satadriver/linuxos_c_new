#include "Utils.h"
#include "def.h"
#include "Kernel.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "process.h"
#include "task.h"
#include "Pe.h"
#include "ata.h"
#include "fat32/FAT32.h"
#include "fat32/fat32file.h" 
#include "file.h"
#include "NTFS/ntfs.h"
#include "NTFS/ntfsFile.h"
#include "pci.h"
#include "speaker.h"
#include "cmosAlarm.h"
#include "serialUART.h"
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
#include "apic.h"
#include "acpi.h"


//#pragma comment(linker, "/ENTRY:DllMain")
//#pragma comment(linker, "/align:512")
//#pragma comment(linker, "/merge:.data=.text")

//https://www.cnblogs.com/ck1020/p/6115200.html



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

	initPaging();

	__initTask();

	initDll();

	initEfer();

	initACPI();

	initCoprocessor();

	initTimer();

	sysEntryInit((DWORD)sysEntry);

	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();

	initDebugger();

	__asm {
		in al, 0x60
		sti
	}

	//__printf(szout, "Hello world of Liunux!\r\n");

#ifdef SINGLE_TASK_TSS
	__createDosInFileTask(gV86VMIEntry, "V86VMIEntry");
#else
	__createDosInFileTask(gV86VMIEntry, "V86VMIEntry");
#endif

	initFileSystem();

	int imagesize = getSizeOfImage((char*)KERNEL_DLL_SOURCE_BASE);
	DWORD kernelMain = getAddrFromName(KERNEL_DLL_BASE, "__kKernelMain");
	if (kernelMain)
	{
		TASKCMDPARAMS cmd;
		__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
		//__kCreateThread((DWORD)__kSpeakerProc, (DWORD)&cmd, "__kSpeakerProc");
		//__kCreateThread((unsigned int)kernelMain, KERNEL_DLL_BASE, (DWORD)&cmd, "__kKernelMain");
		__kCreateProcess((unsigned int)KERNEL_DLL_SOURCE_BASE, imagesize, "kernel.dll", "__kKernelMain", 3, 0);
	}

	//logFile("__kernelEntry\n");
	
	//ret = loadLibRunFun("c:\\liunux\\main.dll", "__kMainProcess");

	//__kGetKbd(0);

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

	char* str = "Hi,how are you?Fine,thank you, and you ? I'm fine too!";

 	ret = sendUARTData((unsigned char*)str, __strlen(str),COM1PORT);
 
 	unsigned char recvbuf[1024];
 	int recvlen = getCom1Data(recvbuf);
 	if (recvlen > 0)
 	{
 		*(recvbuf + recvlen) = 0;

 		__printf(szout, "com recv data:%s\n", recvbuf);
 	}
	return;
}



//ע���λ�������ڴ��е����кͽṹ
void mytest(int p) {
	__asm {
		lea eax,p

	}
	return;
}

#ifdef _USRDLL
int __stdcall DllMain( HINSTANCE hInstance,  DWORD fdwReason,  LPVOID lpvReserved) {
	return TRUE;
}
#else
int __stdcall WinMain(  HINSTANCE hInstance,  HINSTANCE hPrevInstance,  LPSTR lpCmdLine,  int nShowCmd )
{
	mytest(0);
	return TRUE;
}
#endif