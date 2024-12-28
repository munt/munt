/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2024 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_TESTACCESSORS_H
#define MT32EMU_TESTACCESSORS_H

// The intention is to provide access to certain internals for the test code which
// facilitates mocking and/or testing, yet such access is not needed outside of tests.

namespace MT32Emu {

class MachineConfiguration;

namespace Test {

// Delegates to the identically named static function in c_interface.cpp
const MachineConfiguration *findMachineConfiguration(const char *machineID);

} // namespace Test

} // namespace MT32Emu

#endif // #ifndef MT32EMU_TESTACCESSORS_H
