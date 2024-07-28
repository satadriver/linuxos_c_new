
#include "textMode.h"
#include "file.h"
#include "core.h"
#include "device.h"
#include "task.h"
#include "servicesProc.h"
#include "rs232.h"
#include "coprocessor.h"
#include "Pe.h"
#include "cmosPeriodTimer.h"
#include "debugger.h"
#include "hardware.h"
#include "Utils.h"
#include "v86.h"
#include "textMode.h"
#include "mouse.h"
#include "keyboard.h"
#include "pci.h"
#include "hardware.h"
#include "kernel.h"



#define INPUT_TEXTMODE_COLOR		0X0f
#define OUTPUT_TEXTMODE_COLOR		0X0a

#define LINE_CHAR_COUNT				80
#define ROW_CHAR_COUNT				25
#define LINE_SIZE					(LINE_CHAR_COUNT<<1)

#define TEXTMODE_BASE				0XB8000



char* gTxtBuf = 0;

int gTxtOffset = 0;



int runcmd(char * cmd) {
	int res = 0;

	if (__strcmp(cmd, "windows") == 0) {

		char szout[1024];

		VesaSimpleInfo vsi[64];

		res = getVideoMode(vsi);

		for (int idx = 0; idx < res; idx++) {
			__sprintf(szout, "mode:%d,width:%d,height:%d,bytesPerPixel:%d,base:%x,offset:%x,size:%x\r\n",
				vsi[idx].mode, vsi[idx].x, vsi[idx].y, vsi[idx].bpp, vsi[idx].base, vsi[idx].offset, vsi[idx].size);
			outputStr(szout, OUTPUT_TEXTMODE_COLOR);
		}

	}
	else if (__strcmp(cmd, "cpuinfo") == 0) {

		char cpu[1024];
		getCpuInfo(cpu);
		outputStr(cpu, OUTPUT_TEXTMODE_COLOR);
	}
	else if (__strcmp(cmd, "cputype") == 0) {

		char cpu[1024];
		getCpuType(cpu);
		outputStr(cpu, OUTPUT_TEXTMODE_COLOR);
	}
	else if (__strcmp(cmd, "timestamp") == 0) {

	}
	else if (__strcmp(cmd, "date") == 0) {

		DATETIME dt;
		__getDateTime(&dt);
		char datetime[1024];
		__sprintf(datetime, "year:%d,month:%d,day:%d,hour:%d,minute:%d,second:%d\r\n",
			dt.year, dt.month, dt.dayInMonth, dt.hour, dt.minute, dt.second);
		outputStr(datetime, OUTPUT_TEXTMODE_COLOR);
	}
	
	return 0;
}

int outputStr(char* str,char color) {
	int size = __strlen(str);
	for (int i = 0; i < size; i++) {
		outputChar(str[i], color);
	}
	return 0;
}


int outputChar(char c,char color) {


	if (c == 0x0a ) {
		gTxtOffset = (gTxtOffset / LINE_SIZE) * LINE_SIZE + LINE_SIZE;
		if (gTxtOffset >= LINE_SIZE * ROW_CHAR_COUNT) {
			gTxtOffset = 0;
		}
	}
	else if (c == 0x0d) {
		//int mod = gTxtOffset % LINE_SIZE;
		//if (mod) {
		//	gTxtOffset -= mod;
		//}
		//gTxtOffset += LINE_SIZE;

		gTxtOffset = (gTxtOffset / LINE_SIZE) * LINE_SIZE ;	
		if (gTxtOffset >= LINE_SIZE * ROW_CHAR_COUNT) {
			gTxtOffset = 0;
		}
	}
	else if (c == 0x08) {
		gTxtOffset -= 2;
		if (gTxtOffset <= 0) {
			gTxtOffset = 0;
		}
		*(gTxtBuf + gTxtOffset) = 0;
		*(gTxtBuf + gTxtOffset + 1) = 0;
	}
	else {
		*(gTxtBuf + gTxtOffset) = c;
		gTxtOffset++;
		*(gTxtBuf + gTxtOffset) = color;
		gTxtOffset++;
		if (gTxtOffset >= LINE_SIZE * ROW_CHAR_COUNT) {
			gTxtOffset = 0;
		}
	}

	setCursor(gTxtOffset/2);
	return 0;
}


extern "C" __declspec(dllexport) int __kTextModeEntry(LPVESAINFORMATION vesa, DWORD fontbase, 
	DWORD v86Proc, DWORD v86Addr, DWORD kerneldata, DWORD kernel16, DWORD kernel32) {
	int res = 0;

	gV86VMIEntry = v86Proc;
	gV86Process = v86Addr;
	gKernelData = kerneldata;
	gKernel16 = kernel16;
	gKernel32 = kernel32;

 	DWORD svgaregs[16];
 	DWORD svgadev = 0;
 	DWORD svgairq = 0;
 	res = getPciDevBasePort(svgaregs, 0x0300, &svgadev, &svgairq);
 	if ( svgaregs && (svgaregs[0] & 1) == 0 )		
 	{
		gTxtBuf = (char*)(svgaregs[0] & 0xfffffff0);
 	}

	gTxtBuf = (char*)TEXTMODE_BASE;

	gTxtOffset = 0;

	initGdt();
	initIDT();

	initTextModeDevices();

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

	__asm {
		in al, 60h
		sti
		
	}

	initFileSystem();

	initDebugger();

	char cmd[1024];
	char* lpcmd = cmd;
	while (1)
	{
		unsigned int asc = __kGetKbd(0) & 0xff;
		//unsigned int asc = __getchar(0);
		//res = isScancodeAsc(asc);
		if (asc)
		{
			__sleep(0);

			outputChar(asc, INPUT_TEXTMODE_COLOR);
			*lpcmd= asc;
			lpcmd++;
			if (lpcmd - cmd >= sizeof(cmd)) {
				lpcmd = cmd;
			}
			else if (asc == 0x0a) {
				lpcmd--;
				*lpcmd = 0;
				lpcmd = cmd;
				runcmd(cmd);
			}
			else if (asc == 0x08) {
				lpcmd--;
				*lpcmd = 0;
				lpcmd--;
				if (lpcmd <= cmd) {
					lpcmd = cmd;
				}
				*lpcmd = 0;
			}
		}		
	}

	return 0;
}



int setCursor(int pos) {

	outportb(0x3d4, 0x0e);
	//int h = inportb(0x3d5);
	outportb(0x3d5, (pos>>8)&0xff);

	outportb(0x3d4, 0x0f);
	outportb(0x3d5, pos  & 0xff);
	//int h = inportb(0x3d5);
	return 0;
}