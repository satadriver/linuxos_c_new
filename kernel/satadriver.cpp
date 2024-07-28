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

WORD gAtaBasePort = 0;

DWORD gATADev = 0;

DWORD gAtapiPackSize = 0;

WORD gAtapiBasePort = 0;

WORD gATAPIDev = 0;

DWORD gMimo = 0;

DWORD gSecMax = ONCE_READ_LIMIT;

int(__cdecl* readSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf) = readPortSector;

int(__cdecl* writeSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf) = writePortSector;

//在编译器参数里（项目->属性->配置属性->C/C++->命令行->其他选项）
//说明如下:/Gs 设置堆栈检查字节数. 我们使用/Gs8192设置堆栈检查字节为8MB,经测试,对 __chkstk的调用没有了,成功达到我们的目的.

//1f7读出的是0x58，atapi 170读出的是0x41，含义不同
//1f7
//bit0 之前的命令发生错误 if is 1
//bit1 磁盘每转一周等于1
//bit2 ecc check correctly to read sector data if is 1
//bit3 work complete if is 1
//bit4 磁头停留在磁道上 if is 1
//bit5	write error if 1
//bit6 ready to work if is 1
//bit7 be busy if is 1

//1f6
//bit7:1
//bit6:1 is lba,0 is chs
//bit5:1
//bit4:0 is master,1 is slave
//bit0-bit3:if bit6 is 1,the sector no of 24-27,if bit6 is 0,header number

//柱面（cylinder），磁头（head）,sector
//LBA(逻辑扇区号)=磁头数 × 每磁道扇区数 × 当前所在柱面号 + 每磁道扇区数 × 当前所在磁头号 + 当前所在扇区号 – 1
//CHS=0/0/1，则根据公式LBA=255 × 63 × 0 + 63 × 0 + 1 – 1= 0
//CHS模式支持的硬盘 用8bit来存储磁头地址，用10bit来存储柱面地址，用6bit来存储扇区地址，
//而一个扇区共有512Byte，这样使用CHS寻址一块硬盘最大容量为256 * 1024 * 63 * 512B = 8064 MB
int checkIDEPort(unsigned short port) {

	int r = inportb(port + 7);
	if (r == 0x50) {

		char buffer[0x1000];
		r = identifyDevice(port ,0xa1, buffer);
		if (r) {
			WORD gc = *(WORD*)buffer;
			if (gc & 3 == 1) {
				gAtapiPackSize = 16;
			}
			else if (gc & 3 == 0) {
				gAtapiPackSize = 12;
			}
			return 2;
		}
		else {
			r = identifyDevice(port , 0xec, buffer);
			if (r) {
				return 1;
			}
		}
		return FALSE;
	}
	else {
		return FALSE;
	}
}

int checkIDEMimo(unsigned int addr) {

	char* p = (char*)addr;
	if (p[0] == 0x50) {
		return TRUE;
	}
	return FALSE;
}

int getIDEPort() {

	unsigned char szshow[1024];

	int ret = 0;

	gSecMax = ONCE_READ_LIMIT;
	readSector = readPortSector;
	writeSector = writePortSector;

	ret = checkIDEPort(0x3f0);
	if (ret == 1)
	{
		gAtaBasePort = 0x3f0;	
		gATADev = 0xf0;
	}
	else if (ret == 2) {
		gATAPIDev = 0xf0;
		gAtapiBasePort = 0x3f0;
	}

	ret = checkIDEPort(0x370);
	if (ret==1)
	{
		gATADev = 0xf0;
		gAtaBasePort = 0x370;
	}
	else if (ret == 2) {
		gATAPIDev = 0xf0;
		gAtapiBasePort = 0x370;
	}

	//1f7 = 3f6 = 3f7,376=377=177
	ret = checkIDEPort(0x1f0);
	if (ret == 1)
	{
		gAtaBasePort = 0x1f0;
		gATADev = 0xe0;
	}
	else if (ret == 2) {
		gATAPIDev = 0xe0;
		gAtapiBasePort = 0x1f0;
	}

	ret = checkIDEPort(0x170);
	if (ret == 1)
	{
		gATADev = 0xe0;
		gAtaBasePort = 0x170;
	}
	else if (ret == 2) {
		gATAPIDev = 0xe0;
		gAtapiBasePort = 0x170;
	}

	DWORD hdport[16] ;
	DWORD dev = 0;
	DWORD irq = 0;
	int cnt = getPciDevBasePort(hdport, 0x0101, &dev, &irq);
	for (int i = 0; i < cnt; i++)
	{
		if (hdport[i])
		{
			if (i & 1)
			{
				gATADev = 0xf0;
			}
			else {
				gATADev = 0xe0;
			}

			if ((hdport[i] & 1) == 0)
			{
				gMimo = 1;

				ret = checkIDEMimo((hdport[i] & 0xFFFFfff0 ) );
				if (ret)
				{
					gAtaBasePort = hdport[i] & 0xFFFFfff0;

					ret = checkIDEMimo((hdport[i + 1] & 0xFFFFfff0) );
					if (ret)
					{
						gAtapiBasePort = hdport[i + 1]&0xFFFFfff0;
					}
				}
			}
			else {
				ret = checkIDEPort((hdport[i] & 0xFFFFfff0) );
				if (ret == 1)
				{
					gAtaBasePort = hdport[i] & 0xFFFFfff0;
				}
				else if (ret == 2) {
					gAtapiBasePort = hdport[i] & 0xFFFFfff0;
				}
			}
		}
	}

	if (gAtaBasePort ) {
		__printf((char*)szshow, "ide master port:%x,device:%x,slave port:%d.device:%d\n", gAtaBasePort, gATADev, gAtapiBasePort, gATAPIDev);
	}
	else {
		readSector = vm86ReadSector;
		writeSector = vm86WriteSector;
	}

	return TRUE;
}

