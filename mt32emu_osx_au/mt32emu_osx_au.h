/*
 *  mt32emu_osx_au.h
 *  mt32emu_osx_au
 *
 *  Created by Stino former ByteGeiZ on 7/27/11
 *
 *  Copyright 2011 OpenSource. GNU GPLv3.
 *
 */

#include "mt32emu.h"

#include "mt32emu_osx_auVersion.h"
#include "AUInstrumentBase.h"


// #define DEBUG_PRINT 1 


static void vdebug(void *userData, const char *fmt, va_list list) {
	char message[1024];
	
	vsprintf(message, fmt, list);
#if DEBUG_PRINT
	printf("MT32: %s\n",message);
#endif
}

static int report(void *userData, MT32Emu::ReportType type, const void *reportData) {
	switch(type) {
		case MT32Emu::ReportType_errorControlROM:
#if DEBUG_PRINT
			printf("MT32: Couldn't find control files\n");
#endif
			break;
		case MT32Emu::ReportType_errorPCMROM:
#if DEBUG_PRINT
			printf("MT32: Couldn't open MT32_PCM.ROM file\n");
#endif
			break;
		default:
#if DEBUG_PRINT
			printf("MT32: Report %d\n",type);
#endif						
			break;
	}
	return 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


enum {
	kGlobalVolumeParam = 0,
	//Add your parameters here...
	kNumberOfParameters=1
};

static const CFStringRef kGlobalVolumeName = CFSTR("global volume");

typedef   signed short int Bit16s;

class mt32emu_osx_au : public AUInstrumentBase
{
public:
	mt32emu_osx_au(ComponentInstance inComponentInstance);
	virtual						~mt32emu_osx_au();
	ComponentResult				RestoreState(	CFPropertyListRef	plist);
	
	virtual ComponentResult		Render(AudioUnitRenderActionFlags &	ioActionFlags,
									   const AudioTimeStamp &			inTimeStamp,
									   UInt32							inNumberFrames);
	
	
	virtual OSStatus			Initialize();
	virtual OSStatus			Version() { return kmt32emu_osx_auVersion; }
	
	virtual OSStatus			GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo);
	
	virtual OSStatus			HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inStartFrame);
	virtual OSStatus			HandleSysEx(		const UInt8 *	inData,	UInt32			inLength );
private:
	
	
	MT32Emu::Synth *_synth;
	bool isOpen;
	int mt32samplerate;
	
};
