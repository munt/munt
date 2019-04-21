Munt mt32emu_alsadrv
====================

mt32emu_alsadrv is a module of the Munt project. It uses the mt32emu library and
provides an ALSA MIDI driver which emulates (approximately) the Roland MT-32,
CM-32L and LAPC-I synthesiser modules.


Building
========

Requirements:
- A gcc/g++ version newer than 2.95.*. Successfully tested with GCC 3.3.
- libmt32emu built and installed.
- libasound development files.
- libx11 development files.
- libxpm development files.
- libxt development files.

Build mt32d and xmt32:
>> make

Change to root user in order to install:
>> su root

Install:
>> make install

mt32d and xmt32 will be installed to /usr/local/bin

Please ensure that the ROM files are installed in 
/usr/share/mt32-rom-data

If the ROM files are correctly installed yet the 
program cannot open them, check the filenames (case sensitive) 
and permissions.


Usage
=====

Requirements:
- A copy of the MT32_PCM.ROM file (not provided).
- A copy of the MT32_CONTROL.ROM file (not provided).
- A fastish computer.
- ALSA MIDI support (not OSS)

Running mt32d/xmt32 will provide two ports 128:0 and 128:1 provided
that it is the only soft-synth running. When trying to play MT-32 
midi streams use ALSA port 128:0.

128:1 attempts to emulate general midi support using the MT-32 mode,
try it for a laugh. Run either mt32d which is the console version or
run xmt32 for the X-Windows graphical version. Both versions have optional
command line parameters, run mt32d or xmt32 with the -h parameter to get a
list and description of the other parameters.

Running mt32d or xmt32 as root user will allow program to use real-time
scheduling which may reduce/remove drop outs as the program can use the 
CPU more aggressively. 


The user interface for xmt32
============================

The main screen shows the latency (top left), outputs (top right), LCD
messages (centre) and general messages (below LCD messages).

The outputs section will contain one or more of the following letters:

 X - Recording sysex messages
 P - Playing digital audio in real-time
 W - Writing .wav file

The buttons on the right control several aspects of operation:

 Clear - Clears the currently playing midi events
 Reset - Causes the emulated device to revert to its power up state


License
=======

Copyright (C) 2003 Tristan
Copyright (C) 2004, 2005 Tristan, Jerome Fisher
Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
Copyright (C) 2013-2019 Tristan, Jerome Fisher, Jörg Walter, Sergey V. Mikayev

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


Trademark disclaimer
====================

Roland is a trademark of Roland Corp. All other brand and product names are
trademarks or registered trademarks of their respective holder. Use of
trademarks is for informational purposes only and does not imply endorsement by
or affiliation with the holder.
