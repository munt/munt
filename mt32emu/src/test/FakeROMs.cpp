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

#include <cstring>

#include "../File.h"
#include "../ROMInfo.h"
#include "../Types.h"
#include "TestAccessors.h"

#include "FakeROMs.h"

namespace MT32Emu {

namespace Test {

static const ROMImage *createFakeROMImage(const ROMInfo *romInfo, const Bit8u *data = NULL) {
	return ROMImage::makeROMImage(new ArrayFile(data, romInfo->fileSize, romInfo->sha1Digest));
}

static Bit8u *createFakeControlROM(const ROMInfo *romInfo) {
	Bit8u *data = new Bit8u[romInfo->fileSize];
	memset(data, 0, romInfo->fileSize);
	// Populate the max tables so that any value in a SysEx is accepted.
	// The ranges are wider than necessary to overlap variations between specific ROM versions.
	// It's OK as we don't actually read nearby values anyway.
	memset(data + 0x4880, 0x7f, 256); // New-gen
	memset(data + 0x51f0, 0x7f, 256); // Old-gen
	return data;
}

static Bit8u *createFakePCMROM(const ROMInfo *romInfo) {
	Bit8u *data = new Bit8u[romInfo->fileSize];
	memset(data, 0, romInfo->fileSize);
	return data;
}

const ROMInfo *findROMInfo(const char *romShortName) {
	for (const ROMInfo * const *romInfo = ROMInfo::getAllROMInfos(); *romInfo != NULL; romInfo++) {
		if (!strcmp(romShortName, (*romInfo)->shortName)) return *romInfo;
	}
	return NULL;
}

void createROMSet(const char *machineID, const ROMImage *&controlROMImage, const ROMImage *&pcmROMImage, bool empty) {
	controlROMImage = NULL;
	pcmROMImage = NULL;
	const MachineConfiguration *config = findMachineConfiguration(machineID);
	if (!config) return;
	for (const ROMInfo * const *romInfo = config->getCompatibleROMInfos(); *romInfo != NULL; romInfo++) {
		if ((*romInfo)->pairType != ROMInfo::Full) continue;
		if ((*romInfo)->type == ROMInfo::Control) {
			controlROMImage = createFakeROMImage(*romInfo, empty ? NULL : createFakeControlROM(*romInfo));
		}
		if ((*romInfo)->type == ROMInfo::PCM) {
			pcmROMImage = createFakeROMImage(*romInfo, empty ? NULL : createFakePCMROM(*romInfo));
		}
	}
}

void freeROMSet(const ROMImage *controlROMImage, const ROMImage *pcmROMImage) {
	const ROMImage *romImages[] = { controlROMImage, pcmROMImage };
	for (Bit32u i = 0; i < 2; i++) {
		const ROMImage *romImage = romImages[i];
		if (romImage == NULL) continue;
		File *file = romImage->getFile();
		const Bit8u *data = file->getData();
		ROMImage::freeROMImage(romImage);
		delete file;
		delete[] data;
	}
}

ROMSet::ROMSet() : controlROMImage(), pcmROMImage() {}

ROMSet::~ROMSet() {
	freeROMSet(controlROMImage, pcmROMImage);
	controlROMImage = NULL;
	pcmROMImage = NULL;
}

const ROMImage *ROMSet::getControlROMImage() const {
	return controlROMImage;
}

const ROMImage *ROMSet::getPCMROMImage() const {
	return pcmROMImage;
}

void ROMSet::init(const char *machineID, bool empty) {
	freeROMSet(controlROMImage, pcmROMImage);
	createROMSet(machineID, controlROMImage, pcmROMImage, empty);
}

void ROMSet::initMT32Old(bool empty) {
	init("mt32_1_07", empty);
}

void ROMSet::initMT32New(bool empty) {
	init("mt32_2_07", empty);
}

void ROMSet::initCM32L(bool empty) {
	init("cm32l_1_02", empty);
}

} // namespace Test

} // namespace MT32Emu
