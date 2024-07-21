#include "satadriver.h"
#include "def.h"
#include "Utils.h"
#include "pci.h"
#include "task.h"
#include "Utils.h"
#include "video.h"
#include "atapi.h"
#include "Kernel.h"

#include "hardware.h"

WORD gHdBasePort = 0;

DWORD gHdBDF = 0;

WORD gCDROMBasePort = 0;

DWORD gMSDev = 0;

DWORD gMimo = 0;

DWORD gAtaIRQ = 0;

DWORD gSecMax = ONCE_READ_LIMIT;

int(__cdecl* readSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf) = readPortSector;

int(__cdecl* writeSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf) = writePortSector;

//�ڱ������������Ŀ->����->��������->C/C++->������->����ѡ�
//˵������:/Gs ���ö�ջ����ֽ���. ����ʹ��/Gs8192���ö�ջ����ֽ�Ϊ8MB,������,�� __chkstk�ĵ���û����,�ɹ��ﵽ���ǵ�Ŀ��.

//1f7��������0x58��atapi 170��������0x41�����岻ͬ
//1f7
//bit0 ֮ǰ����������� if is 1
//bit1 ����ÿתһ�ܵ���1
//bit2 ecc check correctly to read sector data if is 1
//bit3 work complete if is 1
//bit4 ��ͷͣ���ڴŵ��� if is 1
//bit5	write error if 1
//bit6 ready to work if is 1
//bit7 be busy if is 1

//1f6
//bit7:1
//bit6:1 is lba,0 is chs
//bit5:1
//bit4:0 is master,1 is slave
//bit0-bit3:if bit6 is 1,the sector no of 24-27,if bit6 is 0,header number

