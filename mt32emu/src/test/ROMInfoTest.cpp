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

#include "../mt32emu.h"

#include "FakeROMs.h"
#include "TestAccessors.h"
#include "Testing.h"

namespace MT32Emu {

using namespace Test;

String toString(const ROMInfo *romInfo) {
	return romInfo ? String("ROMInfo*{") + romInfo->shortName + "}(" + Test::toString(romInfo) + ")" : "NULL";
}

MT32EMU_STRINGIFY_ENUM(ROMInfo::Type)
MT32EMU_STRINGIFY_ENUM(ROMInfo::PairType)

namespace Test {

TEST_CASE("ROMInfo lists should be NULL-terminated") {
	Bit32u romInfosCount;
	const ROMInfo * const *romInfos = NULL;

	SUBCASE("All ROMInfos") {
		romInfos = ROMInfo::getAllROMInfos(&romInfosCount);
	}

	SUBCASE("Full ROMInfos") {
		romInfos = ROMInfo::getFullROMInfos(&romInfosCount);
	}

	SUBCASE("Partial ROMInfos") {
		romInfos = ROMInfo::getPartialROMInfos(&romInfosCount);
	}

	Bit32u i;
	for (i = 0; i < romInfosCount; i++) {
		CAPTURE(i);
		CHECK(romInfos[i] != NULL_PTR);
	}
	CAPTURE(i);
	REQUIRE(romInfos[i] == NULL_PTR);
}

TEST_CASE("ROMInfo lists should contain distinct ROMInfos with unique shortNames") {
	Bit32u romInfosCount;
	const ROMInfo * const *romInfos = NULL;

	SUBCASE("All ROMInfos") {
		romInfos = ROMInfo::getAllROMInfos(&romInfosCount);
	}

	SUBCASE("Full ROMInfos") {
		romInfos = ROMInfo::getFullROMInfos(&romInfosCount);
	}

	SUBCASE("Partial ROMInfos") {
		romInfos = ROMInfo::getPartialROMInfos(&romInfosCount);
	}

	for (Bit32u i = 1; i < romInfosCount; i++) {
		CAPTURE(i);
		for (Bit32u j = 0; j < i; j++) {
			CAPTURE(j);
			CHECK(romInfos[i]->shortName != romInfos[j]->shortName);
			CHECK(romInfos[i] != romInfos[j]);
		}
	}
}

TEST_CASE("All known ROMInfos should comprise of Full and Partial ones") {
	Bit32u allROMInfosCount;
	ROMInfo::getAllROMInfos(&allROMInfosCount);

	Bit32u fullROMInfosCount;
	ROMInfo::getFullROMInfos(&fullROMInfosCount);
	CAPTURE(fullROMInfosCount);

	Bit32u partialROMInfosCount;
	ROMInfo::getPartialROMInfos(&partialROMInfosCount);
	CAPTURE(partialROMInfosCount);

	REQUIRE(allROMInfosCount == fullROMInfosCount + partialROMInfosCount);
}

TEST_CASE("Full and Partial ROMInfo lists should be consistent and complete") {
	bool expectedPairTypeFull;
	Bit32u romInfosCount;
	const ROMInfo * const *romInfos = NULL;

	SUBCASE("Full ROMInfos") {
		expectedPairTypeFull = true;
		romInfos = ROMInfo::getFullROMInfos(&romInfosCount);
	}

	SUBCASE("Partial ROMInfos") {
		expectedPairTypeFull = false;
		romInfos = ROMInfo::getPartialROMInfos(&romInfosCount);
	}

	for (Bit32u i = 0; i < romInfosCount; i++) {
		CAPTURE(i);
		CAPTURE(romInfos[i]->shortName);
		if (expectedPairTypeFull) {
			CHECK(romInfos[i]->pairType == ROMInfo::Full);
			CHECK(romInfos[i]->pairROMInfo == NULL_PTR);
		} else {
			CHECK(romInfos[i]->pairType != ROMInfo::Full);
			CHECK(romInfos[i]->pairROMInfo != NULL_PTR);
		}
	}
	Bit32u allROMInfosCount;
	const ROMInfo * const *allROMInfos = ROMInfo::getAllROMInfos(&allROMInfosCount);
	for (Bit32u i = 0; i < allROMInfosCount; i++) {
		CAPTURE(i);
		if (expectedPairTypeFull) {
			if (allROMInfos[i]->pairType != ROMInfo::Full) continue;
		} else {
			if (allROMInfos[i]->pairType == ROMInfo::Full) continue;
		}
		Bit32u found = false;
		for (Bit32u j = 0; j < romInfosCount; j++) {
			if (allROMInfos[i] == romInfos[j]) {
				found = true;
				break;
			}
		}
		CHECK_MESSAGE(found, "Filtered list should contain " << allROMInfos[i]);
	}
}

TEST_CASE("Filtered ROMInfo lists should be consistent and complete") {
	SUBCASE("Empty ROMInfos") {
		const ROMInfo **romInfos = ROMInfo::getROMInfoList(0, 0);
		CHECK(romInfos[0] == NULL_PTR);
		ROMInfo::freeROMInfoList(romInfos);
	}

	SUBCASE("All ROMInfos") {
		const ROMInfo **romInfos = ROMInfo::getROMInfoList(0x1f, 0x1f);
		bool pcmROMFound = false;
		bool controlROMFound = false;
		bool fullROMFound = false;
		bool partialROMFound = false;
		Bit32u romInfosCount = 0;
		for (const ROMInfo * const *romInfo = romInfos; *romInfo != NULL; romInfo++) {
			if ((*romInfo)->type == ROMInfo::PCM) {
				pcmROMFound = true;
			} else if ((*romInfo)->type == ROMInfo::Control) {
				controlROMFound = true;
			}
			if ((*romInfo)->pairType == ROMInfo::Full) {
				fullROMFound = true;
			} else {
				partialROMFound = true;
			}
			romInfosCount++;
		}
		CHECK(pcmROMFound);
		CHECK(controlROMFound);
		CHECK(fullROMFound);
		CHECK(partialROMFound);
		Bit32u allROMInfosCount;
		ROMInfo::getAllROMInfos(&allROMInfosCount);
		CHECK(romInfosCount == allROMInfosCount);
		ROMInfo::freeROMInfoList(romInfos);
	}

	SUBCASE("Full PCM ROMInfos") {
		const ROMInfo **romInfos = ROMInfo::getROMInfoList(1 << ROMInfo::PCM, 1 << ROMInfo::Full);
		Bit32u romInfosCount = 0;
		for (const ROMInfo * const *romInfo = romInfos; *romInfo != NULL; romInfo++) {
			CAPTURE((*romInfo)->shortName);
			CHECK((*romInfo)->type == ROMInfo::PCM);
			CHECK((*romInfo)->pairType == ROMInfo::Full);
			romInfosCount++;
		}
		// Only two were made
		CHECK(romInfosCount == 2);
		ROMInfo::freeROMInfoList(romInfos);
	}
}

TEST_CASE("ROM files should be recognised by size and SHA1 digest") {
	const ROMInfo *knownROMInfo = findROMInfo("ctrl_mt32_2_07");
	REQUIRE(knownROMInfo != NULL_PTR);

	SUBCASE("File with unexpected size should not be recognised") {
		ArrayFile file(NULL, knownROMInfo->fileSize + 16, knownROMInfo->sha1Digest);
		const ROMInfo *romInfo = ROMInfo::getROMInfo(&file);
		CHECK(romInfo == NULL_PTR);
	}

	SUBCASE("File with unexpected SHA1 digest should not be recognised") {
		const File::SHA1Digest sha1Digest = { 0 };
		ArrayFile file(NULL, knownROMInfo->fileSize, sha1Digest);
		const ROMInfo *romInfo = ROMInfo::getROMInfo(&file);
		CHECK(romInfo == NULL_PTR);
	}

	SUBCASE("File with both expected size and SHA1 digest should be recognised") {
		ArrayFile file(NULL, knownROMInfo->fileSize, knownROMInfo->sha1Digest);
		const ROMInfo *romInfo = ROMInfo::getROMInfo(&file);
		CHECK(romInfo == knownROMInfo);
	}
}

TEST_CASE("ROMImages can be created") {
	ROMSet romSet;
	const char *controlROMName = "";
	const char *pcmROMName = "";

	SUBCASE("Old-gen MT-32") {
		romSet.initMT32Old(true);
		controlROMName = "ctrl_mt32_1_07";
		pcmROMName = "pcm_mt32";
	}

	SUBCASE("New-gen MT-32") {
		romSet.initMT32New(true);
		controlROMName = "ctrl_mt32_2_07";
		pcmROMName = "pcm_mt32";
	}

	SUBCASE("CM-32L") {
		romSet.initCM32L(true);
		controlROMName = "ctrl_cm32l_1_02";
		pcmROMName = "pcm_cm32l";
	}

	const ROMImage *controlROMImage = romSet.getControlROMImage();
	REQUIRE(controlROMImage != NULL_PTR);
	REQUIRE(controlROMImage->getROMInfo() != NULL_PTR);
	CHECK(controlROMImage->getROMInfo()->shortName == controlROMName);
	CHECK(controlROMImage->isFileUserProvided());

	const ROMImage *pcmROMImage = romSet.getPCMROMImage();
	REQUIRE(pcmROMImage != NULL_PTR);
	REQUIRE(pcmROMImage->getROMInfo() != NULL_PTR);
	CHECK(pcmROMImage->getROMInfo()->shortName == pcmROMName);
	CHECK(pcmROMImage->isFileUserProvided());
}

TEST_CASE("Different ROMImages can be created for aliased ROM files") {
	const ROMInfo * const *romInfos = NULL;
	const char *expectedROMName;

	SUBCASE("Full: MT-32 PCM ROM") {
		romInfos = ROMInfo::getFullROMInfos();
		expectedROMName = "pcm_mt32";
	}

	SUBCASE("Partial: lower half of CM-32L PCM ROM") {
		romInfos = ROMInfo::getPartialROMInfos();
		expectedROMName = "pcm_cm32l_l";
	}

	const ROMInfo *mt32PCMROMInfo = findROMInfo("pcm_mt32");
	REQUIRE(mt32PCMROMInfo != NULL_PTR);
	ArrayFile file(NULL, mt32PCMROMInfo->fileSize, mt32PCMROMInfo->sha1Digest);
	const ROMImage *romImage = ROMImage::makeROMImage(&file, romInfos);
	CHECK(romImage->getROMInfo() != NULL_PTR);
	CHECK(romImage->getROMInfo()->shortName == expectedROMName);
	ROMImage::freeROMImage(romImage);
}

TEST_CASE("Partial ROMImages can't be merged if incompatible") {
	const ROMInfo *romInfo1;
	const ROMInfo *romInfo2;
	SUBCASE("Partial ROM with full ROM") {
		// Technically, this is the same ROM as pcm_cm32l_l but their ROMInfos are distinct.
		romInfo1 = findROMInfo("pcm_mt32");
		romInfo2 = findROMInfo("pcm_cm32l_h");
	}

	SUBCASE("Partial ROMs for different models") {
		romInfo1 = findROMInfo("ctrl_mt32_1_07_a");
		romInfo2 = findROMInfo("ctrl_mt32_1_06_b");
	}
	REQUIRE(romInfo1 != NULL_PTR);
	REQUIRE(romInfo2 != NULL_PTR);

	ArrayFile file1(NULL, romInfo1->fileSize, romInfo1->sha1Digest);
	ArrayFile file2(NULL, romInfo2->fileSize, romInfo2->sha1Digest);
	const ROMImage *romImage1 = ROMImage::makeROMImage(&file1, &romInfo1);
	const ROMImage *romImage2 = ROMImage::makeROMImage(&file2, &romInfo2);
	File *result = mergePartialROMs(romImage1, romImage2);
	CHECK(result == NULL_PTR);
	ROMImage::freeROMImage(romImage1);
	ROMImage::freeROMImage(romImage2);
}

TEST_CASE("Pairable partial ROMImages can be merged into one full ROM file") {
	const ROMInfo *romInfo1;
	const ROMInfo *romInfo2;
	const char *expectedSHA1;

	SUBCASE("Interleaved ROMs A+B") {
		romInfo1 = findROMInfo("ctrl_mt32_1_07_a");
		romInfo2 = findROMInfo("ctrl_mt32_1_07_b");
		expectedSHA1 = "c05b4a0426b99e6a3e5e7f50f361bfe59b1fb003";
	}

	SUBCASE("Interleaved ROMs B+A") {
		romInfo1 = findROMInfo("ctrl_mt32_1_07_b");
		romInfo2 = findROMInfo("ctrl_mt32_1_07_a");
		expectedSHA1 = "9daf97604a65a71ff04f1b20cac91343e7a26d0a";
	}

	SUBCASE("Appended ROMs L+H") {
		romInfo1 = findROMInfo("pcm_mt32_l");
		romInfo2 = findROMInfo("pcm_mt32_h");
		expectedSHA1 = "b96d172eb57a5270d159810ffd7b3346651d3990";
	}

	SUBCASE("Appended ROMs H+L") {
		romInfo1 = findROMInfo("pcm_mt32_h");
		romInfo2 = findROMInfo("pcm_mt32_l");
		expectedSHA1 = "1e79d1750c3c87d22f56577a96541ebfce7e158e";
	}

	REQUIRE(romInfo1 != NULL_PTR);
	REQUIRE(romInfo2 != NULL_PTR);
	size_t dataSize = romInfo1->fileSize;
	Bit8u *data1 = new Bit8u[dataSize];
	for (size_t i = 0; i < dataSize; i++) data1[i] = Bit8u(i);
	Bit8u *data2 = new Bit8u[dataSize];
	for (size_t i = 0; i < dataSize; i++) data2[i] = Bit8u(~i);

	File *result;
	{
		ArrayFile file1(data1, dataSize, romInfo1->sha1Digest);
		ArrayFile file2(data2, dataSize, romInfo2->sha1Digest);
		const ROMImage *romImage1 = ROMImage::makeROMImage(&file1, &romInfo1);
		REQUIRE(romImage1->getROMInfo() != NULL_PTR);
		const ROMImage *romImage2 = ROMImage::makeROMImage(&file2, &romInfo2);
		REQUIRE(romImage2->getROMInfo() != NULL_PTR);

		result = mergePartialROMs(romImage1, romImage2);
		ROMImage::freeROMImage(romImage1);
		ROMImage::freeROMImage(romImage2);
	}

	delete[] data1;
	delete[] data2;

	REQUIRE(result != NULL_PTR);
	CHECK(result->getSize() == 2 * dataSize);
	CHECK(result->getSHA1() == expectedSHA1);
}

TEST_CASE("Failed merge of files with partial ROM images") {
	const ROMInfo *romInfo1 = findROMInfo("ctrl_mt32_1_07_a");
	const ROMInfo *romInfo2 = findROMInfo("ctrl_mt32_1_07_b");
	REQUIRE(romInfo1 != NULL_PTR);
	REQUIRE(romInfo2 != NULL_PTR);

	size_t dataSize = romInfo1->fileSize;
	Bit8u *data1 = new Bit8u[dataSize];
	for (size_t i = 0; i < dataSize; i++) data1[i] = Bit8u(i);
	Bit8u *data2 = new Bit8u[dataSize];
	for (size_t i = 0; i < dataSize; i++) data2[i] = Bit8u(~i);

	const ROMImage *fullROMImage;
	{
		SUBCASE("One source file is unrecognised") {
			ArrayFile file1(data1, dataSize);
			ArrayFile file2(data2, dataSize, romInfo2->sha1Digest);
			fullROMImage = ROMImage::makeROMImage(&file1, &file2);
		}

		SUBCASE("Merged result has unknown SHA1 digest") {
			ArrayFile file1(data1, dataSize, romInfo1->sha1Digest);
			ArrayFile file2(data2, dataSize, romInfo2->sha1Digest);
			fullROMImage = ROMImage::makeROMImage(&file1, &file2);
		}
	}

	delete[] data1;
	delete[] data2;

	REQUIRE(fullROMImage == NULL_PTR);
}

} // namespace Test

} // namespace MT32Emu
