#include <mt32emu/mt32emu.h>
#include <SDL_thread.h>
#include <SDL_timer.h>
#include <SDL_endian.h>
#include "mixer.h"
#include "control.h"

static class MidiHandler_mt32 : public MidiHandler {
private:
	static const Bitu SAMPLE_RATE = MT32Emu::SAMPLE_RATE;
	static const Bitu MILLIS_PER_SECOND = 1000;
	static const Bitu MINIMUM_RENDER_FRAMES = (16 * SAMPLE_RATE) / MILLIS_PER_SECOND;
	static const Bitu AUDIO_BUFFER_SIZE = MIXER_BUFSIZE >> 1;
	static const Bitu FRAMES_PER_AUDIO_BUFFER = AUDIO_BUFFER_SIZE >> 1;
	MixerChannel *chan;
	MT32Emu::Synth *synth;
	SDL_Thread *thread;
	Bit16s audioBuffer[AUDIO_BUFFER_SIZE];
	volatile Bitu renderPos, playPos, playedBuffers;
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
	MidiHandler_mt32() : open(false), chan(NULL), synth(NULL), thread(NULL) {}

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

		playPos = 0;
		chan = MIXER_AddChannel(mixerCallBack, MT32Emu::SAMPLE_RATE, "MT32");
		if (renderInThread) {
			stopProcessing = false;
			renderPos = 0;
			render(FRAMES_PER_AUDIO_BUFFER - 1, audioBuffer);
			playedBuffers = 1;
			thread = SDL_CreateThread(processingThread, NULL);
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
			SDL_WaitThread(thread, NULL);
			thread = NULL;
		}
		MIXER_DelChannel(chan);
		chan = NULL; 
		synth->close();
		delete synth;
		synth = NULL;
		open = false;
	}

	void PlayMsg(Bit8u *msg) {
		if (renderInThread) {
			synth->playMsg(SDL_SwapLE32(*(Bit32u *)msg), getMidiEventTimestamp());
		} else {
			synth->playMsg(SDL_SwapLE32(*(Bit32u *)msg));
		}
	}

	void PlaySysex(Bit8u *sysex, Bitu len) {
		if (renderInThread) {
			synth->playSysex(sysex, len, getMidiEventTimestamp());
		} else {
			synth->playSysex(sysex, len);
		}
	}

private:
	Bit32u inline getMidiEventTimestamp() {
		return playedBuffers * FRAMES_PER_AUDIO_BUFFER + (playPos >> 1);
	}

	void render(const Bitu len, Bit16s *buf) {
		Bit16s *revBuf = &buf[renderPos];
		synth->render(revBuf, len);
		if (reverseStereo) {
			for(Bitu i = 0; i < len; i++) {
				Bit16s left = revBuf[0];
				Bit16s right = revBuf[1];
				*revBuf++ = right;
				*revBuf++ = left;
			}
		}
		renderPos = (renderPos + (len << 1)) % AUDIO_BUFFER_SIZE;
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
		while (midiHandler_mt32.renderPos == midiHandler_mt32.playPos) {
			SDL_Delay(1);
		}
		Bitu renderPos = midiHandler_mt32.renderPos;
		Bitu playPos = midiHandler_mt32.playPos;
		Bitu samplesReady;
		if (renderPos < playPos) {
			samplesReady = AUDIO_BUFFER_SIZE - playPos;
		} else {
			samplesReady = renderPos - playPos;
		}
		if (len > (samplesReady >> 1)) {
			len = samplesReady >> 1;
		}
		midiHandler_mt32.chan->AddSamples_s16(len, &midiHandler_mt32.audioBuffer[playPos]);
		playPos += (len << 1);
		while (AUDIO_BUFFER_SIZE <= playPos) {
			playPos -= AUDIO_BUFFER_SIZE;
			midiHandler_mt32.playedBuffers++;
		}
		midiHandler_mt32.playPos = playPos;
	} else {
		midiHandler_mt32.renderPos = 0;
		midiHandler_mt32.render(len, (Bit16s *)MixTemp);
		midiHandler_mt32.chan->AddSamples_s16(len, (Bit16s *)MixTemp);
	}
}

int MidiHandler_mt32::processingThread(void *) {
	while (!midiHandler_mt32.stopProcessing) {
		Bitu renderPos = midiHandler_mt32.renderPos;
		Bitu playPos = midiHandler_mt32.playPos;
		Bitu framesToRender;
		if (renderPos < playPos) {
			framesToRender = (playPos - renderPos - 2) >> 1;
			if (framesToRender < MINIMUM_RENDER_FRAMES) {
				SDL_Delay(1 + (MILLIS_PER_SECOND * (MINIMUM_RENDER_FRAMES - framesToRender)) / SAMPLE_RATE);
				continue;
			}
		} else {
			framesToRender = (AUDIO_BUFFER_SIZE - renderPos) >> 1;
			if (playPos == 0) {
				framesToRender--;
				if (framesToRender == 0) {
					SDL_Delay(1 + (MILLIS_PER_SECOND * MINIMUM_RENDER_FRAMES) / SAMPLE_RATE);
					continue;
				}
			}
		}
		midiHandler_mt32.render(framesToRender, midiHandler_mt32.audioBuffer);
	}
	return 0;
}