//���棨cylinder������ͷ��head��,sector
//LBA(�߼�������)=��ͷ�� �� ÿ�ŵ������� �� ��ǰ��������� + ÿ�ŵ������� �� ��ǰ���ڴ�ͷ�� + ��ǰ���������� �C 1
//CHS=0/0/1������ݹ�ʽLBA=255 �� 63 �� 0 + 63 �� 0 + 1 �C 1= 0
//CHSģʽ֧�ֵ�Ӳ�� ��8bit���洢��ͷ��ַ����10bit���洢�����ַ����6bit���洢������ַ��
//��һ����������512Byte������ʹ��CHSѰַһ��Ӳ���������Ϊ256 * 1024 * 63 * 512B = 8064 MB
int testHdPort(unsigned short port) {

	int r = inportb(port);
	if (r == 0x50) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

int testHdPortMimo(unsigned int addr) {

	char* p = (char*)addr;
	if (p[0] == 0x50) {
		return TRUE;
	}
	return FALSE;
}

int getHdPort() {

	unsigned char szshow[1024];

	int ret = 0;

	gSecMax = ONCE_READ_LIMIT;
	readSector = readPortSector;
	writeSector = writePortSector;

	ret = testHdPort(0x3f7);
	if (ret)
	{
		gHdBasePort = 0x3f0;	
		gMSDev = 0xf0;

		__printf((char*)szshow, "get ide hd master port:%x,device:%x\n", gHdBasePort, gMSDev);

		ret = testHdPort(0x377);
		if (ret)
		{
			gCDROMBasePort = 0x370;
			__printf((char*)szshow, "get ide cdrom slave port:%x\n", gCDROMBasePort);
		}

		return TRUE;
	}

	//1f7 = 3f6 = 3f7,376=377=177
	ret = testHdPort(0x1f7);
	if (ret)
	{
		gHdBasePort = 0x1f0;
		gMSDev = 0xe0;

		__printf((char*)szshow, "get ide hd slave port:%x,device:%x\n", gHdBasePort, gMSDev);

		ret = testHdPort(0x177);
		if (ret)
		{
			gCDROMBasePort = 0x170;
			__printf((char*)szshow, "get ide cdrom master port:%x\n", gCDROMBasePort);
		}
		return TRUE;
	}

	DWORD hdport[16] ;
	int cnt = getPciDevBasePort(hdport, 0x0101, &gHdBDF, &gAtaIRQ);
	for (int i = 0; i < cnt; i+=2)
	{
		if (hdport[i])
		{
			if (i & 1)
			{
				gMSDev = 0xf0;
			}
			else {
				gMSDev = 0xe0;
			}

			if ((hdport[i] & 1) == 0)
			{
				gMimo = 1;

				ret = testHdPortMimo((hdport[i] & 0xFFFFfff0 ) + 7);
				if (ret)
				{
					gHdBasePort = hdport[i] & 0xFFFFfff0;

					__printf((char*)szshow, "get sata hd mimo:%x,master_slave:%x\n", gHdBasePort, gMSDev);

					ret = testHdPortMimo((hdport[i + 1] & 0xFFFFfff0) + 7);
					if (ret)
					{
						gCDROMBasePort = hdport[i + 1]&0xFFFFfff0;
						__printf((char*)szshow, "get sata cdrom mimo:%x,master_slave:%x\n", gCDROMBasePort, gMSDev);
					}

					return TRUE;
				}
			}
			else {
				ret = testHdPort((hdport[i] & 0xFFFFfff0) + 7);
				if (ret)
				{
					gHdBasePort = hdport[i] & 0xFFFFfff0;

					__printf((char*)szshow, "get sata hd port:%x,master_slave:%x\n", gHdBasePort, gMSDev);

					ret = testHdPort((hdport[i + 1] & 0xFFFFfff0) + 7);
					if (ret)
					{
						gCDROMBasePort = hdport[i + 1] & 0xFFFFfff0;

						__printf((char*)szshow, "get sata cdrom port:%x,master_slave:%x\n", gCDROMBasePort, gMSDev);
					}

					return TRUE;
				}
			}
		}
	}

	__drawGraphChars((unsigned char*)"int13h\n", 0);
	readSector = vm86ReadSector;
	writeSector = vm86WriteSector;
	return TRUE;
}

//376�˿ڿ���˵��:
//���˿�ʱ��1f6һ�£�һ��Ϊ50h
//д��ʱ�õ�4λ:
//bit0: always be 0
//bit1:1���жϣ�0���ж�
//bit2:1 reset��λ����,0����λ����
//bit3:always be 0
//bit4-bit7:��ȡ�˿ڵ�ֵ��1f7��ȡ�ø�4λһ��
int __initHardDisk() {

	outportb(0x3f6, 0); //IRQ15
	outportb(0x376, 0);	//IRQ14

	int r = getHdPort();

	getHarddiskInfo();

	return r;
}

int vm86ReadBlock(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize) {

	unsigned int counter = 0;

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
		counter++;
		if (counter && (counter % 256 == 0))
		{
			__drawGraphChars((unsigned char*)"bwork is 1,work not start\n", 0);
		}
	}

	params->intno = 0x13;
	params->reax = 0x4200;
	params->recx = 0;
	params->redx = disk;
	params->rebx = 0;
	params->resi = V86VMIDATA_OFFSET;
	params->redi = 0;
	params->res = 0;
	params->rds = V86VMIDATA_SEG;
	params->result = 0;

	LPINT13PAT pat = (LPINT13PAT)V86VMIDATA_ADDRESS;
	pat->len = 0x10;
	pat->reserved = 0;
	pat->seccnt = seccnt;
	pat->segoff = (INT13_RM_FILEBUF_SEG << 16) + INT13_RM_FILEBUF_OFFSET;
	pat->secnolow = secno;
	pat->secnohigh = secnohigh;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
		counter++;
		if (counter && (counter % 256 == 0))
		{
			__drawGraphChars((unsigned char*)"bwork is 1,wait to complete\n", 0);
		}
	}

	if (params->result > 0)
	{
		__memcpy(buf, (char*)INT13_RM_FILEBUF_ADDR, seccnt * sectorsize);
		return seccnt * sectorsize;
	}

	return 0;
}


int vm86WriteBlock(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x13;
	params->reax = 0x4300;
	params->recx = 0;
	params->redx = disk;
	params->rebx = 0;
	params->resi = V86VMIDATA_OFFSET;
	params->redi = 0;
	params->res = 0;
	params->rds = V86VMIDATA_SEG;
	params->result = 0;

	__memcpy((char*)INT13_RM_FILEBUF_ADDR, buf, seccnt * sectorsize);

	LPINT13PAT pat = (LPINT13PAT)V86VMIDATA_ADDRESS;
	pat->len = 0x10;
	pat->reserved = 0;
	pat->seccnt = seccnt;
	pat->segoff = (INT13_RM_FILEBUF_SEG << 16) + INT13_RM_FILEBUF_OFFSET;
	pat->secnolow = secno;
	pat->secnohigh = secnohigh;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}

	if (params->result)
	{
		return seccnt * sectorsize;
	}
	return 0;
}


