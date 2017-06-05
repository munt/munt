/*
     File: AUBaseHelper.cpp 
 Abstract:  AUBaseHelper.h  
  Version: 1.0.4 
  
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple 
 Inc. ("Apple") in consideration of your agreement to the following 
 terms, and your use, installation, modification or redistribution of 
 this Apple software constitutes acceptance of these terms.  If you do 
 not agree with these terms, please do not use, install, modify or 
 redistribute this Apple software. 
  
 In consideration of your agreement to abide by the following terms, and 
 subject to these terms, Apple grants you a personal, non-exclusive 
 license, under Apple's copyrights in this original Apple software (the 
 "Apple Software"), to use, reproduce, modify and redistribute the Apple 
 Software, with or without modifications, in source and/or binary forms; 
 provided that if you redistribute the Apple Software in its entirety and 
 without modifications, you must retain this notice and the following 
 text and disclaimers in all such redistributions of the Apple Software. 
 Neither the name, trademarks, service marks or logos of Apple Inc. may 
 be used to endorse or promote products derived from the Apple Software 
 without specific prior written permission from Apple.  Except as 
 expressly stated in this notice, no other rights or licenses, express or 
 implied, are granted by Apple herein, including but not limited to any 
 patent rights that may be infringed by your derivative works or by other 
 works in which the Apple Software may be incorporated. 
  
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE 
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION 
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND 
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS. 
  
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL 
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, 
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED 
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), 
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE 
 POSSIBILITY OF SUCH DAMAGE. 
  
 Copyright (C) 2013 Apple Inc. All Rights Reserved. 
  
*/
#include "AUBaseHelper.h"

#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
	#include <AudioUnit/AudioUnitProperties.h>
#else
	#include <AudioUnitProperties.h>
#endif

OSStatus	GetFileRefPath (CFDictionaryRef parent, CFStringRef frKey, CFStringRef * fPath)
{	
	static CFStringRef kFRString = CFSTR (kAUPresetExternalFileRefs);
	
	const void* frVal = CFDictionaryGetValue(parent, kFRString);
	if (!frVal) return kAudioUnitErr_InvalidPropertyValue;

	const void* frString = CFDictionaryGetValue ((CFDictionaryRef)frVal, frKey);
	if (!frString) return kAudioUnitErr_InvalidPropertyValue;
		
	if (fPath)
		*fPath = (CFStringRef)frString;
	
	return noErr;
}

// write valid samples check, with bool for zapping

UInt32 FindInvalidSamples(Float32 *inSource, UInt32 inFramesToProcess, bool &outHasNonZero, bool zapInvalidSamples)
{
	float *sourceP = inSource;
	
	UInt32 badSamplesDetected = 0;
	for (UInt32 i=0; i < inFramesToProcess; i++)
	{
		float  input = *sourceP;
		
		if(input > 0) 
			outHasNonZero = true;

		float absx = fabs(input);
		
		// a bad number!
		if (!(absx < 1e15))
		{
			if (!(absx == 0))
			{
				//printf("\tbad sample: %f\n", input);
				badSamplesDetected++;
				if (zapInvalidSamples)
					*sourceP = 0;
			}
		}
        sourceP++;
	}
	
	return badSamplesDetected;
}


CFMutableDictionaryRef CreateFileRefDict (CFStringRef fKey, CFStringRef fPath, CFMutableDictionaryRef fileRefDict)
{
	if (!fileRefDict)
		fileRefDict = CFDictionaryCreateMutable	(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	CFDictionarySetValue (fileRefDict, fKey, fPath);
	
	return fileRefDict;
}

#if DEBUG
//_____________________________________________________________________________
//
void PrintAUParamEvent (AudioUnitParameterEvent& event, FILE* f)
{
		bool isRamp = event.eventType == kParameterEvent_Ramped;
		fprintf (f, "\tParamID=%ld,Scope=%ld,Element=%ld\n", (long)event.parameter, (long)event.scope, (long)event.element);
		fprintf (f, "\tEvent Type:%s,", (isRamp ? "ramp" : "immediate"));
		if (isRamp)
			fprintf (f, "start=%ld,dur=%ld,startValue=%f,endValue=%f\n",
					(long)event.eventValues.ramp.startBufferOffset, (long)event.eventValues.ramp.durationInFrames, 
					event.eventValues.ramp.startValue, event.eventValues.ramp.endValue);
		else
			fprintf (f, "start=%ld,value=%f\n", 
					(long)event.eventValues.immediate.bufferOffset, 
					event.eventValues.immediate.value);
		fprintf (f, "- - - - - - - - - - - - - - - -\n");
}
#endif

