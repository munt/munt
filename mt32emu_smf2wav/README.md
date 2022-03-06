Munt mt32emu-smf2wav
====================

_mt32emu-smf2wav_ is a part of the Munt project. It makes use of [the mt32emu
library](https://github.com/munt/munt/tree/master/mt32emu) to produce a WAVE
file from a [Standard MIDI
file](https://www.midi.org/specifications-old/item/standard-midi-files-smf)
(SMF). Files in this format commonly have the extension ".smf" or ".mid".

This program is experimental and mainly intended as an aid to Munt developers
and an example of embedding _mt32emu_ library in a program.

There is no documentation, program arguments are likely to change in future and
there are undoubtedly bugs.

Note that only SMF files designed for the Roland MT-32 and compatible devices
are likely to produce pleasing output. The MT-32 is *not* a General MIDI device.


Building
========

_mt32emu-smf2wav_ requires CMake to build. More info can be found at [the CMake
homepage](http://www.cmake.org/). GLIB is a required dependency. Can be found at
http://www.gtk.org/.

For a simple in-tree build in a POSIX environment, you can probably just run the
following commands from the source directory:

    cmake -DCMAKE_BUILD_TYPE:STRING=Release .
    make
    sudo make install


License
=======

Copyright (C) 2009, 2011 Jerome Fisher <re_munt@kingguppy.com><br>
Copyright (C) 2012-2022 Jerome Fisher, Sergey V. Mikayev

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