int vm86ReadSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {

	int readcnt = seccnt / ONCE_READ_LIMIT;
	int readmod = seccnt % ONCE_READ_LIMIT;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
		ret = vm86ReadBlock(secno, secnohigh, ONCE_READ_LIMIT, offset, 0x80, BYTES_PER_SECTOR);

		offset += (BYTES_PER_SECTOR * ONCE_READ_LIMIT);
		secno += ONCE_READ_LIMIT;
	}

	if (readmod)
	{
		ret = vm86ReadBlock(secno, secnohigh, readmod, offset, 0x80, BYTES_PER_SECTOR);
	}
	return ret;
}


int vm86WriteSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {

	int readcnt = seccnt / ONCE_READ_LIMIT;
	int readmod = seccnt % ONCE_READ_LIMIT;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
		ret = vm86WriteBlock(secno, secnohigh, ONCE_READ_LIMIT, offset, 0x80, BYTES_PER_SECTOR);

		offset += BYTES_PER_SECTOR * ONCE_READ_LIMIT;
		secno += ONCE_READ_LIMIT;
	}

	if (readmod)
	{
		ret = vm86WriteBlock(secno, secnohigh, readmod, offset, 0x80, BYTES_PER_SECTOR);
	}
	return ret;
}





int readPortSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {
	int readcnt = seccnt / gSecMax;
	int readmod = seccnt % gSecMax;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
		ret = readSectorLBA48(secno, secnohigh, gSecMax, offset, gMSDev);
		offset += (BYTES_PER_SECTOR * gSecMax);
		secno += gSecMax;
	}

	if (readmod)
	{
		ret = readSectorLBA48(secno, secnohigh, readmod, offset, gMSDev);
	}
	return ret;
}


int writePortSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {
	int readcnt = seccnt / gSecMax;
	int readmod = seccnt % gSecMax;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
		ret = writeSectorLBA48(secno, secnohigh, gSecMax, offset, gMSDev);
		offset += BYTES_PER_SECTOR * gSecMax;
		secno += gSecMax;
	}

	if (readmod)
	{
		ret = writeSectorLBA48(secno, secnohigh, readmod, offset, gMSDev);
	}
	return ret;
}



int waitComplete(WORD port) {

	int r = inportb(port - 6);
	if (r == 0) {
		while (1) {
			r = inportb(port) & 0x58 ;
			if (r == 0x58) {
				return 0;
			}
			else {
				continue;
			}
		}
	}
	return -1;
}

void waitFree(WORD port) {
	while (1)
	{
		int r = inportb(port);
		if (r & 0x80) {
			continue;
		}
		else {
			break;
		}
	}
}



void waitReady(WORD port) {
	while (1)
	{
		int r = inportb(port);
		if (r & 0x40) {
			break;
		}
		else {
			continue;
		}
	}
}




void writesector(char* buf) {
	__asm {
		cld
		mov esi, buf
		mov ecx, BYTES_PER_SECTOR / 4
		mov dx, gHdBasePort
		rep outsd
	}
}

void readsector(char * buf) {
	__asm {
		cld
		mov edi,buf
		mov ecx, BYTES_PER_SECTOR / 4
		mov dx, gHdBasePort
		rep insd
	}
}

int readSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf, int device) {

	__asm{
		cli
	}

	waitFree(gHdBasePort + 7);

	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gHdBasePort + 2, seccnt & 0xff);

	outportb(gHdBasePort + 3, secno & 0xff);
	outportb(gHdBasePort + 4, (secno>>8) & 0xff);
	outportb(gHdBasePort + 5,( secno>>16) & 0xff);
	outportb(gHdBasePort + 6, ((secno >> 24) & 0x0f) + gMSDev);

	waitReady(gHdBasePort + 7);
	outportb(gHdBasePort + 7, HD_READ_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		waitComplete(gHdBasePort + 7);
		readsector(lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	__asm {
		sti
	}

	return seccnt * BYTES_PER_SECTOR;
}



int writeSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf, int device) {

	__asm {
		cli
	}

	waitFree(gHdBasePort + 7);

	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gHdBasePort + 2, seccnt & 0xff);

	outportb(gHdBasePort + 3, secno & 0xff);
	outportb(gHdBasePort + 4, (secno >> 8) & 0xff);
	outportb(gHdBasePort + 5, (secno >> 16) & 0xff);
	outportb(gHdBasePort + 6, ((secno >> 24) & 0x0f) + gMSDev);

	waitReady(gHdBasePort + 7);
	outportb(gHdBasePort + 7, HD_WRITE_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		waitComplete(gHdBasePort + 7);
		writesector(lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	__asm {
		sti
	}

	return seccnt * BYTES_PER_SECTOR;
}


//most 6 bytes sector no
int readSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf, int device) {

	__asm {
		cli
	}

	waitFree(gHdBasePort + 7);

	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gHdBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter
	outportb(gHdBasePort + 2, seccnt & 0xff);

	outportb(gHdBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gHdBasePort + 4, secnoHigh & 0xff);
	outportb(gHdBasePort + 3, (secnoLow>>24) & 0xff);

	outportb(gHdBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gHdBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gHdBasePort + 3, secnoLow & 0xff);
	
	outportb(gHdBasePort + 6, gMSDev);

	waitReady(gHdBasePort + 7);

	outportb(gHdBasePort + 7, HD_LBA48READ_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		waitComplete(gHdBasePort + 7);
		readsector(lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	__asm {
		sti
	}

	return seccnt * BYTES_PER_SECTOR;
}



int writeSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf, int device) {

	__asm {
		cli
	}

	waitFree(gHdBasePort + 7);

	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gHdBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter
	outportb(gHdBasePort + 2, seccnt & 0xff);

	outportb(gHdBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gHdBasePort + 4, secnoHigh & 0xff);
	outportb(gHdBasePort + 3, (secnoLow >> 24) & 0xff);

	outportb(gHdBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gHdBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gHdBasePort + 3, secnoLow & 0xff);

	outportb(gHdBasePort + 6, gMSDev);

	waitReady(gHdBasePort + 7);
	outportb(gHdBasePort + 7, HD_LBA48WRITE_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		waitComplete(gHdBasePort + 7);
		writesector(lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	__asm {
		sti
	}

	return seccnt * BYTES_PER_SECTOR;
}



//bit0:0==memmory address,1== io address
//bit1:size larger than 1MB
//bit2:0 == 32bits address,1 == 64 bits address
//bit4:1== prefetch,0==false
int readSectorLBA24Mimo(unsigned int secno, unsigned char seccnt, char* buf, int device) {
	return 0;
}

int readSectorLBA48Mimo(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf, int device) {
	return 0;
}

int getHarddiskInfo() {

	char szout[1024];

	__asm {
		cli
	}

	waitFree(gHdBasePort + 7);

	outportb(gHdBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gHdBasePort + 2, 0);

	outportb(gHdBasePort + 3, 0);
	outportb(gHdBasePort + 4, 0);
	outportb(gHdBasePort + 5, 0);
	outportb(gHdBasePort + 6, gMSDev);

	waitReady(gHdBasePort + 7);
	outportb(gHdBasePort + 7, 0xec);

	char* lpbuf = (char*) HARDDISK_INFO_BASE;
	waitComplete(gHdBasePort + 7);

	readsector(lpbuf);
	lpbuf += BYTES_PER_SECTOR;

	__asm {
		sti
	}

	__printf(szout, "harddisk sequence:%s,firmware version:%s,type:%s,type sequence:%s\r\n",
		(char*)HARDDISK_INFO_BASE + 20, (char*)HARDDISK_INFO_BASE + 46, (char*)HARDDISK_INFO_BASE + 54, (char*)HARDDISK_INFO_BASE + 176 * 2);

	return BYTES_PER_SECTOR;
}

//��������ȡһ���������Զ�����״̬�Ĵ���1F7H��DRQ��������λ�������BSYλæ�źš� 
//DRQλ֪ͨ�������ڿ��Դӻ������ж�ȡ512�ֽڻ��������ݣ�ͬʱ��������INTRQ�ж������ź�
void __kDriverIntProc() {


}

int readDmaSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf, int device) {
	return 0;
}
