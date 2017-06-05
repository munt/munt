//
//  MT32EmuAU.cpp
//  muntAU
//
//  Created by Ivan Safrin on 1/17/14.
//  Copyright (c) 2014 Ivan Safrin. All rights reserved.
//

#include "MT32EmuAU.h"
#include <AudioToolbox/AudioToolbox.h>

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, MT32Synth)


static const AudioUnitParameterID kGlobalVolumeParam = 0;
static const CFStringRef kGlobalVolumeName = CFSTR("Volume");

static const AudioUnitParameterID kInstrumentParam = 1;
static const CFStringRef kInstrumentName = CFSTR("Instrument");

static const AudioUnitParameterID kReverbEnabledParam = 2;
static const CFStringRef kReverbEnabledParamName = CFSTR("Reverb enabled");

static const AudioUnitParameterID kReverbGainParam = 3;
static const CFStringRef kReverbGainParamName = CFSTR("Reverb gain");


MT32Synth::MT32Synth(ComponentInstance inComponentInstance)
: MusicDeviceBase(inComponentInstance, 0, 8) {
    CreateElements();
    
    Globals()->SetParameter(kGlobalVolumeParam, 2.111);
    Globals()->SetParameter(kInstrumentParam, 0.0);
    Globals()->SetParameter(kReverbEnabledParam, 1.0);
    Globals()->SetParameter(kReverbGainParam, 1.0);
    
    synth = NULL;
    lastBufferData = NULL;
}

MT32Synth::~MT32Synth() {
    
}

OSStatus MT32Synth::Version()
{
    return 0xFFFFFFFF;
}

OSStatus MT32Synth::Initialize() {
    
    destFormat = GetStreamFormat(kAudioUnitScope_Output, 0);
    
    AudioStreamBasicDescription sourceDescription;
    sourceDescription.mSampleRate = 32000;
    sourceDescription.mBytesPerFrame = 4;
    sourceDescription.mBitsPerChannel = 16;
    sourceDescription.mFormatID = kAudioFormatLinearPCM;
    sourceDescription.mBytesPerPacket = 4;
    sourceDescription.mChannelsPerFrame = 2;
    sourceDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger;
    sourceDescription.mFramesPerPacket = 1;
    sourceDescription.mReserved = 0;
    
    AudioConverterNew(&sourceDescription, &destFormat, &audioConverterRef);
    MT32Emu::FileStream controlROMFile;
    MT32Emu::FileStream pcmROMFile;
    
    if(!controlROMFile.open("/Library/MT32/MT32_CONTROL.ROM")) {
        printf("Error opening MT32_CONTROL.ROM\n");
    }
    
    if(!pcmROMFile.open("/Library/MT32/MT32_PCM.ROM")) {
        printf("Error opening MT32_PCM.ROM\n");
    }
    
    romImage = MT32Emu::ROMImage::makeROMImage(&controlROMFile);
    pcmRomImage = MT32Emu::ROMImage::makeROMImage(&pcmROMFile);
    
    synth = new MT32Emu::Synth();
    synth->open(*romImage, *pcmRomImage);
    
    MT32Emu::ROMImage::freeROMImage(romImage);
    MT32Emu::ROMImage::freeROMImage(pcmRomImage);
    
    sendMIDI(0xC1, 0x00, 0x00, 0x00);
    MusicDeviceBase::Initialize();
    
    synth->setOutputGain(2.0);
    
    return noErr;
}

void MT32Synth::sendMIDI(unsigned char b0, unsigned char b1, unsigned char b2,unsigned char b3) {
    unsigned int msg = 0;
    msg |= ( b3 << 24);
    msg |= ( b2 << 16);
    msg |= ( b1 <<  8);
    msg |= ( b0);
    if(synth) {
        synth->playMsg(msg);
    }
}

UInt32 MT32Synth::SupportedNumChannels (const AUChannelInfo** outInfo) {
    static const AUChannelInfo sChannels[2] = { {0, 1}, {0, 2} };
    if (outInfo) *outInfo = sChannels;
    return sizeof (sChannels) / sizeof (AUChannelInfo);
}

