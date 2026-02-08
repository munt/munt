/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2026 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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
#include "../Synth.h"

#include "FakeROMs.h"
#include "TestAccessors.h"
#include "TestReportHandler.h"
#include "Testing.h"

#define MT32EMU_API_TYPE 3

#include "../mt32emu.h"

namespace MT32Emu {

template <class ServiceImpl>
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

template <class ServiceImpl>
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

template <class ReportHandlerImpl>
static ReportHandler3 *ensureNewContextReportHandler(Service &service, ReportHandlerImpl &rh) {
	service.createContext(rh);
	REQUIRE(service.getContext() != NULL_PTR);
	mt32emu_context context = service.getContext();
	REQUIRE(context != NULL_PTR);
	ReportHandler3 *res = getReportHandlerDelegate(context);
	REQUIRE(res != NULL_PTR);
	return res;
}

#define MT32EMU_createSubcase(reportedEventType, factory) \
	SUBCASE(#reportedEventType) { \
		static const ReportedEvent e = factory; \
		reportedEvent = &e; \
	}

TEST_CASE("Service creates ReportHandler that delegates handling of all events to provided interface") {
	const ReportedEvent *reportedEvent = NULL;
	MT32EMU_createSubcase(ReportedEvent::DEBUG_MESSAGE, ReportedEvent::debugMessage("Missing PCM ROM"))
	MT32EMU_createSubcase(ReportedEvent::ERROR_CONTROL_ROM, ReportedEvent::errorControlROM())
	MT32EMU_createSubcase(ReportedEvent::ERROR_PCM_ROM, ReportedEvent::errorPCMROM())
	MT32EMU_createSubcase(ReportedEvent::LCD_MESSAGE, ReportedEvent::lcdMessageShown("*** Waiting... ***"))
	MT32EMU_createSubcase(ReportedEvent::MIDI_MESSAGE, ReportedEvent::midiMessage())
	MT32EMU_createSubcase(ReportedEvent::MIDI_QUEUE_OVERFLOW, ReportedEvent::midiQueueOverflow())
	MT32EMU_createSubcase(ReportedEvent::MIDI_SYSTEM_REALTIME, ReportedEvent::midiSystemRealtime(0xFE))
	MT32EMU_createSubcase(ReportedEvent::DEVICE_RESET, ReportedEvent::deviceReset())
	MT32EMU_createSubcase(ReportedEvent::DEVICE_RECONFIG, ReportedEvent::deviceReconfig())
	MT32EMU_createSubcase(ReportedEvent::NEW_REVERB_MODE, ReportedEvent::newReverbMode(2))
	MT32EMU_createSubcase(ReportedEvent::NEW_REVERB_TIME, ReportedEvent::newReverbTime(3))
	MT32EMU_createSubcase(ReportedEvent::NEW_REVERB_LEVEL, ReportedEvent::newReverbLevel(7))
	MT32EMU_createSubcase(ReportedEvent::POLY_STATE_CHANGED, ReportedEvent::polyStateChanged(6))
	MT32EMU_createSubcase(ReportedEvent::PROGRAM_CHANGED, ReportedEvent::programChanged(3, "Group 5", "Timbre 7"))
	MT32EMU_createSubcase(ReportedEvent::LCD_STATE_UPDATED, ReportedEvent::lcdStateUpdated())
	MT32EMU_createSubcase(ReportedEvent::MIDI_MESSAGE_LED_STATE_UPDATED, ReportedEvent::midiMessageLEDStateUpdated(true))
	MT32EMU_createSubcase(ReportedEvent::NOTE_ON_IGNORED, ReportedEvent::noteOnIgnored(4, 2))
	MT32EMU_createSubcase(ReportedEvent::PLAYING_POLY_SILENCED, ReportedEvent::playingPolySilenced(3, 1))

	REQUIRE(reportedEvent != NULL_PTR);
	TestReportHandler<IReportHandlerV2> rh(Array<const ReportedEvent>(reportedEvent, 1));
	Service service;
	ReportHandler3 *emitter = ensureNewContextReportHandler(service, rh);
	switch (reportedEvent->type) {
	case ReportedEvent::DEBUG_MESSAGE:
		emitter->printDebug("Missing PCM ROM", 0);
		break;
	case ReportedEvent::ERROR_CONTROL_ROM:
		emitter->onErrorControlROM();
		break;
	case ReportedEvent::ERROR_PCM_ROM:
		emitter->onErrorPCMROM();
		break;
	case ReportedEvent::LCD_MESSAGE:
		emitter->showLCDMessage("*** Waiting... ***");
		break;
	case ReportedEvent::MIDI_MESSAGE:
		emitter->onMIDIMessagePlayed();
		break;
	case ReportedEvent::MIDI_QUEUE_OVERFLOW:
		emitter->onMIDIQueueOverflow();
		break;
	case ReportedEvent::MIDI_SYSTEM_REALTIME:
		emitter->onMIDISystemRealtime(0xFE);
		break;
	case ReportedEvent::DEVICE_RESET:
		emitter->onDeviceReset();
		break;
	case ReportedEvent::DEVICE_RECONFIG:
		emitter->onDeviceReconfig();
		break;
	case ReportedEvent::NEW_REVERB_MODE:
		emitter->onNewReverbMode(2);
		break;
	case ReportedEvent::NEW_REVERB_TIME:
		emitter->onNewReverbTime(3);
		break;
	case ReportedEvent::NEW_REVERB_LEVEL:
		emitter->onNewReverbLevel(7);
		break;
	case ReportedEvent::POLY_STATE_CHANGED:
		emitter->onPolyStateChanged(6);
		break;
	case ReportedEvent::PROGRAM_CHANGED:
		emitter->onProgramChanged(3, "Group 5", "Timbre 7");
		break;
	case ReportedEvent::LCD_STATE_UPDATED:
		emitter->onLCDStateUpdated();
		break;
	case ReportedEvent::MIDI_MESSAGE_LED_STATE_UPDATED:
		emitter->onMidiMessageLEDStateUpdated(true);
		break;
	case ReportedEvent::NOTE_ON_IGNORED:
		emitter->onNoteOnIgnored(4, 2);
		break;
	case ReportedEvent::PLAYING_POLY_SILENCED:
		emitter->onPlayingPolySilenced(3, 1);
		break;
	default:
		FAIL("Unknown event type" << reportedEvent->type);
		break;
	}

	rh.checkRemainingEvents();
}

#undef MT32EMU_createSubcase

TEST_CASE("Service creates ReportHandler that ignores events not supported by provided interface V0") {
	const ReportedEvent events[] = { ReportedEvent::deviceReset() };
	TestReportHandler<IReportHandler> rh(events);
	Service service;
	ReportHandler3 *emitter = ensureNewContextReportHandler(service, rh);
	emitter->onNoteOnIgnored(4, 2);
	emitter->onLCDStateUpdated();
	emitter->onDeviceReset();

	rh.checkRemainingEvents();
}

TEST_CASE("Service creates ReportHandler that ignores events not supported by provided interface V1") {
	const ReportedEvent events[] = { ReportedEvent::lcdStateUpdated(), ReportedEvent::deviceReset() };
	TestReportHandler<IReportHandlerV1> rh(events);
	Service service;
	ReportHandler3 *emitter = ensureNewContextReportHandler(service, rh);
	emitter->onNoteOnIgnored(4, 2);
	emitter->onLCDStateUpdated();
	emitter->onDeviceReset();

	rh.checkRemainingEvents();
}

TEST_CASE("Service creates ReportHandler that ignores events if it cannot deliver them to provided interface") {
	struct Inner {
		static mt32emu_report_handler_version MT32EMU_C_CALL getVersionID(mt32emu_report_handler_i) {
			return MT32EMU_REPORT_HANDLER_VERSION_CURRENT;
		}

		mt32emu_report_handler_i_v2 rh;

		Inner() : rh() {
			rh.getVersionID = getVersionID;
		}
	} wrapper;
	mt32emu_report_handler_i rrh;
	rrh.v2 = &wrapper.rh;
	Service service;
	ReportHandler3 *emitter = ensureNewContextReportHandler(service, rrh);
	emitter->onNoteOnIgnored(4, 2);
	emitter->onLCDStateUpdated();
	emitter->onDeviceReset();
}

} // namespace Test

} // namespace MT32Emu
