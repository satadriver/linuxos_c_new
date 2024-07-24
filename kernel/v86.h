#pragma once


#ifdef DLL_EXPORT


extern "C" __declspec(dllexport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int cmd);
#else

extern "C" __declspec(dllimport) int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int cmd);
#endif