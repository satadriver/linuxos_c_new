
#include "satadriver.h"
#include "Utils.h"
#include "video.h"
#include "atapi.h"
#include "hardware.h"
#include "satadriver.h"


unsigned char gAtapiCmdOpen[16] =	{ 0x1b,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0};

unsigned char gAtapiCmdClose[16] =	{ 0x1b,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0 };

unsigned char gAtapiCmdRead[16] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0 };

unsigned char gAtapiCmdWrite[16] = {0xAA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0 };

//2,3,4,5 is bit31-bit24,bit23-bit16,bit15-bit8,bit7-bit0,9 is sector number



int writeAtapiCMD(unsigned short* cmd) {
	__asm {
		mov dx, gAtapiBasePort
		mov ecx, gAtapiPackSize
		shr ecx, 1
		mov esi, cmd
		cld
		rep outsw
	}
	return 0;
}


int checkAtapiPort(WORD port) {
	char param[2048];

	waitFree(port);

	outportb(port, 0xa1);

	waitComplete(port);

	readsector(port - 7, BYTES_PER_SECTOR / 4, param);

	unsigned char szshow[0x1000];
	__dump((char*)param, BYTES_PER_SECTOR, 0, szshow);
	__drawGraphChars((unsigned char*)szshow, 0);
	return TRUE;
}




int atapiCMD(unsigned short *cmd) {

	__asm {
		cli
	}

	waitFree(gAtapiBasePort + 7);

	outportb(gAtapiBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtapiBasePort + 4, gAtapiPackSize );
	outportb(gAtapiBasePort + 5, 0);
	outportb(gAtapiBasePort + 6, gATAPIDev);
	outportb(gAtapiBasePort + 7, 0xa0);

	int res = waitComplete(gAtapiBasePort + 7);

	int low = inportb(gAtapiBasePort + 4);
	int high = inportb(gAtapiBasePort + 5);
	writeAtapiCMD(cmd);

	__asm {
		sti
	}

	return 0;
}


int readAtapiSector(char * buf,unsigned int secnum,unsigned char seccnt) {

	__asm {
		cli
	}

	waitFree(gAtapiBasePort + 7);

	int readsize = ATAPI_SECTOR_SIZE * seccnt;

	outportb(gAtapiBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtapiBasePort + 4, readsize& 0xff);
	outportb(gAtapiBasePort + 5, (readsize >> 8)&0xff);
	outportb(gAtapiBasePort + 6, gATAPIDev);
	outportb(gAtapiBasePort + 7, 0xa0);

	int res = waitComplete(gAtapiBasePort + 7);

	int low = inportb(gAtapiBasePort + 4);
	int high = inportb(gAtapiBasePort + 5);
	gAtapiCmdRead[9] = seccnt;

	gAtapiCmdRead[5] = secnum&0xff;
	gAtapiCmdRead[4] = (secnum>>8) & 0xff;
	gAtapiCmdRead[3] = (secnum >> 16) & 0xff;
	gAtapiCmdRead[2] = (secnum >> 24) & 0xff;
	writeAtapiCMD((unsigned short*)gAtapiCmdRead);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtapiBasePort + 7);
		res = readsector(gAtapiBasePort, ATAPI_SECTOR_SIZE / 4,lpbuf);
		lpbuf += ATAPI_SECTOR_SIZE;
	}

	__asm {
		sti
	}

	return 0;
}


