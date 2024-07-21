#pragma once



void initDevices();
void init8254();
void initCMOS();
void enableMouse();
void init8042();
void init8259();
void enableSpeaker();
void getKeyboardID();

void __wait8042Empty();

void __wait8042Full();

void waitPs2Out();

void waitPs2In();

void __waitPs2Out();

void __waitPs2In();
