// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <process.h>

#include <iostream>

#include <mt32emu.h>

#if !MT32EMU_IS_COMPATIBLE(2, 8)
#error Incompatible mt32emu library version
#endif

#include "MidiSynth.h"
