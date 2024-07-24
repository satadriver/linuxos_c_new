#include "v86.h"
#include "def.h"

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