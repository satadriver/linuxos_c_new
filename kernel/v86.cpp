#include "v86.h"


extern "C" __declspec(naked) int __V86Proc(){

	__asm {

		mov ax,0
		mov eax,0
		add ax,cx
		add eax,ecx
	}
}