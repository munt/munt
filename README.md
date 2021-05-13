This repository consists of several related subprojects collected in one place
for convenience.

# [mt32emu](https://github.com/munt/munt/tree/master/mt32emu)

mt32emu is a _C/C++ library_ which allows to emulate (approximately) [the Roland
MT-32, CM-32L and LAPC-I synthesiser
modules](https://en.wikipedia.org/wiki/Roland_MT-32).

# [mt32emu_alsadrv](https://github.com/munt/munt/tree/master/mt32emu_alsadrv)

Comprises of a GUI and a console applications that make use of _mt32emu library_
to provide emulation services via *ALSA MIDI sequencer* interface for Linux
applications. Applications that rely on *raw ALSA MIDI ports* may connect via
virtual raw MIDI ports, e.g. created with help of `snd_virmidi` kernel module.
The GUI application is mostly obsolete and can be replaced with _mt32emu_qt_.
Still, _mt32emu_alsadrv_ may be preferred for systems with limited resources.

# [mt32emu_smf2wav](https://github.com/munt/munt/tree/master/mt32emu_smf2wav)

A console application intended to facilitate conversion a pre-recorded [Standard
MIDI file](https://www.midi.org/specifications-old/item/standard-midi-files-smf)
(SMF) to a WAVE file using the mt32emu library for audio rendering. The output
file is equivalent to a direct recording from a Roland MT-32, CM-32L or LAPC-I
synthesiser module.

# [mt32emu_win32drv](https://github.com/munt/munt/tree/master/mt32emu_win32drv)

Windows MME driver that provides for creating a MIDI output port and
transferring MIDI messages received from an external MIDI program to _mt32emu_qt_,
the main synthesiser application. It also includes the _mt32emu_ engine built-in
and is able to operate in stand-alone mode if the main application _mt32emu_qt_
is unavailable.

# [mt32emu_win32drv_setup](https://github.com/munt/munt/tree/master/mt32emu_win32drv_setup)

Helper tools intended to simplify installation / upgrade of the Windows MME
driver mt32emu_win32drv.

# [mt32emu_qt](https://github.com/munt/munt/tree/master/mt32emu_qt)

The main synthesiser application. It facilitates both realtime synthesis and
conversion of pre-recorded SMF files to WAVE making use of the mt32emu library
and [the Qt framework](https://www.qt.io/). Key features:

1. Support for multiple simultaneous synths with separate state & configuration.
2. GUI to configure synths, manage ROMs, connections to external MIDI ports and
   MIDI programs and interfaces to the host audio systems.
3. Emulates the funny MT-32 LCD. Also displays the internal synth state in
   realtime.
4. Being a cross-platform application, provides support for different operating
   systems and multimedia systems such as Windows multimedia, PulseAudio, JACK
   Audio Connection Kit, ALSA, OSS and CoreMIDI.
5. Contains built-in MIDI player of Standard MIDI files optimised for mt32emu.
6. Makes it easy to record either the MIDI input or the produced audio output.
7. Simplifies batch conversion of collections of SMF files to .wav / .raw audio
   files.

# [DOSBox-mt32-patch](https://github.com/munt/munt/tree/master/DOSBox-mt32-patch)

Patch for the official [DOSBox](https://www.dosbox.com/) release v.0.74 to
demonstrate a possibility to add mt32 MIDI device. Intended for developers and
maintainers of customised DOSBox builds.

# [FreeBSD](https://github.com/munt/munt/tree/master/FreeBSD)

Files related to the port of munt components for [the FreeBSD
system](https://www.freebsd.org/).
