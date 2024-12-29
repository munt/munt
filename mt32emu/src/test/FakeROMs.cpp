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
#include "../Structures.h"
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
	const ControlROMMap *controlROMMap = getControlROMMap(romInfo->shortName);
	if (controlROMMap != NULL) {
		// Populate things we care about in tests:
		// The max tables, so that any value in a SysEx is accepted
		memset(data + controlROMMap->patchMaxTable, 0x7f, 16);
		memset(data + controlROMMap->rhythmMaxTable, 0x7f, 4);
		memset(data + controlROMMap->systemMaxTable, 0x7f, 23);
		memset(data + controlROMMap->timbreMaxTable, 0x7f, 72);

		// The user-visible display messages
		memcpy(data + controlROMMap->startupMessage, STARTUP_DISPLAY_MESSAGE, 21);
		memcpy(data + controlROMMap->sysexErrorMessage, ERROR_DISPLAY_MESSAGE, 21);

		// A few sound groups
		Bit32u fakeSoundGroupsCount = 4;
		data[controlROMMap->soundGroupsCount] = fakeSoundGroupsCount;
		SoundGroup *soundGroupsTable = reinterpret_cast<SoundGroup *>(data + controlROMMap->soundGroupsTable);
		for (Bit32u i = 0; i < fakeSoundGroupsCount; i++) {
			memcpy(soundGroupsTable[i].name, "Group 1|", 8);
			soundGroupsTable[i].name[6] += Bit8u(i);
		}

		// The sound group index
		for (int i = -128; i < 0; i++) {
			data[controlROMMap->soundGroupsTable + i] = Bit8u(i & 1);
		}
	}
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
