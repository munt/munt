mt32emu
=======
mt32emu is a C++ static link library which provides classes to approximately emulate
the Roland MT-32, CM-32L and LAPC-I synthesiser modules.

mt32emu_alsadrv
===============
ALSA MIDI driver uses mt32emu to provide ALSA MIDI interface for Linux applications (now obsolete).

DOSBox-mt32-patch
=================
Patch for official DOSBox release v.0.74 to demonstrate a possibility to add mt32 MIDI device.

mt32emu_smf2wav
===============
mt32emu-smf2wav makes use of mt32emu to produce a WAVE file from an SMF file.
The output file corresponds a digital recording from a Roland MT-32, CM-32L and LAPC-I
synthesiser module.

mt32emu_win32drv
================
Windows driver that provides for creating MIDI output port and transferring MIDI messages
to mt32emu_qt, the main UI-enabled synthesiser application. It also contains mt32emu engine
and is able to operate in stand-alone mode if the main application mt32emu_qt is unavailable.

mt32emu_win32drv_setup
======================
A tool intended to fascilate installation / upgrade of the Windows driver mt32emu_win32drv.

portaudio
=========
Portable Real-Time Audio Library, the main UI-enabled synthesiser application mt32emu_qt depends on.

mt32emu_qt
==========
Main synthesiser application. It makes use of mt32emu and Qt to provide for:

1) Multiple simultaneous synths, GUI to configure synths, manage ROMs and connections
2) Funny LCD
3) Easy usage in different operating system environments:
   Windows multimedia, PulseAudio, ALSA, OSS and CoreMIDI supported
4) Play and record Standard MIDI files
5) Perform batch conversion of Standard MIDI files directly to .wav / .raw audio files

FreeBSD
=======
Files related to FreeBSD port of munt.
