Munt mt32emu-au
===============

mt32emu-au is a MacOS X AudioUnit instrument wrapper for libmt32emu. 

The AudioUnit instrument lets you use mt32emu as an instrument in any OSX audio software that supports AudioUnits, such as GarageBand, Logic or Ableton Live. 


Building
========
Latest version of XCode is required for building. mt32emu must be built and installed on the system.


Installing and using the component
===================

The AudioUnit component does not include the MT32 roms in the component bundle and requires them to be placed under /Library/MT32 as MT32_CONTROL.ROM and MT32_PCM.ROM to function.

To install the component, place muntAU.component under either

/Library/Audio/Plug-Ins/Components

or

~/Library/Audio/Plug-Ins/Components

After that, an instument named MT-32 Emulator should become available in your music software.

The AudioUnit property UI should have controls for volume, instrument, reverb on/off and reverb gain available.

License
=======

Copyright (C) 2014, Ivan Safrin

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