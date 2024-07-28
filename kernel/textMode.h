#pragma once

#include "def.h"
#include "video.h"

int outputStr(char* str, char color);

int outputChar(char c, char color);

int setCursor(int pos);

extern "C" __declspec(dllexport) int __kTextModeEntry(LPVESAINFORMATION vesa, DWORD fontbase,
	DWORD v86Proc, DWORD v86Addr, DWORD kerneldata, DWORD kernel16, DWORD kernel32);