bool MT32Synth::StreamFormatWritable(	AudioUnitScope scope, AudioUnitElement element) {
        return true;
}

OSStatus MT32Synth::HandleNoteOn(UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame) {
    sendMIDI(0x91, inNoteNumber, inVelocity, 0x00);
    return MusicDeviceBase::HandleNoteOn(inChannel, inNoteNumber, inVelocity, inStartFrame);
}

OSStatus MT32Synth::HandlePitchWheel(UInt8 inChannel, UInt8 inPitch1, UInt8 inPitch2, UInt32 inStartFrame) {
    sendMIDI(0xE1, inPitch1, inPitch2, inStartFrame);
    return MusicDeviceBase::HandlePitchWheel(inChannel, inPitch1, inPitch2, inStartFrame);
}

OSStatus MT32Synth::HandleNoteOff(	UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame) {
    sendMIDI(0x81, inNoteNumber, inVelocity, 0x00);
    return MusicDeviceBase::HandleNoteOff(inChannel, inNoteNumber, inVelocity, inStartFrame);
}

static OSStatus EncoderDataProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
    MT32Synth *_this = (MT32Synth*) inUserData;

    if(_this->lastBufferData) {
        free(_this->lastBufferData);
    }
    
    unsigned int amountToWrite = *ioNumberDataPackets;

    unsigned int dataSize = amountToWrite * sizeof(MT32Emu::Bit16s) * 2;
    MT32Emu::Bit16s *data = (MT32Emu::Bit16s*) malloc(dataSize);
    _this->synth->render(data, amountToWrite);
    
    ioData->mNumberBuffers = 1;
    ioData->mBuffers[0].mData = data;
    ioData->mBuffers[0].mDataByteSize = dataSize;
    _this->lastBufferData = data;
    
    return noErr;
}

OSStatus MT32Synth::Render(AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp &inTimeStamp, UInt32 inNumberFrames) {

    if(!synth) {
        return noErr;
    }
    
    AUOutputElement* outputBus = GetOutput(0);
	outputBus->PrepareBuffer(inNumberFrames);
	
	AudioBufferList& outputBufList = outputBus->GetBufferList();
	AUBufferList::ZeroBuffer(outputBufList);

    UInt32 ioOutputDataPackets = inNumberFrames * destFormat.mFramesPerPacket;
    AudioConverterFillComplexBuffer(audioConverterRef, EncoderDataProc, (void*) this, &ioOutputDataPackets, &outputBufList, NULL);

    return noErr;
}

void MT32Synth::Cleanup() {
    synth->close();
    delete synth;
    MusicDeviceBase::Cleanup();
}

OSStatus MT32Synth::SetParameter(AudioUnitParameterID inID, AudioUnitScope 	inScope, AudioUnitElement inElement, AudioUnitParameterValue	inValue, UInt32	inBufferOffsetInFrames) {
    
    if(inID == kGlobalVolumeParam) {
        if(synth){
            synth->setOutputGain(inValue);
        }
    } else  if(inID == kInstrumentParam) {
        sendMIDI(0xC1, inValue, 0x00, 0x00);
    } else if(inID == kReverbEnabledParam) {
        if(synth){
            synth->setReverbEnabled(inValue == 1.0);
        }
    } else if(inID == kReverbGainParam) {
        if(synth){
            synth->setReverbOutputGain(inValue);
        }
    }
    return MusicDeviceBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
}

