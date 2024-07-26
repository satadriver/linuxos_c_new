#include "v86.h"
#include "def.h"

#include "Utils.h"
#include "video.h"
#include "satadriver.h"
#include "task.h"
#include "atapi.h"

#pragma pack(1)


typedef struct {
	
	DWORD reax;
	DWORD recx;
	DWORD redx;
	DWORD rebx;
	DWORD resi;
	DWORD redi;
	DWORD int_cmd;
}V86_INT_PARAMETER;

#pragma pack()



int v86Process(int reax,int recx,int redx,int rebx,int resi,int redi,int cmd){

	V86_INT_PARAMETER* param = (V86_INT_PARAMETER*)V86_INT_ADDRESS;
	param->reax = reax;
	param->recx = recx;
	param->redx = redx;
	param->rebx = rebx;
	param->resi = resi;
	param->redi = redi;

	param->int_cmd = cmd;

	__asm {	
		int 255
	}
	return 0;
}









void saveScreen() {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;

	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x10;
	params->reax = 0x4f04;
	//cx:
	//d0:hardware
	//d1:bios
	//d2:dac
	//d3:register
	params->recx = 1;
	params->redx = 1;
	params->rebx = VESA_STATE_OFFSET;
	params->resi = 0;
	params->redi = 0;
	params->res = VESA_STATE_SEG;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
}


void restoreScreen() {
	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x10;
	params->reax = 0x4f04;
	params->recx = 1;
	params->redx = 2;
	params->rebx = VESA_STATE_OFFSET;
	params->resi = 0;
	params->redi = 0;
	params->res = VESA_STATE_SEG;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
}

int setGraphMode(int mode) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x10;
	params->reax = 0x4f02;
	params->recx = 0;
	params->redx = 0;
	params->rebx = mode;
	params->resi = 0;
	params->redi = 0;
	params->res = 0;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
	if (params->result)
	{
		return FALSE;
	}

	return TRUE;
}





int getAtapiDev(int disk, int maxno) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;

	for (int dev = disk; dev <= maxno; dev++)
	{
		while (params->bwork == 1)
		{
			__sleep(0);
		}

		params->intno = 0x13;
		params->reax = 0x4100;
		params->recx = 0;
		params->redx = dev;
		params->rebx = 0x55aa;
		params->resi = 0;
		params->redi = 0;
		params->res = 0;
		params->rds = 0;
		params->result = 0;

		params->bwork = 1;

		while (params->bwork == 1)
		{
			__sleep(0);
		}

		if (params->result)
		{
			return dev;
		}
	}

	return -1;
}

/*
AH = 46h
AL = 0 保留
DL = 驱动器号

返回:
CF = 0, AH = 0 成功
CF = 1, AH = 错误码
*/
int rejectCDROM(int dev) {
	if (dev <= 0)
	{
		dev = getAtapiDev(0x81, 0xff);
		if (dev <= 0)
		{
			return FALSE;
		}
	}

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x13;
	params->reax = 0x4600;
	params->recx = 0;
	params->redx = dev;
	params->rebx = 0;
	params->resi = 0;
	params->redi = 0;
	params->res = 0;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
	if (params->result)
	{
		return dev;
	}

	return FALSE;
}