int writeAtapiSector(char* buf, unsigned int secnum, unsigned char seccnt) {

	__asm {
		cli
	}

	waitFree(gAtapiBasePort + 7);

	int readsize = ATAPI_SECTOR_SIZE * seccnt;

	outportb(gAtapiBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtapiBasePort + 4, readsize & 0xff);
	outportb(gAtapiBasePort + 5, (readsize >> 8) & 0xff);
	outportb(gAtapiBasePort + 6, gATAPIDev);
	outportb(gAtapiBasePort + 7, 0xa0);

	int res = waitComplete(gAtapiBasePort + 7);

	int low = inportb(gAtapiBasePort + 4);
	int high = inportb(gAtapiBasePort + 5);
	gAtapiCmdWrite[9] = seccnt;

	gAtapiCmdWrite[5] = secnum & 0xff;
	gAtapiCmdWrite[4] = (secnum >> 8) & 0xff;
	gAtapiCmdWrite[3] = (secnum >> 16) & 0xff;
	gAtapiCmdWrite[2] = (secnum >> 24) & 0xff;
	writeAtapiCMD((unsigned short*)gAtapiCmdWrite);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtapiBasePort + 7);
		res = writesector(gAtapiBasePort, ATAPI_SECTOR_SIZE / 4, lpbuf);
		lpbuf += ATAPI_SECTOR_SIZE;
	}

	__asm {
		sti
	}

	return 0;
}



/*
int writesector(WORD port, char* buf) {
	__asm {
		cld
		mov esi, buf
		mov ecx, ATAPI_SECTOR_SIZE / 4
		mov dx, port
		rep outsd
	}
	return 0;
}

int readsector(WORD port, char* buf) {
	__asm {
		cld
		mov ecx, ATAPI_SECTOR_SIZE / 4
		mov edi, buf
		mov dx, port
		rep insd
	}
	return 0;
}
*/


/*
__asm {
	mov dx, gAtaBasePort + 1
	mov al, 0		//1 is dma,0 is pio
	out dx, al

	mov dx, gAtapiPort + 6
	mov al, 0xa0
	out dx, al

	mov dx, gAtapiPort + 4
	mov al, 12
	out dx, al

	mov dx, gAtapiPort + 5
	mov al, 0
	out dx, al

	mov dx, gAtapiPort+7
	mov al, 0a0h
	out dx, al

	movzx edx, gAtapiPort
	add edx, 7
	push edx
	call waitComplete
	add esp,4
	cmp eax,-1
	jz _atapi_a0_end

	mov dx, gAtapiPort + 4
	in al, dx
	mov cl, al

	mov dx, gAtapiPort + 5
	in al, dx
	mov ch, al

	mov cx, 6
	mov dx, gAtapiPort
	mov esi, cmd
	cld
	rep outsw
	_atapi_a0_end:
}
*/



/*
__asm {
	mov dx, gAtapiPort + 1
	mov al,0
	out dx,al

	mov dx, gAtapiPort + 6
	mov al, 0a0h
	out dx, al

	mov eax, ATAPI_SECTOR_SIZE
	movzx ecx, seccnt
	mul ecx
	mov dx, gAtapiPort + 4
	OUT dx,al
	mov dx, gAtapiPort + 5
	mov al,ah
	OUT dx,al

	mov dx, gAtapiPort + 7
	mov al, 0a0h
	out dx, al

	movzx edx, gAtapiPort
	add edx,7
	push edx
	call waitComplete
	add esp,4
	cmp eax, -1
	jz _atapi_a0_end

	mov dx, gAtapiPort + 4
	in al, dx
	mov cl, al

	mov dx, gAtapiPort + 5
	in al, dx
	mov ch, al

	lea esi, gAtapiCmdRead
	mov al,seccnt
	mov byte ptr [esi + 9], al

	mov eax, secno
	mov byte ptr[esi + 5], al
	mov byte ptr[esi + 4], ah
	shr eax,16
	mov byte ptr[esi + 3], al
	mov byte ptr[esi + 2], ah

	mov ecx, 6
	mov dx, gAtapiPort
	rep outsw

	movzx edx, gAtapiPort
	add edx, 7
	push edx
	call waitComplete
	add esp, 4
	cmp eax, -1
	jz _atapi_a0_end

	movzx edx, gAtapiPort
	mov edi, buf
	movzx ecx,seccnt
	shl ecx,9
	rep insd

	_atapi_a0_end:
}
*/