/*
 *	File:		mt32emu_osx_au.cpp
 *	
 *	Version:	1.0
 * 
 *  Created by Stino former ByteGeiZ on 7/27/11
 *
 *	Copyright:  Copyright © 2011 OpenSource, GNU GPLv3
 * 
 *
 */
/*=============================================================================
 mt32emu_osx_au.cpp
 
 =============================================================================*/


#include "mt32emu_osx_au.h"
#include "AUInstrumentBase.h"


#include <unistd.h>
#include <stdio.h>
#include "CoreFoundation/CoreFoundation.h"


//this value scales the volume output 
//use a small value here, if not it is much too loud
#define OUTSCALE 0.0001f;

// 6 channels support not used since OSX seems to init AU allways with 2 channels ?
// maybe there is an API call to activate 6 channels explicitly ?
//#define SUPPORT_6_CHANELS


COMPONENT_ENTRY(mt32emu_osx_au)

#pragma mark mt32emu_osx_au Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::mt32emu_osx_au
//
// This synth has No inputs, One output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mt32emu_osx_au::mt32emu_osx_au(ComponentInstance inComponentInstance)
: AUInstrumentBase(inComponentInstance, 0, 1),
_synth(0),
isOpen(false)

{
	MT32Emu::SynthProperties tmpProp;
	memset(&tmpProp, 0, sizeof(tmpProp));
	
	CreateElements();	
	
	_synth = new MT32Emu::Synth();
	if (!_synth) COMPONENT_THROW(-1);
	
	// force sample rate to 32000 Hz scince the original MT32 use this
	mt32samplerate = 32000;
	
	tmpProp.sampleRate = mt32samplerate;
	tmpProp.useDefaultReverb = true;
	tmpProp.useReverb = true;
	tmpProp.reverbType = 1;
	tmpProp.reverbTime = 5;
	tmpProp.reverbLevel = 3;
	tmpProp.printDebug = &vdebug;
	tmpProp.report = &report;
	
	// lett MUNT also search for MT32 ROM's in the common path
    // when I use baseDir than my bundle path is not longer working, so I comment this out
	//tmpProp.baseDir = "/usr/share/mt32-rom-data";
	
	// change dir to Resurce Path of our Component Bundle for ROMS before init MUNT
	CFBundleRef muntBundle = CFBundleGetBundleWithIdentifier( CFSTR("com.mt32emu.mt32emu_osx_au"));
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(muntBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
	{
		COMPONENT_THROW(-2);
	}
	CFRelease(resourcesURL);
	chdir(path);
	
	// try to init MUNT, if this failes most times the MT32 ROM's are not found
	// we expect them by default in the Resources Folder of out AU component bundel
	if (_synth->open(tmpProp)==0) COMPONENT_THROW(-3);
	
	isOpen = true;
	
	Globals()->UseIndexedParameters (kNumberOfParameters); // we're only defining one param
	Globals()->SetParameter (kGlobalVolumeParam, 0.5);
	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::~mt32emu_osx_au
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mt32emu_osx_au::~mt32emu_osx_au()
{
	if (!isOpen) return;
	_synth->close();
	delete _synth;
	_synth = NULL;
	isOpen=false;	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::Initialize
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus mt32emu_osx_au::Initialize()
{	
	AUInstrumentBase::Initialize();	
	
	return noErr;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::GetParameterInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus mt32emu_osx_au::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
	switch(inParameterID)
	{
		case kGlobalVolumeParam:
			outParameterInfo.flags = SetAudioUnitParameterDisplayType (0, kAudioUnitParameterFlag_DisplaySquareRoot);
			outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
			outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
			
			AUBase::FillInParameterName (outParameterInfo, kGlobalVolumeName, false);
			outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			outParameterInfo.minValue = 0;
			outParameterInfo.maxValue = 1;
			outParameterInfo.defaultValue = 0.5;
			
			return noErr;
			
		default:
			return kAudioUnitErr_InvalidParameter;
	}
	

}

ComponentResult mt32emu_osx_au::RestoreState(CFPropertyListRef plist)
{
	return AUInstrumentBase::RestoreState(plist);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::Render
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// The Sound output, that is created by MT32 synth
ComponentResult	mt32emu_osx_au::Render(AudioUnitRenderActionFlags &	ioActionFlags,
								const AudioTimeStamp &			inTimeStamp,
								UInt32							inNumberFrames)
{
	float *l1, *r1 = 0;
	float scaleVol = Globals()->GetParameter(kGlobalVolumeParam) * OUTSCALE;
	UInt32 len;
	AudioBufferList& outOutputData = GetOutput(0)->GetBufferList();
	
	// buffer for 2 channel variant
	Bit16s *tempbuff = new Bit16s[inNumberFrames << 1]; // "<< 1" is like "* 2" just faster
	Bit16s *t = tempbuff;
		
	int numChans = outOutputData.mNumberBuffers;
	
	switch (numChans)
	{
		case 1:
#if DEBUG_PRINT
			printf("MT32: Mono rendering\n");
#endif
		case 2:
#if DEBUG_PRINT			
			printf("MT32: Stereo rendering\n");
#endif
			// mono and stereo are handled by 2 channel render of MUNT
			l1 = (float*)outOutputData.mBuffers[0].mData;
			if (numChans == 2) r1 = (float*)outOutputData.mBuffers[1].mData;
			
			// call 2 channel render of MUNT (know it is intenal an 6 channel render)
			_synth->render((Bit16s *)tempbuff, inNumberFrames);
		
			len = inNumberFrames;
			while(len>0)
			{
				len--;
				*l1 = (*t) * scaleVol;
				l1++;
				t++;
				t++;
			}
			if (numChans == 2) 
			{ 
				len = inNumberFrames;
				t=tempbuff;
				while(len>0)
				{
					len--;
					t++;
					*r1 = (*t) * scaleVol;
					r1++;
					t++;
				}
			}	
			
			
			break;		
		case 6:
			// curently not supportet
		default:
			return -1;
	}
	
	delete tempbuff;	

	return noErr;
}

// The MIDI input, that we forward to MT32 synth
OSStatus 	mt32emu_osx_au::HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inStartFrame)
{
	unsigned long msg = 0;
	
	msg = channel & 0x0000000F;
	msg = msg + ((status     ) & 0x000000F0); // normaly status needs to shift 4 bits, but status is already shifted
	msg = msg + ((data1 <<  8) & 0x0000FF00);
	msg = msg + ((data2 << 16) & 0x00FF0000);
#if DEBUG_PRINT
	printf("MT32: Midi-Data: %8.8X\n",(unsigned int)msg);
#endif	
	
	
	if (_synth) _synth->playMsg(msg);

	return noErr;
}

// The SysEx input, that we forward to MT32 synth
OSStatus		mt32emu_osx_au::HandleSysEx(		const UInt8 *	inData,	UInt32			inLength )
{ 
#if DEBUG_PRINT
	printf("MT32: SysEx-Data: ");
	for (unsigned long i = 0;i<inLength;i++) printf("%2.2X ",inData[i]);
	printf("\n");
#endif	
	if (inData[0]== 0xf0)
	{
		_synth->playSysex(inData,inLength);
	}
	else
	{
		if (inData[inLength-1]== 0xf7) {
			// only header removed, but footer still exsiting, so we cut off footer as well
			_synth->playSysexWithoutFraming(inData,inLength-1); 
		}
		else
		{
			// allready removed header and footer, so we pass it as it is
			_synth->playSysexWithoutFraming(inData,inLength); 
		}
	}
	return noErr; 
}
