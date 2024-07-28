#include "v86.h"
#include "def.h"

#include "Utils.h"
#include "video.h"
#include "satadriver.h"
#include "task.h"
#include "atapi.h"
#include "core.h"






int v86Process(int reax,int recx,int redx,int rebx,int resi,int redi,int rds, int res,int cmd ){

do {
	TssDescriptor* lptssd = (TssDescriptor*)(GDT_BASE + kTssV86Selector);
	TSS* tss = (TSS*)V86_TSS_BASE;
	if ((lptssd->type & 2) || (tss->link)) {
		__sleep(0);
		break;
	}
	else {
		break;
	}
} while (TRUE);

	V86_INT_PARAMETER* param = (V86_INT_PARAMETER*)V86_INT_ADDRESS;
	param->reax = reax;
	param->recx = recx;
	param->redx = redx;
	param->rebx = rebx;
	param->resi = resi;
	param->redi = redi;
	param->rds = rds;
	param->res = res;
	param->int_cmd = cmd;

	__asm {	
		int 255
	}

	return param->result;
}







int getVideoMode(VesaSimpleInfo vsi[64] ) {

	int res = 0;
	int idx = 0;

	char szout[1024];

	//res = v86Process(0x4f02, 0, 0, 0x4108, 0, VESA_STATE_OFFSET, 0, VESA_STATE_SEG, 0x10);
	//return 0;

	res = v86Process(0x4f00, 0, 0, 0, 0, VESA_STATE_OFFSET, 0, VESA_STATE_SEG ,0x10 );
	if ((res & 0xffff) == 0x4f) {
		VESAINFOBLOCK* vib = (VESAINFOBLOCK*)VESA_STATE_ADDRESS;

		WORD * addr = (WORD*)(vib->mode_dos_offset + (vib->mode_dos_seg << 4));

		WORD mode = *addr;
		while (mode != 0 && mode != 0xffff) {

			res = v86Process(0x4f01, mode, 0, 0, 0, VESA_STATE_OFFSET + 0x100, 0, VESA_STATE_SEG, 0x10);
			if ((res & 0xffff) == 0x4f)
			{
				VESAINFORMATION * vi = (VESAINFORMATION*)(VESA_STATE_OFFSET + 0x100 + (VESA_STATE_SEG << 4));
				if (vi->ModeAttr & 0x80) {
					if (vi->BitsPerPixel >= 24) {
						if (vi->XRes >= 800 && vi->YRes >= 600) {

							vsi[idx].mode = mode;
							vsi[idx].x = vi->XRes;
							vsi[idx].y = vi->YRes;
							vsi[idx].bpp = vi->BitsPerPixel / 8;

							vsi[idx].base = vi->PhyBasePtr;
							vsi[idx].offset = vi->OffScreenMemOffset;
							vsi[idx].size = vi->OffScreenMemSize;

							idx++;
						}
					}
				}
			}
			addr++;
			mode = *addr;
		} 
	}
	
	return idx;
}


/*
-----------------------------------------------------------
				����0x00������VBE��Ϣ
------------------------------------------------------
��ڣ�
	AX			0x4F00
	ES��DI		ָ��VBE��Ϣ���ָ��
���ڣ�
	AX			VBE����ֵ
------------------------------------------------------------

-----------------------------------------------------------
			����0x01������VBE�ض�ģʽ��Ϣ
------------------------------------------------------
��ڣ�
	AX			0x4F01
	CX			ģʽ��
	ES��DI		ָ��VBE�ض�ģʽ��Ϣ���ָ��
���ڣ�
	AX			VBE����ֵ
------------------------------------------------------------

-----------------------------------------------------------
			����0x02������VESA VBE ģʽ
------------------------------------------------------
��ڣ�
	AX			0x4F02
	BX			ģʽ��
���ڣ�
	AX			VBE����ֵ
------------------------------------------------------------
������ģʽʧ��ʱ�����ش�����룬һ�㷵��AH=01H

VESA 2.0����������BX��D14��D15��λ���壬�����������£�
BX = ģʽ��
	D0��D8��9λģʽ��
	D9��D13������������Ϊ0
	D14 = 0��ʹ����ͨ�Ĵ���ҳ�滺��ģʽ����VBE����05H�л���ʾҳ��
		= 1��ʹ�ô�����Ի����������ַ�ɴ�VBE����01H�ķ�����ϢModeInfo���
	D15 = 0�������ʾ�ڴ�
		= 1���������ʾ�ڴ�
------------------------------------------------------------
*/




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
AL = 0 ����
DL = ��������

����:
CF = 0, AH = 0 �ɹ�
CF = 1, AH = ������
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