OSStatus MT32Synth::GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID, CFArrayRef *outStrings) {

    if (outStrings == NULL) return noErr;
    
    CFStringRef  strings [] = {
        CFSTR("001 Acou Piano 1"),
        CFSTR("002 Acou Piano 2"),
        CFSTR("003 Acou Piano 3"),
        CFSTR("004 Elec Piano 1"),
        CFSTR("005 Elec Piano 2"),
        CFSTR("006 Elec Piano 3"),
        CFSTR("007 Elec Piano 4"),
        CFSTR("008 Honkytonk"),
        CFSTR("009 Elec Org 1"),
        CFSTR("010 Elec Org 2"),
        CFSTR("011 Elec Org 3"),
        CFSTR("012 Elec Org 4"),
        CFSTR("013 Pipe Org 1"),
        CFSTR("014 Pipe Org 2"),
        CFSTR("015 Pipe Org 3"),
        CFSTR("016 Accordion"),
        CFSTR("017 Harpsi 1"),
        CFSTR("018 Harpsi 2"),
        CFSTR("019 Harpsi 3"),
        CFSTR("020 Clavi 1"),
        CFSTR("021 Clavi 2"),
        CFSTR("022 Clavi 3"),
        CFSTR("023 Celesta 1"),
        CFSTR("024 Celesta 2"),
        CFSTR("025 Syn Brass 1"),
        CFSTR("026 Syn Brass 2"),
        CFSTR("027 Syn Brass 3"),
        CFSTR("028 Syn Brass 4"),
        CFSTR("029 Syn Bass 1"),
        CFSTR("030 Syn Bass 2"),
        CFSTR("031 Syn Bass 3"),
        CFSTR("032 Syn Bass 4"),
        CFSTR("033 Fantasy"),
        CFSTR("034 Harmo Pan"),
        CFSTR("035 Chorale"),
        CFSTR("036 Glasses"),
        CFSTR("037 Soundtrack"),
        CFSTR("038 Atmosphere"),
        CFSTR("039 Warm Bell"),
        CFSTR("040 Funny Vox"),
        CFSTR("041 Echo Bell"),
        CFSTR("042 Ice Rain"),
        CFSTR("043 Oboe 2001"),
        CFSTR("044 Echo Pan"),
        CFSTR("045 Doctor Solo"),
        CFSTR("046 Schooldaze"),
        CFSTR("047 Bell Singer"),
        CFSTR("048 Square Wave"),
        CFSTR("049 Str Sect 1"),
        CFSTR("050 Str Sect 2"),
        CFSTR("051 Str Sect 3"),
        CFSTR("052 Pizzicato"),
        CFSTR("053 Violin 1"),
        CFSTR("054 Violin 2"),
        CFSTR("055 Cello 1"),
        CFSTR("056 Cello 2"),
        CFSTR("057 Contrabass"),
        CFSTR("058 Harp 1"),
        CFSTR("059 Harp 2"),
        CFSTR("060 Guitar 1"),
        CFSTR("061 Guitar 2"),
        CFSTR("062 Elec Gtr 1"),
        CFSTR("063 Elec Gtr 2"),
        CFSTR("064 Sitar"),
        CFSTR("065 Acou Bass 1"),
        CFSTR("066 Acou Bass 2"),
        CFSTR("067 Elec Bass 1"),
        CFSTR("068 Elec Bass 2"),
        CFSTR("069 Slap Bass 1"),
        CFSTR("070 Slap Bass 2"),
        CFSTR("071 Fretless 1"),
        CFSTR("072 Fretless 2"),
        CFSTR("073 Flute 1"),
        CFSTR("074 Flute 2"),
        CFSTR("075 Piccolo 1"),
        CFSTR("076 Piccolo 2"),
        CFSTR("077 Recorder"),
        CFSTR("078 Pan Pipes"),
        CFSTR("079 Sax 1"),
        CFSTR("080 Sax 2"),
        CFSTR("081 Sax 3"),
        CFSTR("082 Sax 4"),
        CFSTR("083 Clarinet 1"),
        CFSTR("084 Clarinet 2"),
        CFSTR("085 Oboe"),
        CFSTR("086 Engl Horn"),
        CFSTR("087 Bassoon"),
        CFSTR("088 Harmonica"),
        CFSTR("089 Trumpet 1"),
        CFSTR("090 Trumpet 2"),
        CFSTR("091 Trombone 1"),
        CFSTR("092 Trombone 2"),
        CFSTR("093 Fr Horn 1"),
        CFSTR("094 Fr Horn 2"),
        CFSTR("095 Tuba"),
        CFSTR("096 Brs Sect 1"),
        CFSTR("097 Brs Sect 2"),
        CFSTR("098 Vibe 1"),
        CFSTR("099 Vibe 2"),
        CFSTR("100 Syn Mallet"),
        CFSTR("101 Wind Bell"),
        CFSTR("102 Glock"),
        CFSTR("103 Tube Bell"),
        CFSTR("104 Xylophone"),
        CFSTR("105 Marimba"),
        CFSTR("106 Koto"),
        CFSTR("107 Sho"),
        CFSTR("108 Shakuhachi"),
        CFSTR("109 Whistle 1"),
        CFSTR("110 Whistle 2"),
        CFSTR("111 Bottle Blow"),
        CFSTR("112 Breathpipe"),
        CFSTR("113 Timpani"),
        CFSTR("114 Melodic Tom"),
        CFSTR("115 Deep Snare"),
        CFSTR("116 Elec Perc 1"),
        CFSTR("117 Elec Perc 2"),
        CFSTR("118 Taiko"),
        CFSTR("119 Taiko Rim"),
        CFSTR("120 Cymbal"),
        CFSTR("121 Castanets"),
        CFSTR("122 Triangle"),
        CFSTR("123 Orche Hit"),
        CFSTR("124 Telephone"),
        CFSTR("125 Bird Tweet"),
        CFSTR("126 One Note Jam"),
        CFSTR("127 Water Bells"),
        CFSTR("128 Jungle Tune")
            };

    *outStrings = CFArrayCreate (
                                 NULL,
                                 (const void **) strings,
                                 (sizeof (strings) / sizeof (strings [0])),
                                 NULL
                                 );
    
    return noErr;
}

