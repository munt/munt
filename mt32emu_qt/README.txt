Munt mt32emu-qt
===============

mt32emu-qt makes use of libmt32emu and Qt to provide for:

1) Multiple simultaneous synths, GUI to configure synths, manage ROMs and connections
2) Funny LCD
3) Easy usage in different operating system environments:
   Windows multimedia, PulseAudio, ALSA, OSS and CoreMIDI supported
4) Play and record Standard MIDI files
5) Perform batch conversion of Standard MIDI files directly to .wav / .raw audio files


MIDI Support
============

mt32emu-qt can communicate with other MIDI applications using virtual MIDI ports which depends on the operating system.
Currently, supported MIDI systems are: Windows Multimedia, CoreMIDI, ALSA MIDI sequencer, and ALSA & OSS4 raw MIDI ports.
In order to use this feature, the client MIDI applications need to setup correctly, as well as some more steps may be
necessary on particular systems.

Because CoreMIDI, ALSA MIDI sequencer support virtual MIDI ports natively, these systems seem simplest to use.
As mt32emu-qt starts, it creates a virtual MIDI port and listens for incoming connections. Contrary to that, Windows Multimedia
requires the dedicated MIDI driver to be setup in the system and mt32emu-qt instead communicates with the driver as a proxy.
Some brief notes follow below related to each MIDI system.

1) Windows Multimedia
   If a system happens to have a hardware MIDI-in port or an existing virtual loopback MIDI port driver, mt32emu-qt readily
   supports this configuration and should be able to open any MIDI-in port in the system by using '+' button of "MIDI Input" panel
   or "New MIDI port..." item in "Tools" menu.
   When there is no MIDI-in port available, the dedicated MIDI driver can be used to forward MIDI messages to mt32emu-qt
   for playback. This is done automatically by the driver when it detects a running instanse of mt32emu-qt.

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


Building
========
Cmake is required to building. The minimum set of dependencies is:

1) Cmake - cross platform make utility
   @ http://www.cmake.org/

2) Qt library
   @ http://www.qt.io/

Additional dependencies maybe needed (depending on the platform):

1) PortAudio - cross-platform audio I/O library
   @ http://www.portaudio.com/

2) DirectX SDK - for building PortAudio with DirectSound and WDMKS support
   @ http://www.microsoft.com/en-us/download/details.aspx?id=6812

3) PulseAudio - sound system for POSIX OSes - provides for accurate audio rendering
   @ http://www.pulseaudio.org/ or http://www.freedesktop.org/wiki/Software/PulseAudio

4) libsoxr - The SoX Resampler library - to perform fast and high quality sample-rate conversion
   @ http://sourceforge.net/projects/soxr/

5) libsamplerate - Secret Rabbit Code - Sample Rate Converter
   @ http://www.mega-nerd.com/SRC/


License
=======

Copyright (C) 2011-2016 Jerome Fisher, Sergey V. Mikayev

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
