/*
 *	File:		mt32emu_osx_au.r
 *	
 *	Version:	1.0
 * 
 *  Created by Stino former ByteGeiZ on 7/27/11
 *	
 *	Copyright:  Copyright © 2011 OpenSource, GNU GPLv3
 * 
 *
 */
#include <AudioUnit/AudioUnit.r>

#include "mt32emu_osx_auVersion.h"

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define kAudioUnitResID_mt32emu_osx_au				1000

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ mt32emu_osx_au~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define RES_ID			kAudioUnitResID_mt32emu_osx_au
#define COMP_TYPE		kAudioUnitType_MusicDevice
#define COMP_SUBTYPE	mt32emu_osx_au_COMP_SUBTYPE
#define COMP_MANUF		mt32emu_osx_au_COMP_MANF	

#define VERSION			kmt32emu_osx_auVersion
#define NAME			"mt32emu: mt32emu_osx_au"
#define DESCRIPTION		"mt32emu_osx_au AU"
#define ENTRY_POINT		"mt32emu_osx_auEntry"

#include "AUResources.r"
