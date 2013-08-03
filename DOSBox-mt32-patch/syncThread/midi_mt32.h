#include <mt32emu/mt32emu.h>
#include <SDL_thread.h>
#include <SDL_endian.h>
#include "mixer.h"
#include "control.h"

static class MidiHandler_mt32 : public MidiHandler {
private:
	static const Bitu MIXER_BUFFER_SIZE = MIXER_BUFSIZE >> 2;
	MixerChannel *chan;
	MT32Emu::Synth *synth;
	SDL_Thread *thread;
	SDL_mutex *synthMutex;
	SDL_semaphore *procIdleSem, *mixerReqSem;
	Bit16s mixerBuffer[2 * MIXER_BUFFER_SIZE];
	volatile Bitu mixerBufferSize;
	volatile bool stopProcessing;
	bool open, noise, reverseStereo, renderInThread;

	class MT32ReportHandler : public MT32Emu::ReportHandler {
		protected:
			virtual void onErrorControlROM() {
				LOG_MSG("MT32: Couldn't open Control ROM file");
			}

			virtual void onErrorPCMROM() {
				LOG_MSG("MT32: Couldn't open PCM ROM file");
			}

			virtual void showLCDMessage(const char *message) {
				LOG_MSG("MT32: LCD-Message: %s", message);
			}

			virtual void printDebug(const char *fmt, va_list list);
		} reportHandler;

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
		MT32Emu::FileStream controlROMFile;
		MT32Emu::FileStream pcmROMFile;

		if (!controlROMFile.open("CM32L_CONTROL.ROM")) {
			if (!controlROMFile.open("MT32_CONTROL.ROM")) {
				LOG_MSG("MT32: Control ROM file not found");
				return false;
			}
		}
		if (!pcmROMFile.open("CM32L_PCM.ROM")) {
			if (!pcmROMFile.open("MT32_PCM.ROM")) {
				LOG_MSG("MT32: PCM ROM file not found");
				return false;
			}
		}
		const MT32Emu::ROMImage *controlROMImage = MT32Emu::ROMImage::makeROMImage(&controlROMFile);
		const MT32Emu::ROMImage *pcmROMImage = MT32Emu::ROMImage::makeROMImage(&pcmROMFile);
		synth = new MT32Emu::Synth(&reportHandler);
		if (!synth->open(*controlROMImage, *pcmROMImage)) {
			LOG_MSG("MT32: Error initialising emulation");
			return false;
		}
		MT32Emu::ROMImage::freeROMImage(controlROMImage);
		MT32Emu::ROMImage::freeROMImage(pcmROMImage);

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

		chan = MIXER_AddChannel(mixerCallBack, MT32Emu::SAMPLE_RATE, "MT32");
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
		synth->playMsg(SDL_SwapLE32(*(Bit32u *)msg));
	}

	void PlaySysex(Bit8u *sysex, Bitu len) {
		synth->playSysex(sysex, len);
	}

private:
	void render(Bitu len, Bit16s *buf) {
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

void MidiHandler_mt32::MT32ReportHandler::printDebug(const char *fmt, va_list list) {
	if (midiHandler_mt32.noise) {
		char s[1024];
		strcpy(s, "MT32: ");
		vsnprintf(s + 6, 1017, fmt, list);
		LOG_MSG(s);
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
