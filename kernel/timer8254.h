#pragma once

#include "def.h"

#define DOS_SYSTIMER_ADDR		0X46C



extern int g_timeslip;

void systimerProc();