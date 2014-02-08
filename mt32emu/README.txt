Munt mt32emu
============

mt32emu is a module of the Munt project. It produces a C++ static link library
named libmt32emu which provides classes to emulate (approximately) the Roland
MT-32, CM-32L and LAPC-I synthesiser modules.

This library is intended for developers wishing to integrate an MT-32 emulator
into a driver or an application. "Official" driver for Windows and a cross-platform
UI-enabled application are available in the Munt project and use this library:
mt32emu_win32drv and mt32emu_qt respectively.

mt32emu requires CMake to build. See http://www.cmake.org/ for details. For a
simple in-tree build, you can probably just do:

cmake .
make
sudo make install


Hardware requirements
=====================

The emulation engine requires at least 800 MHz CPU to perform in real-time. 8MB of RAM is needed.


License
=======

Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
Copyright (C) 2011, 2012, 2013, 2014 Dean Beeler, Jerome Fisher, Sergey V. Mikayev

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
