#include <mt32emu/mt32emu.h>
#include <SDL_thread.h>
#include "mixer.h"
#include "control.h"

class RingBuffer {
private:
	static const unsigned int bufferSize = 1024;
	volatile unsigned int startpos;
	volatile unsigned int endpos;
	Bit32u ringBuffer[bufferSize];

public:
	RingBuffer() {
		startpos = 0;
		endpos = 0;
	}

	bool put(Bit32u data) {
		unsigned int newEndpos = endpos;
		newEndpos++;
		if (newEndpos == bufferSize) newEndpos = 0;
		if (startpos == newEndpos) return false;
		ringBuffer[endpos] = data;
		endpos = newEndpos;
		return true;
	}

	Bit32u get() {
		if (startpos == endpos) return 0;
		Bit32u data = ringBuffer[startpos];
		startpos++;
		if (startpos == bufferSize) startpos = 0;
		return data;
	}
};

static class MidiHandler_mt32 : public MidiHandler {
private:
	static const Bitu MIXER_BUFFER_SIZE = MIXER_BUFSIZE >> 2;
	MixerChannel *chan;
	MT32Emu::Synth *synth;
	RingBuffer midiBuffer;
	SDL_Thread *thread;
	SDL_mutex *synthMutex;
	SDL_semaphore *procIdleSem, *mixerReqSem;
	Bit16s mixerBuffer[2 * MIXER_BUFFER_SIZE];
	volatile Bitu mixerBufferSize;
	volatile bool stopProcessing;
	bool open, noise, reverseStereo, renderInThread;

static void printDebug(void *userData, const char *fmt, va_list list);
static void mixerCallBack(Bitu len);
static int processingThread(void *);

public:
	MidiHandler_mt32() : open(false), chan(NULL), synth(NULL), thread(NULL), synthMutex(NULL), procIdleSem(NULL), mixerReqSem(NULL) {}

	~MidiHandler_mt32() {
		Close();
	}

	const char *GetName(void) {
		return "mt32";
	}

	bool Open(const char *conf) {
		MT32Emu::SynthProperties tmpProp = {0};
		tmpProp.sampleRate = 32000;

		tmpProp.useDefaultReverb = false;
		tmpProp.useReverb = true;
		tmpProp.reverbType = 0;
		tmpProp.reverbTime = 5;
		tmpProp.reverbLevel = 3;
		tmpProp.userData = this;
		tmpProp.printDebug = printDebug;
		tmpProp.report = &report;

		synth = new MT32Emu::Synth();
		if (!synth->open(tmpProp)) {
			LOG_MSG("MT32: Error initialising emulation");
			return false;
		}

		Section_prop *section = static_cast<Section_prop *>(control->GetSection("midi"));
		if (strcmp(section->Get_string("mt32.reverb.mode"), "auto") != 0) {
			Bit8u reverbsysex[] = {0x10, 0x00, 0x01, 0x00, 0x05, 0x03};
			reverbsysex[3] = (Bit8u)atoi(section->Get_string("mt32.reverb.mode"));
			reverbsysex[4] = (Bit8u)section->Get_int("mt32.reverb.time");
			reverbsysex[5] = (Bit8u)section->Get_int("mt32.reverb.level");
			synth->writeSysex(16, reverbsysex, 6);
			synth->setReverbOverridden(true);
		} else {
			LOG_MSG("MT32: Using default reverb");
		}

		if (strcmp(section->Get_string("mt32.dac"), "auto") != 0) {
			synth->setDACInputMode((MT32Emu::DACInputMode)atoi(section->Get_string("mt32.dac")));
		}

		reverseStereo = strcmp(section->Get_string("mt32.reverse.stereo"), "on") == 0;
		noise = strcmp(section->Get_string("mt32.verbose"), "on") == 0;
		renderInThread = strcmp(section->Get_string("mt32.thread"), "on") == 0;

		chan = MIXER_AddChannel(mixerCallBack, tmpProp.sampleRate, "MT32");
		if (renderInThread) {
			mixerBufferSize = 0;
			stopProcessing = false;
			synthMutex = SDL_CreateMutex();
			procIdleSem = SDL_CreateSemaphore(0);
			mixerReqSem = SDL_CreateSemaphore(0);
			thread = SDL_CreateThread(processingThread, NULL);
			//if (thread == NULL || synthMutex == NULL || sleepMutex == NULL) renderInThread = false;
		}
		chan->Enable(true);

		open = true;
		return true;
	}