OSStatus MT32Synth::RestoreState(CFPropertyListRef inData) {
    MusicDeviceBase::RestoreState(inData);
    synth->setOutputGain(Globals()->GetParameter(kGlobalVolumeParam));
    synth->setReverbOutputGain(Globals()->GetParameter(kReverbGainParam));
    synth->setReverbEnabled(Globals()->GetParameter(kReverbEnabledParam) == 1.0);
    sendMIDI(0xC1, Globals()->GetParameter(kInstrumentParam), 0x00, 0x00);
    return noErr;
}

OSStatus MT32Synth::GetParameterInfo(AudioUnitScope inScope,
                                     AudioUnitParameterID inParameterID,
                                     AudioUnitParameterInfo & outParameterInfo)
{

    if (inParameterID == kGlobalVolumeParam) {
        if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
        
        outParameterInfo.flags = SetAudioUnitParameterDisplayType (0, kAudioUnitParameterFlag_DisplaySquareRoot);
        outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
        outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
        
        AUBase::FillInParameterName (outParameterInfo, kGlobalVolumeName, false);
        outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
        outParameterInfo.minValue = 0;
        outParameterInfo.maxValue = 5.0;
        outParameterInfo.defaultValue = 1.0;
        return noErr;
    } else if (inParameterID == kInstrumentParam) {
        if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
        
        outParameterInfo.flags = kAudioUnitParameterFlag_ValuesHaveStrings;
        outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
        outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
        
        AUBase::FillInParameterName (outParameterInfo, kInstrumentName, false);
        outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
        outParameterInfo.minValue = 0;
        outParameterInfo.maxValue = 127.0;
        outParameterInfo.defaultValue = 0.0;        
        
        return noErr;
    } else if (inParameterID == kReverbEnabledParam) {
        if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
        
        outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
        outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
        
        AUBase::FillInParameterName (outParameterInfo, kReverbEnabledParamName, false);
        outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
        outParameterInfo.minValue = 0;
        outParameterInfo.maxValue = 1.0;
        outParameterInfo.defaultValue = 1.0;
        
        return noErr;
    } else if (inParameterID == kReverbGainParam) {
        if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
        
        outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
        outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
        
        AUBase::FillInParameterName (outParameterInfo, kReverbGainParamName, false);
        outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
        outParameterInfo.minValue = 0;
        outParameterInfo.maxValue = 5.0;
        outParameterInfo.defaultValue = 1.0;
        
        return noErr;
    } else {
        return noErr;
    }
}

