#include <AudioUnit/AudioUnit.r>

#define RES_ID    1000
#define COMP_TYPE 'aumu'
#define COMP_SUBTYPE 'MT32'
#define COMP_MANUF 'MUNT'
#define VERSION 0xFFFFFFFF
#define NAME "MUNT: MT-32 Emulator"
#define DESCRIPTION "Roland MT-32 emulator"
#define ENTRY_POINT "MT32SynthEntry"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 // AUResources.r
 //
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* sample macro definitions -- all of these symbols must be defined
 #define RES_ID kHALOutputResID
 #define COMP_TYPE kAudioUnitComponentType
 #define COMP_SUBTYPE kAudioUnitOutputSubType
 #define COMP_MANUF kAudioUnitAudioHardwareOutputSubSubType
 #define VERSION 0x00010000
 #define NAME "AudioHALOutput"
 #define DESCRIPTION "Audio hardware output AudioUnit"
 #define ENTRY_POINT "AUHALEntry"
 */
#define UseExtendedThingResource 1

#include <CoreServices/CoreServices.r>

// this is a define used to indicate that a component has no static data that would mean
// that no more than one instance could be open at a time - never been true for AUs
#ifndef cmpThreadSafeOnMac
#define cmpThreadSafeOnMac 0x10000000
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

resource 'STR ' (RES_ID, purgeable) {
    NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
    DESCRIPTION
};

resource 'dlle' (RES_ID) {
    ENTRY_POINT
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

resource 'thng' (RES_ID, NAME) {
    COMP_TYPE,
    COMP_SUBTYPE,
    COMP_MANUF,
    0, 0, 0, 0, // no 68K
    'STR ', RES_ID,
    'STR ', RES_ID + 1,
    0, 0, /* icon */
    VERSION,
    componentHasMultiplePlatforms | componentDoAutoVersion,
    0,
    {
        #if defined(ppc_YES)
        cmpThreadSafeOnMac,
        'dlle', RES_ID, platformPowerPCNativeEntryPoint
        #define NeedLeadingComma 1
        #endif
        #if defined(ppc64_YES)
        #if defined(NeedLeadingComma)
        ,
        #endif
        cmpThreadSafeOnMac,
        'dlle', RES_ID, platformPowerPC64NativeEntryPoint
        #define NeedLeadingComma 1
        #endif
        #if defined(i386_YES)
        #if defined(NeedLeadingComma)
        ,
        #endif
        cmpThreadSafeOnMac,
        'dlle', RES_ID, platformIA32NativeEntryPoint
        #define NeedLeadingComma 1
        #endif
        #if defined(x86_64_YES)
        #if defined(NeedLeadingComma)
        ,
        #endif
        cmpThreadSafeOnMac,
        'dlle', RES_ID, 8
        #define NeedLeadingComma 1
        #endif
    }
};

#undef RES_ID
#undef COMP_TYPE
#undef COMP_SUBTYPE
#undef COMP_MANUF
#undef VERSION
#undef NAME
#undef DESCRIPTION
#undef ENTRY_POINT
#undef NeedLeadingComma
