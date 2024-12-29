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

#ifndef MT32EMU_FAKEROMS_H
#define MT32EMU_FAKEROMS_H

namespace MT32Emu {

class ROMImage;
struct ROMInfo;

namespace Test {

const char STARTUP_DISPLAY_MESSAGE[] = "Starting up...      ";
const char ERROR_DISPLAY_MESSAGE[] = "SysEx error!        ";

class ROMSet {
	const ROMImage *controlROMImage;
	const ROMImage *pcmROMImage;

	ROMSet(const ROMSet &);
	ROMSet &operator=(const ROMSet &);

public:
	ROMSet();
	~ROMSet();

	const ROMImage *getControlROMImage() const;
	const ROMImage *getPCMROMImage() const;

	void init(const char *machineID, bool empty = false);
	void initMT32Old(bool empty = false);
	void initMT32New(bool empty = false);
	void initCM32L(bool empty = false);
};

const ROMInfo *findROMInfo(const char *romShortName);

void createROMSet(const char *machineID, const ROMImage *&controlROMImage, const ROMImage *&pcmROMImage, bool empty = false);
void freeROMSet(const ROMImage *controlROMImage, const ROMImage *pcmROMImage);

} // namespace Test

} // namespace MT32Emu

#endif // #ifndef MT32EMU_FAKEROMS_H
