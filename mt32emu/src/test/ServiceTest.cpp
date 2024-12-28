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

#include "../ROMInfo.h"

#include "FakeROMs.h"
#include "Testing.h"

#define MT32EMU_API_TYPE 3

#include "../mt32emu.h"

namespace MT32Emu {

template<class ServiceImpl>
struct TestService : ServiceImpl {
	TestService() {}

	void addROMSet(const Test::ROMSet &romSet);
};

namespace PluginApiTest {

#undef MT32EMU_API_TYPE
#define MT32EMU_API_TYPE 2

#undef MT32EMU_MT32EMU_H
#undef MT32EMU_CPP_INTERFACE_H
#include "../mt32emu.h"

} // namespace PluginApiTest

typedef PluginApiTest::MT32Emu::Service PluginService;

template<>
TestService<PluginService>::TestService() : PluginService(mt32emu_get_service_i()) {}

} // namespace MT32Emu

using namespace MT32Emu;
using namespace MT32Emu::Test;

#define UnwrappedTestTypes Service, PluginService

#if MT32EMU_NEED_WRAPPING_TEMPLATE_TEST_TYPES
MT32EMU_WRAP_TEMPLATE_TEST_TYPES(TestTypes, UnwrappedTestTypes)
#else
#define TestTypes UnwrappedTestTypes
#endif

TYPE_TO_STRING(Service);
TYPE_TO_STRING(PluginService);

MT32EMU_STRINGIFY_ENUM(mt32emu_return_code)

namespace MT32Emu {

template<class ServiceImpl>
void TestService<ServiceImpl>::addROMSet(const ROMSet &romSet) {
	const ROMImage *controlROMImage = romSet.getControlROMImage();
	const ROMImage *pcmROMImage = romSet.getPCMROMImage();
	REQUIRE(controlROMImage != NULL_PTR);
	REQUIRE(pcmROMImage != NULL_PTR);
	mt32emu_return_code rc = ServiceImpl::addROMData(controlROMImage->getFile()->getData(), controlROMImage->getFile()->getSize(),
		&controlROMImage->getROMInfo()->sha1Digest);
	REQUIRE(rc == MT32EMU_RC_ADDED_CONTROL_ROM);
	rc = ServiceImpl::addROMData(pcmROMImage->getFile()->getData(), pcmROMImage->getFile()->getSize(),
		&pcmROMImage->getROMInfo()->sha1Digest);
	REQUIRE(rc == MT32EMU_RC_ADDED_PCM_ROM);
}

namespace Test {

TEST_CASE_TEMPLATE("Service provides for checking library version ", ServiceImpl, TestTypes) {
	TestService<ServiceImpl> service;
	CHECK(service.getContext() == NULL_PTR);
	CHECK(service.getLibraryVersionInt() == MT32EMU_CURRENT_VERSION_INT);
	CHECK(service.getLibraryVersionString() == MT32EMU_VERSION);
}

TEST_CASE_TEMPLATE("Service can't open synth without ROMs ", ServiceImpl, TestTypes) {
	TestService<ServiceImpl> service;
	CHECK(service.getContext() == NULL_PTR);
	service.createContext();
	REQUIRE(service.getContext() != NULL_PTR);

	mt32emu_rom_info rom_info;
	service.getROMInfo(&rom_info);
	CHECK(rom_info.control_rom_id == NULL_PTR);
	CHECK(rom_info.pcm_rom_id == NULL_PTR);

	mt32emu_return_code rc = service.openSynth();
	CHECK(rc == MT32EMU_RC_MISSING_ROMS);
	CHECK_FALSE(service.isOpen());

	service.freeContext();
	CHECK(service.getContext() == NULL_PTR);
}

TEST_CASE_TEMPLATE("Service can open synth with ROMs ", ServiceImpl, TestTypes) {
	TestService<ServiceImpl> service;
	CHECK(service.getContext() == NULL_PTR);
	service.createContext();
	REQUIRE(service.getContext() != NULL_PTR);

	ROMSet romSet;
	romSet.initMT32New();
	service.addROMSet(romSet);

	mt32emu_rom_info rom_info;
	service.getROMInfo(&rom_info);
	CHECK(rom_info.control_rom_id == "ctrl_mt32_2_07");
	CHECK(rom_info.pcm_rom_id == "pcm_mt32");

	mt32emu_return_code rc = service.openSynth();
	CHECK(rc == MT32EMU_RC_OK);
	CHECK(service.isOpen());

	service.freeContext();
	CHECK(service.getContext() == NULL_PTR);
}

} // namespace Test

} // namespace MT32Emu