	void Close(void) {
		if (!open) return;
		chan->Enable(false);
		if (renderInThread) {
			stopProcessing = true;
			SDL_SemPost(mixerReqSem);
			SDL_WaitThread(thread, NULL);
			thread = NULL;
			SDL_DestroyMutex(synthMutex);
			synthMutex = NULL;
			SDL_DestroySemaphore(procIdleSem);
			procIdleSem = NULL;
			SDL_DestroySemaphore(mixerReqSem);
			mixerReqSem = NULL;
		}
		MIXER_DelChannel(chan);
		chan = NULL;
		synth->close();
		delete synth;
		synth = NULL;
		open = false;
	}

	void PlayMsg(Bit8u *msg) {
		if (!midiBuffer.put(*(Bit32u *)msg)) LOG_MSG("MT32: Playback buffer full!");
	}

	void PlaySysex(Bit8u *sysex, Bitu len) {
		if (renderInThread) SDL_LockMutex(synthMutex);
		synth->playSysex(sysex, len);
		if (renderInThread) SDL_UnlockMutex(synthMutex);
	}

private:
	static int report(void *userData, MT32Emu::ReportType type, const void *reportData) {
		switch (type) {
			case MT32Emu::ReportType_errorControlROM:
				LOG_MSG("MT32: Couldn't find Control ROM file");
				break;
			case MT32Emu::ReportType_errorPCMROM:
				LOG_MSG("MT32: Couldn't open PCM ROM file");
				break;
			case MT32Emu::ReportType_lcdMessage:
				LOG_MSG("MT32: LCD-Message: %s", reportData);
				break;
			default:
				//LOG(LOG_ALL,LOG_NORMAL)("MT32: Report %d",type);
				break;
		}
		return 0;
	}

	void render(Bitu len, Bit16s *buf) {
		Bit32u msg = midiBuffer.get();
		if (msg != 0) synth->playMsg(msg);
		synth->render(buf, len);
		if (reverseStereo) {
			Bit16s *revBuf = buf;
			for(Bitu i = 0; i < len; i++) {
				Bit16s left = revBuf[0];
				Bit16s right = revBuf[1];
				*revBuf++ = right;
				*revBuf++ = left;
			}
		}
		chan->AddSamples_s16(len, buf);
	}
} midiHandler_mt32;

void MidiHandler_mt32::printDebug(void *userData, const char *fmt, va_list list) {
	if (midiHandler_mt32.noise) {
		printf("MT32: ");
		vprintf(fmt, list);
		printf("\n");
	}
}

void MidiHandler_mt32::mixerCallBack(Bitu len) {
	if (midiHandler_mt32.renderInThread) {
		SDL_SemWait(midiHandler_mt32.procIdleSem);
		midiHandler_mt32.mixerBufferSize += len;
		SDL_SemPost(midiHandler_mt32.mixerReqSem);
	} else {
		midiHandler_mt32.render(len, (Bit16s *)MixTemp);
	}
}

int MidiHandler_mt32::processingThread(void *) {
	while (!midiHandler_mt32.stopProcessing) {
		SDL_SemPost(midiHandler_mt32.procIdleSem);
		SDL_SemWait(midiHandler_mt32.mixerReqSem);
		for (;;) {
			Bitu samplesToRender = midiHandler_mt32.mixerBufferSize;
			if (samplesToRender == 0) break;
			SDL_LockMutex(midiHandler_mt32.synthMutex);
			midiHandler_mt32.render(samplesToRender, midiHandler_mt32.mixerBuffer);
			SDL_UnlockMutex(midiHandler_mt32.synthMutex);
			midiHandler_mt32.mixerBufferSize -= samplesToRender;
		}
	}
	return 0;
}
