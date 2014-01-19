//
//  MT32EmuAU.h
//  muntAU
//
//  Created by Ivan Safrin on 1/17/14.
//  Copyright (c) 2014 Ivan Safrin. All rights reserved.
//

#include "MusicDeviceBase.h"
#include "mt32emu/mt32emu.h"

#pragma once

class MT32Synth : public MusicDeviceBase {
    public:
        MT32Synth(ComponentInstance inComponentInstance);
        virtual ~MT32Synth();
    
        virtual OSStatus Initialize();
        virtual void Cleanup();
        virtual OSStatus Version();
    
        void sendMIDI(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
    
        virtual OSStatus RestoreState(CFPropertyListRef inData);
    
        virtual bool StreamFormatWritable(	AudioUnitScope scope, AudioUnitElement element);
    
        virtual OSStatus HandleNoteOn(UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame);
        virtual OSStatus HandleNoteOff(	UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame);
    
        virtual OSStatus HandlePitchWheel(UInt8 inChannel, UInt8 inPitch1, UInt8 inPitch2, UInt32 inStartFrame);
    
        virtual OSStatus GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID, CFArrayRef *outStrings);
    
        virtual OSStatus SetParameter(AudioUnitParameterID inID, AudioUnitScope 					inScope, AudioUnitElement inElement, AudioUnitParameterValue			inValue, UInt32	inBufferOffsetInFrames);
    
        virtual UInt32 SupportedNumChannels (const AUChannelInfo** outInfo);
    
        virtual OSStatus GetParameterInfo(AudioUnitScope inScope,
                              AudioUnitParameterID inParameterID,
                              AudioUnitParameterInfo & outParameterInfo);
    
        virtual OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp &inTimeStamp, UInt32 inNumberFrames);
    
        MT32Emu::Synth *synth;
        void *lastBufferData;
    
    private:
    
        AudioConverterRef audioConverterRef;
        CAStreamBasicDescription destFormat;
    
        const MT32Emu::ROMImage *romImage;
        const MT32Emu::ROMImage *pcmRomImage;
    
};

