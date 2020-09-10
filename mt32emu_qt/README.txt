Munt mt32emu-qt
===============

mt32emu-qt makes use of libmt32emu and Qt to provide for:

1) Multiple simultaneous synths, GUI to configure synths, manage ROMs and connections
2) Funny LCD
3) Easy usage in different operating system environments:
   Windows multimedia, PulseAudio, JACK, ALSA, OSS and CoreMIDI supported
4) Play and record Standard MIDI files
5) Perform batch conversion of Standard MIDI files directly to .wav / .raw audio files


MIDI Support
============

mt32emu-qt can communicate with other MIDI applications using virtual MIDI ports which depends on the operating system.
Currently, supported MIDI systems are: Windows Multimedia, CoreMIDI, ALSA MIDI sequencer, ALSA & OSS4 raw MIDI ports and JACK MIDI.
In order to use this feature, the client MIDI applications need to be set up correctly, as well as some more steps may be
necessary on particular systems.

Because CoreMIDI and ALSA MIDI sequencer support virtual MIDI ports natively, these systems seem simplest to use.
As mt32emu-qt starts, it creates a virtual MIDI port and listens for incoming connections. Contrary to that, Windows Multimedia
requires the dedicated MIDI driver to be setup in the system and mt32emu-qt instead communicates with the driver as a proxy.
With JACK MIDI, MIDI ports can also be created on the fly, albeit the user does this explicitly via the corresponding commands
in "Tools" menu. Moreover, a whole synth instance can be wrapped into a single JACK client with one MIDI input port and
a couple of output audio ports, so that MIDI and audio processing is performed without explicitly added latency.

Some brief notes follow below related to each MIDI system.

1) Windows Multimedia
   If a system happens to have a hardware MIDI-in port or an existing virtual loopback MIDI port driver, mt32emu-qt readily
   supports this configuration and should be able to open any MIDI-in port in the system by using '+' button of "MIDI Input" panel
   or "New MIDI port..." item in "Tools" menu.
   When there is no MIDI-in port available, the dedicated MIDI driver can be used to forward MIDI messages to mt32emu-qt
   for playback. This is done automatically by the driver when it detects a running instance of mt32emu-qt.

2) CoreMIDI
   Upon startup, mt32emu-qt creates destination "Mt32EmuPort" and listens for connections. The user can create
   more destinations by using '+' button of "MIDI Input" panel or "New MIDI port..." item in "Tools" menu.

3) ALSA MIDI sequencer
   Upon startup, mt32emu-qt allocates a virtual MIDI port with the first available address. This address is displayed
   in the title of the main window for convenience. Using this address, any MIDI client application can attach.
   This MIDI system supports multiple client connections out of the box, so many applications can easily be handled.

4) ALSA & OSS4 raw MIDI ports
   This MIDI system has very basic support in mt32emu-qt. The default MIDI port is automatically tried to open at the startup.
   The user can open and close other MIDI ports by using '+' and '-' buttons of the "MIDI Input" panel. There is also
   limited support for OSS3 MIDI sequencer primarily intended to communicate with vanilla DOSBox.
   Special provisions are needed to create a "virtual MIDI port" in order to provide a way for communication with other
   MIDI applications. The simplest approach looks to be making a FIFO and a corresponding link in the "/dev" directory.
   For example:

     mkfifo /var/tmp/sequencer
     sudo ln -s /var/tmp/sequencer /dev/sequencer

   Linux kernel also supplies a special kernel module that creates several virtual raw MIDI ports in the system. Command:

     sudo modprobe snd_virmidi

   creates by default 4 virtual raw MIDI ports bound with 4 ALSA MIDI sequencer ports, so both subsystems can interact easily.

5) JACK MIDI ports and realtime synchronous MIDI-to-audio processing
   Being cross-platform, JACK MIDI stands out of other platform-specific MIDI systems, yet it may be available along with another.
   Hence, mt3emu-qt does not create JACK MIDI ports (clients) automatically, the user has to create as many clients as necessary.
   In addition, menu option "New exclusive JACK MIDI port" can be used to create a single JACK client that is able to work as
   a complete synth with a MIDI input and a couple of audio outputs. However, this synth working in the exclusive mode cannot be
   "pinned", thus no additional MIDI sessions can be routed in.


Building
========
Cmake is required for building. The minimum set of dependencies is:

1) Cmake - cross platform make utility
   @ http://www.cmake.org/

2) Qt library
   @ http://www.qt.io/

Additional dependencies maybe needed (depending on the platform):

1) PortAudio - cross-platform audio I/O library
   @ http://www.portaudio.com/

2) DirectX SDK - for building PortAudio with DirectSound and WDMKS support
   @ https://www.microsoft.com/en-us/download/details.aspx?id=6812

3) PulseAudio - sound system for POSIX OSes - provides for accurate audio rendering
   @ https://www.freedesktop.org/wiki/Software/PulseAudio/

4) JACK Audio Connection Kit - a low-latency synchronous callback-based media server
   @ https://jackaudio.org/

The build script recognises the following configuration options to control the build:

  * mt32emu-qt_WITH_QT5 - to prefer version 5 of Qt over version 4 if both are available"
  * mt32emu-qt_USE_PULSEAUDIO_DYNAMIC_LOADING - whether to load PulseAudio library dynamically (if available)
  * mt32emu-qt_WITH_DEBUG_WINCONSOLE - enables a console for showing debug output on Windows systems
  * mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER - specifies whether to use the ALSA MIDI sequencer or raw ALSA MIDI ports
    (for Linux targets only)

The options can be set in various ways:

  * specified directly as the command line arguments within the cmake command
  * by editing CMakeCache.txt file that CMake creates in the target directory
  * using the CMake GUI

Note, the compiler optimisations are typically disabled by default. In order to get
a well-performing binary, be sure to set the value of the CMAKE_BUILD_TYPE variable
to Release or customise the compiler options otherwise.

To simplify the build configuration process in a non-POSIX environment, the CMake GUI tool
may come in handy.


License
=======

Copyright (C) 2011-2020 Jerome Fisher, Sergey V. Mikayev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Trademark disclaimer
====================

Roland is a trademark of Roland Corp. All other brand and product names are
trademarks or registered trademarks of their respective holder. Use of
trademarks is for informational purposes only and does not imply endorsement by
or affiliation with the holder.
