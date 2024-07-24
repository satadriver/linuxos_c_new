
#include "def.h"
#include "device.h"
#include "hardware.h"
#include "Utils.h"
#include "keyboard.h"

#define PS2_COMMAND_PORT	0x64
#define PS2_DATA_PORT		0x60

#define TIMER_COMMAND_REG	0X43

#define CMOS_NUM_PORT		0X70
#define CMOS_DATA_PORT		0X71




void waitPs2Out() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while ((status & 1) == 0);
}

void waitPs2In() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while (status & 2);
}

void __waitPs2Out() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while ((status & 1) == 0);
}

void __waitPs2In() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while (status & 2);
}

void __wait8042Full() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while ((status & 1) == 0);
}

void __wait8042Empty() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while (status & 2);
}


void initDevices() {

	init8259();
	init8254();
	initCMOS();
	enableMouse();
	init8042();
	enableSpeaker();
	getKeyboardID();

}


void enableMouse() {
	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xa8);

	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();

	outportb(PS2_DATA_PORT, 0xf4);
}


/*
61h NMI Status and Control Register
bit3:IOCHK NMI Enable(INE) : When set, IOCHK# NMIs are disabledand cleared.When cleared, IOCHK# NMIs are enabled.
bit2:SERR# NMI Enable(SNE) : When set, SERR# NMIs are disabledand cleared.When cleared, SERR# NMIs are enabled.
bit1:Speaker Data Enable(SDE) : When this bit is a 0, the SPKR output is a 0.
When this bit is a 1, the SPKR output is equivalent to the Counter 2 OUT signal value.
bit 0:Timer Counter 2 Enable(TC2E) : When cleared, counter 2 counting is disabled.When set, counting is enabled.
*/
void enableSpeaker() {

	outportb(0x61, 3);
}


//d6 d7 select timer, 00 = 40h, 01 = 41h, 02 = 42h
//d4 d5 mode :11 read read / write low byte first, than read / write high byte
//d1 d2 d3 select work mode
//d0 bcd or binary, 0 = binary, 1 = bcd
void init8254() {

	outportb(TIMER_COMMAND_REG, 0X36);
	outportb(0x40, SYSTEM_TIMER0_FACTOR & 0xff);
	outportb(0x40, (SYSTEM_TIMER0_FACTOR >> 8)&0xff);

	outportb(TIMER_COMMAND_REG, 0X76);
	outportb(0x41, 0);
	outportb(0x41, 0);

	outportb(TIMER_COMMAND_REG, 0Xb6);
	outportb(0x42, 0);
	outportb(0x42, 0);
}

int readTimer(int num) {

	int cmd = (num << 6) + 6;
	outportb(TIMER_COMMAND_REG, cmd);
	int low = inportb(0x40);
	int high = inportb(0x40);
	return low + (high << 8);
}

//https://www.cnblogs.com/LinKArftc/p/5735627.html
void init8042() {

	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xad);

	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0x60);

	__wait8042Empty();

	outportb(PS2_DATA_PORT, 0X47);

	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xae);

}

//https://www.cnblogs.com/LinKArftc/p/5735627.html
//83ABh
void getKeyboardID() {

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0Xf2);

	__wait8042Full();
	unsigned char ack = inportw(PS2_DATA_PORT);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0x20);

	__wait8042Full();
	unsigned char low = inportw(PS2_DATA_PORT);

	__wait8042Full();
	unsigned char high = inportw(PS2_DATA_PORT);

	gKeyboardID = (high << 8) | low;

	char szout[1024];
	__printf(szout, "keyboardid:%x\r\n", gKeyboardID);
}


/*
0001 = 3.90625 ms
0010 = 7.8125 ms
0011 = 122.070 µs
0100 = 244.141 µs
0101 = 488.281 µs
0110 = 976.5625 µs
0111 = 1.953125 ms
1000 = 3.90625 ms
1001 = 7.8125 ms
1010 = 15.625 ms
1011 = 31.25 ms
1100 = 62.5 ms
1101 = 125 ms
1110 = 250 ms
1111= 500 ms
*/
void initCMOS() {

	outportb(CMOS_NUM_PORT, 0X0A);
	outportb(CMOS_DATA_PORT, 0XAA);

	outportb(CMOS_NUM_PORT, 0X0B);
	outportb(CMOS_DATA_PORT, 0X7A);

	outportb(CMOS_NUM_PORT, 0X0D);
	outportb(CMOS_DATA_PORT, 0);
}



void init8259() {

	outportb(0x20, 0x11);
	outportb(0xa0, 0x11);
	outportb(0x21, INTR_8259_MASTER);
	outportb(0xa1, INTR_8259_SLAVE);
	outportb(0x21, 4);
	outportb(0xa1, 2);
	outportb(0x21, 0x1);
	outportb(0xa1, 0x1);

	outportb(0x20, 0x00);
	outportb(0xa0, 0x00);

	//0: level trigger,1: pulse trigger
	outportb(0x4d0, 0);
	outportb(0x4d1, 0);
}