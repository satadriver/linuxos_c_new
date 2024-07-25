#pragma once

#include "def.h"

DWORD getCurrentCr3();

void saveScreen();

void restoreScreen();

int setVideoMode(int mode);

int setGraphMode(int mode);

#ifdef DLL_EXPORT


extern "C" __declspec(dllexport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int cmd);
#else

extern "C" __declspec(dllimport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int cmd);
#endif