//376端口控制说明:
//读端口时跟1f6一致，一般为50h
//写入时得低4位:
//bit0: always be 0
//bit1:1关中断，0开中断
//bit2:1 reset复位磁盘,0不复位磁盘
//bit3:always be 0
//bit4-bit7:读取端口得值跟1f7读取得高4位一致
int __initHardDisk() {

	outportb(0x3f6, 0); //IRQ15
	outportb(0x376, 0);	//IRQ14

	int r = getIDEPort();

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
		ret = readSectorLBA48(secno, secnohigh, gSecMax, offset, gATADev);
		offset += (BYTES_PER_SECTOR * gSecMax);
		secno += gSecMax;
	}

	if (readmod)
	{
		ret = readSectorLBA48(secno, secnohigh, readmod, offset, gATADev);
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
		ret = writeSectorLBA48(secno, secnohigh, gSecMax, offset, gATADev);
		offset += BYTES_PER_SECTOR * gSecMax;
		secno += gSecMax;
	}

	if (readmod)
	{
		ret = writeSectorLBA48(secno, secnohigh, readmod, offset, gATADev);
	}
	return ret;
}



int waitComplete(WORD port) {

	int r = inportb(port - 6);
	if (r == 0) {
		while (1) {
			r = inportb(port);
			if (r & 1) {
				return FALSE;
			}
			else if ((r & 0x58) == 0x58) {
				return TRUE;
			}
			else {
				continue;
			}
		}
	}
	return FALSE;
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




int writesector(int port,int len,char* buf) {
	__asm {
		cld
		mov esi, buf
		mov ecx, len
		mov edx, port
		rep outsd
	}
}

int readsector(int port,int len, char * buf) {
	__asm {
		cld
		mov edi,buf
		mov ecx, len
		mov edx, port
		rep insd
	}
}

int readSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf, int device) {

	__asm{
		cli
	}

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 3, secno & 0xff);
	outportb(gAtaBasePort + 4, (secno>>8) & 0xff);
	outportb(gAtaBasePort + 5,( secno>>16) & 0xff);
	outportb(gAtaBasePort + 6, ((secno >> 24) & 0x0f) + gATADev);

	waitReady(gAtaBasePort + 7);
	outportb(gAtaBasePort + 7, HD_READ_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		readsector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
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

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 3, secno & 0xff);
	outportb(gAtaBasePort + 4, (secno >> 8) & 0xff);
	outportb(gAtaBasePort + 5, (secno >> 16) & 0xff);
	outportb(gAtaBasePort + 6, ((secno >> 24) & 0x0f) + gATADev);

	waitReady(gAtaBasePort + 7);
	outportb(gAtaBasePort + 7, HD_WRITE_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		writesector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
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

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter
	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gAtaBasePort + 4, secnoHigh & 0xff);
	outportb(gAtaBasePort + 3, (secnoLow>>24) & 0xff);

	outportb(gAtaBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gAtaBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gAtaBasePort + 3, secnoLow & 0xff);
	
	outportb(gAtaBasePort + 6, device);

	waitReady(gAtaBasePort + 7);

	outportb(gAtaBasePort + 7, HD_LBA48READ_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		readsector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
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

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter
	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gAtaBasePort + 4, secnoHigh & 0xff);
	outportb(gAtaBasePort + 3, (secnoLow >> 24) & 0xff);

	outportb(gAtaBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gAtaBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gAtaBasePort + 3, secnoLow & 0xff);

	outportb(gAtaBasePort + 6, device);

	waitReady(gAtaBasePort + 7);
	outportb(gAtaBasePort + 7, HD_LBA48WRITE_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		writesector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
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

int identifyDevice(int port,int cmd,char * buffer) {	// IDENTIFY PACKET DEVICE – A1h and  IDENTIFY  DEVICE – ECh
	__asm {
		cli
	}

	waitFree(port + 7);

	outportb(port + 1, 0);	//dma = 1,pio = 0
	outportb(port + 2, 0);
	outportb(port + 3, 0);
	outportb(port + 4, 0);
	outportb(port + 5, 0);
	outportb(port + 6, 0);

	waitReady(port + 7);

	outportb(port + 7, cmd);

	int res = waitComplete(port + 7);
	if (res) {
		readsector(port, BYTES_PER_SECTOR / 4, buffer);
	}

	__asm {
		sti
	}

	//char szout[1024];
	//__printf(szout, "harddisk sequence:%s,firmware version:%s,type:%s,type sequence:%s\r\n",
	//	(char*)HARDDISK_INFO_BASE + 20, (char*)HARDDISK_INFO_BASE + 46, (char*)HARDDISK_INFO_BASE + 54, (char*)HARDDISK_INFO_BASE + 176 * 2);

	unsigned char szshow[0x1000];
	__dump((char*)buffer, BYTES_PER_SECTOR, 0, szshow);
	__drawGraphChars((unsigned char*)szshow, 0);

	return res;
}

//驱动器读取一个扇区后，自动设置状态寄存器1F7H的DRQ数据请求位，并清除BSY位忙信号。 
//DRQ位通知主机现在可以从缓冲区中读取512字节或更多的数据，同时向主机发INTRQ中断请求信号


int readDmaSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf, int device) {
	return 0;
}
