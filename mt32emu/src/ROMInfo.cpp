/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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
#include "ROMInfo.h"

namespace MT32Emu {

	// ROM definitions
	static const ROMInfo mt32ctrl = {
		65536, (unsigned char *)"B083518F-FFB7F66B-03C23B7E-B4F868E6-2DC5A987", ROMInfo::Control,
		(unsigned char *)"mt32ctrl", (unsigned char *)"MT32 Control ROM", ROMInfo::Full, NULL, NULL};

	static const ROMInfo cm32ctrl = {
		65536, (unsigned char *)"A439FBB3-90DA38CA-DA95A7CB-B1D6CA19-9CD66EF8", ROMInfo::Control,
		(unsigned char *)"cm32ctrl", (unsigned char *)"CM32L Control ROM", ROMInfo::Full, NULL, NULL};

	static const ROMInfo mt32pcm = {
		524288, (unsigned char *)"F6B1EEBC-4B2D200E-C6D3D21D-51325D5B-48C60252", ROMInfo::PCM,
		(unsigned char *)"mt32pcm", (unsigned char *)"MT32 PCM ROM", ROMInfo::Full, NULL, NULL};

	static const ROMInfo cm32pcm = {
		1048576, (unsigned char *)"289CC298-AD532B70-2461BFC7-38009D9E-BE8025EA", ROMInfo::PCM,
		(unsigned char *)"cm32pcm", (unsigned char *)"CM32L PCM ROM", ROMInfo::Full, NULL, NULL};

	static const ROMInfo* romInfos[] = {&mt32ctrl, &cm32ctrl, &mt32pcm, &cm32pcm, NULL};

const ROMInfo* ROMInfo::getROMInfo(File *file) {
	size_t fileSize = file->getSize();
	unsigned char *fileDigest = file->getSHA1();
	const ROMInfo **romInfo = romInfos;
	do {
		if ((fileSize == (*romInfo)->fileSize)
				&& (strcmp((char *)fileDigest, (char *)(*romInfo)->sha1Digest) == 0)) {
			break;
		}
	} while (*(++romInfo) != NULL);
	return *romInfo;
}

void ROMInfo::freeROMInfo(const ROMInfo *romInfo) {
}

const ROMInfo** ROMInfo::getROMInfoList(unsigned int types, unsigned int pairTypes) {
	unsigned int romCount = 0;
	const ROMInfo **romInfo = romInfos;
	do 
		++romCount;
	while (*(++romInfo) != NULL);
	if (romCount == 0) {
		return NULL;
	}
	const ROMInfo **romInfoList = new const ROMInfo*[romCount + 1];
	const ROMInfo **currentROMInList = romInfoList;
	romInfo = romInfos;
	do {
		if ((types & (1 << (*romInfo)->type)) && (pairTypes & (1 << (*romInfo)->pairType))) {
			*(currentROMInList++) = *romInfo;
		}
	} while (*(++romInfo) != NULL);
	*currentROMInList = NULL;
	return romInfoList;
}

void ROMInfo::freeROMInfoList(const ROMInfo **romInfos) {
	delete[] romInfos;
}

const ROMImage* ROMImage::makeROMImage(const File *file, const ROMInfo *romInfo) {
	ROMImage *romImage = new ROMImage;
	romImage->file = const_cast <File *> (file);
	romImage->romInfo = const_cast <ROMInfo *> (romInfo);
	return romImage;
}

void ROMImage::freeROMImage(const ROMImage *romImage) {
	delete romImage;
}

}
