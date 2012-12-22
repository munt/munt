#include <mt32emu/mt32emu.h>
#include <SDL_thread.h>
#include <SDL_timer.h>
#include "mixer.h"
#include "control.h"

class RingBuffer {
private:
	static const unsigned int bufferSize = 1024;
	volatile unsigned int startpos;
	volatile unsigned int endpos;
	Bit64u ringBuffer[bufferSize];

public:
	RingBuffer() {
		startpos = 0;
		endpos = 0;
	}

	bool put(Bit64u data) {
		unsigned int newEndpos = endpos;
		newEndpos++;
		if (newEndpos == bufferSize) newEndpos = 0;
		if (startpos == newEndpos) return false;
		ringBuffer[endpos] = data;
		endpos = newEndpos;
		return true;
	}

	Bit64u peek() {
		if (startpos == endpos) return 0;
		return ringBuffer[startpos];
	}

	Bit64u take() {
		if (startpos == endpos) return 0;
		Bit64u data = ringBuffer[startpos];
		startpos++;
		if (startpos == bufferSize) startpos = 0;
		return data;
	}
};

static class MidiHandler_mt32 : public MidiHandler {
private:
	static const Bitu SAMPLE_RATE = 32000;
	static const Bitu MILLIS_PER_SECOND = 1000;
	static const Bitu MINIMUM_RENDER_FRAMES = (16 * SAMPLE_RATE) / MILLIS_PER_SECOND;
	static const Bitu AUDIO_BUFFER_SIZE = MIXER_BUFSIZE >> 1;
	MixerChannel *chan;
	MT32Emu::Synth *synth;
	RingBuffer midiBuffer;
	SDL_Thread *thread;
	SDL_mutex *synthMutex;
	Bit16s audioBuffer[AUDIO_BUFFER_SIZE];
	volatile Bitu renderPos, playPos;
	volatile bool stopProcessing;
	bool open, noise, reverseStereo, renderInThread;

static void printDebug(void *userData, const char *fmt, va_list list);
static void mixerCallBack(Bitu len);
static int processingThread(void *);

public:
	MidiHandler_mt32() : open(false), chan(NULL), synth(NULL), thread(NULL), synthMutex(NULL) {}

	~MidiHandler_mt32() {
		Close();
	}

	const char *GetName(void) {
		return "mt32";
	}

	bool Open(const char *conf) {
		MT32Emu::SynthProperties tmpProp = {0};
		tmpProp.sampleRate = SAMPLE_RATE;

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
		
		playPos = 0;
		chan = MIXER_AddChannel(mixerCallBack, tmpProp.sampleRate, "MT32");
		if (renderInThread) {
			stopProcessing = false;
			renderPos = 0;
			render((AUDIO_BUFFER_SIZE - 2) >> 1, audioBuffer);
			synthMutex = SDL_CreateMutex();
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
			SDL_DestroyMutex(synthMutex);
			synthMutex = NULL;
		}
		MIXER_DelChannel(chan);
		chan = NULL; 
		synth->close();
		delete synth;
		synth = NULL;
		open = false;
	}

	void PlayMsg(Bit8u *msg) {
		if (!midiBuffer.put(*(Bit32u *)msg | (Bit64u(playPos + AUDIO_BUFFER_SIZE) << 32))) LOG_MSG("MT32: Playback buffer full!");
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

	void render(const Bitu len, Bit16s *buf) {
		Bitu framesTotal = len;
		Bitu renderPos = this->renderPos;
		Bit16s *revBuf = &buf[renderPos % AUDIO_BUFFER_SIZE];
		if (renderInThread) SDL_LockMutex(synthMutex);
		while (framesTotal > 0) {
			Bit64u msg = midiBuffer.peek();
			Bitu framesToRender = framesTotal;
			if(msg != 0) {
				if (renderInThread) {
					Bit32u msgTime = Bit32u(msg >> 32);
					if (renderPos < msgTime) {
						framesToRender = (msgTime - renderPos) >> 1;
						if (framesTotal < framesToRender) framesToRender = framesTotal;
					} else {
						synth->playMsg((Bit32u)midiBuffer.take());
						framesToRender = 1;
					}
				} else {
					synth->playMsg((Bit32u)midiBuffer.take());
				}
			}
			synth->render(&buf[renderPos % AUDIO_BUFFER_SIZE], framesToRender);
			framesTotal -= framesToRender;
			renderPos += framesToRender << 1;
			this->renderPos = renderPos;
		}
		if (renderInThread) SDL_UnlockMutex(synthMutex);
		if (reverseStereo) {
			for(Bitu i = 0; i < len; i++) {
				Bit16s left = revBuf[0];
				Bit16s right = revBuf[1];
				*revBuf++ = right;
				*revBuf++ = left;
			}
		}
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
		while (midiHandler_mt32.renderPos == midiHandler_mt32.playPos) {
			SDL_Delay(1);
		}
		Bitu renderPos = midiHandler_mt32.renderPos % AUDIO_BUFFER_SIZE;
		Bitu playPos = midiHandler_mt32.playPos % AUDIO_BUFFER_SIZE;
		Bitu samplesReady;
		if (renderPos < playPos) {
			samplesReady = AUDIO_BUFFER_SIZE - playPos;
		} else {
			samplesReady = renderPos - playPos;
		}
		if ((samplesReady >> 1) < len) {
			len = samplesReady >> 1;
		}
		midiHandler_mt32.chan->AddSamples_s16(len, &midiHandler_mt32.audioBuffer[playPos]);
		midiHandler_mt32.playPos += len << 1;
	} else {
		midiHandler_mt32.renderPos = 0;
		midiHandler_mt32.render(len, (Bit16s *)MixTemp);
		midiHandler_mt32.chan->AddSamples_s16(len, (Bit16s *)MixTemp);
	}
}

int MidiHandler_mt32::processingThread(void *) {
	while (!midiHandler_mt32.stopProcessing) {
		Bitu renderPos = midiHandler_mt32.renderPos % AUDIO_BUFFER_SIZE;
		Bitu playPos = midiHandler_mt32.playPos % AUDIO_BUFFER_SIZE;
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
