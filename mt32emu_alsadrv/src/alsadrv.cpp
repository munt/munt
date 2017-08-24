/* Copyright (C) 2003 Tristan
 * Copyright (C) 2004, 2005 Tristan, Jerome Fisher
 * Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
 * Copyright (C) 2013-2017 Tristan, Jerome Fisher, Jörg Walter, Sergey V. Mikayev
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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <math.h>

#include <alsa/version.h>
#include <alsa/asoundlib.h>

// #define SAVE_TIMBRES 
char *recsyx_filename = NULL;
FILE *recsyx_file = NULL;

char *recwav_filename = NULL;
FILE *recwav_file = NULL;

#define PERC_CHANNEL  9 
const char default_rom_dir[] = "/usr/share/mt32-rom-data/";

#include <mt32emu/mt32emu.h>

#include "maps.h"
#include "drvreport.h"
#include "alsadrv.h"

// #define DEBUG

/* Reverb information */
int rv_type = 0;
int rv_time = 0;
int rv_level = 0;


/* midi event and ui-command pipes */
int eventpipe[2];
int uicmd_pipe[2];


#define BUFFER_SIZE_INC  20
#define MINPROCESS_SIZE  16
#define URUN_MAX         2

class SysexHandler : public MT32Emu::MidiStreamParser {
public:
	explicit SysexHandler(MT32Emu::Synth &useSynth) : synth(useSynth) {}

	void handleSystemRealtimeMessage(const MT32Emu::Bit8u realtime) { /* Not interesting */ }
	void handleShortMessage(const MT32Emu::Bit32u message) { /* Not interesting */ }

	void handleSysex(const MT32Emu::Bit8u stream[], const MT32Emu::Bit32u length) {
		synth.playSysex(stream, length);
	}

	void printDebug(const char *debugMessage) {
		printf("SysexHandler: %s\n", debugMessage);
	}

private:
	MT32Emu::Synth &synth;
};

MT32Emu::Synth *mt32;
SysexHandler *sysexHandler;
snd_seq_t *seq_handle = NULL;


/* Buffer infomation */
#define FRAGMENT_SIZE 256 // 2 milliseconds
#define PERIOD_SIZE   128 // 1 millisecond

MT32Emu::AnalogOutputMode analog_output_mode = MT32Emu::AnalogOutputMode_ACCURATE;
unsigned int sample_rate = 48000;

char *rom_dir = NULL;
enum rom_search_type_t rom_search_type;

int buffermsec = 100;
int minimum_msec = 40;
int maximum_msec = 1500;
int buffer_mode = BUFFER_AUTO;
int num_underruns = 0;	
unsigned int playbuffer_size = 0;


int events_qd = 0;

int tempo = -1, ppq = -1;
double timepertick = -1;
double bytespermsec = (double)(sample_rate * 2 * 2) / 1000000.0;

int channelmap[16];
int channeluse[16];

/* midi structures */
int port_in_mt, port_in_gm;
struct pollfd *fdbank;
int consumer_types = 0;

/* pcm structures */
snd_pcm_t *pcm_handle = NULL;
snd_pcm_hw_params_t *pcm_hwparams;
// char *pcm_name = "plughw:0,0";
char *pcm_name = "default";

double gain_multiplier = 1.0d;

/* midi queue control variables */
int flush_events = 0;


/* template for reverb sysex events */
MT32Emu::Bit8u rvsysex[] = {
	0xf0, 0x41, 0x10, 0x16, 0x12, 
		0x10, 0x00, 0x01, /* system area address */
		0x00,       /* data */
		0x12,       /* check sum */ 
		0xf7
};


#ifdef DEBUG
void debug_msg(const char *msg, ...)
{
	va_list ap;
	
	va_start(ap, msg);	
	vprintf(msg, ap);		
	va_end(ap);
}
#else
void debug_msg(const char *msg, ...)
{
	
}
#endif

int alsa_set_buffer_time(int msec)
{
	int dir, err, channels, realmsec;
	unsigned int v, rate, periods;
	double sec, tpp;
	
	rate = sample_rate;
	channels = 2;
	sec = (double)msec / 1000.0;
	
	snd_pcm_hw_params_malloc(&pcm_hwparams);	
	if (snd_pcm_hw_params_any(pcm_handle, pcm_hwparams) < 0) 
	{	
		fprintf(stderr, "Can not configure this PCM device.\n");
		return -1;
	}
	
	if (snd_pcm_hw_params_set_access(pcm_handle, pcm_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "Error setting access.\n");
		return -1;
	}
			
	/* Set sample format */
	err = snd_pcm_hw_params_set_format(pcm_handle, pcm_hwparams, SND_PCM_FORMAT_S16);
	if (err < 0) 
	{
		fprintf(stderr, "Error setting format: %s\n", snd_strerror(err));
		return -1;
	}

	/* Set number of channels */
	if (snd_pcm_hw_params_set_channels(pcm_handle, pcm_hwparams, channels) < 0) {
		fprintf(stderr, "Error setting channels.\n");
		return -1;
	}

	/* Set sample rate. If the exact rate is not supported */
	/* by the hardware, use nearest possible rate.         */
#if SND_LIB_MAJOR < 1	
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, pcm_hwparams, rate, 0) < 0) 
#else
	dir = 0;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, pcm_hwparams, &rate, &dir) < 0) 		
#endif		
	{
		fprintf(stderr, "Error setting rate.\n");
		return -1;
	}
	
	
	/* calculate time per period in seconds */
	tpp = (double)PERIOD_SIZE / (double)(rate * 2 * channels); 
	
	/* calculate the number of periods required. round it up */
	periods = (unsigned int)ceil(sec / tpp);
	if (periods < 1) periods = 1;
	
	realmsec = (int)((double)periods * tpp * 1000.0);
	
	printf("Buffer resize: Requested %d msec got %d msec / %d periods \n", msec, realmsec, periods);
	
	/* Set number of periods.  */
#if SND_LIB_MAJOR < 1	
	if (snd_pcm_hw_params_set_periods_near(pcm_handle, pcm_hwparams, periods, 0) < 0)
#else
	dir = 0;
	if (snd_pcm_hw_params_set_periods_near(pcm_handle, pcm_hwparams, &periods, &dir) < 0)
#endif		
	{
		fprintf(stderr, "Error setting number of periods.\n");
		return -1;		
	}
	
#if SND_LIB_MAJOR < 1	
	if (snd_pcm_hw_params_set_period_size_near(pcm_handle, pcm_hwparams, PERIOD_SIZE, 0) < 0) 
#else
	v = PERIOD_SIZE; dir = 0;
	if (snd_pcm_hw_params_set_period_size_near(pcm_handle, pcm_hwparams, (snd_pcm_uframes_t *)&v, &dir) < 0) 
#endif				
	{
		fprintf(stderr, "Error setting period size.\n");
		return -1;
	}	

	/* Apply HW parameter settings to */
	/* PCM device and prepare device  */
	err = snd_pcm_hw_params(pcm_handle, pcm_hwparams);
	if (err < 0) {
		fprintf(stderr, "Error setting HW params: %s\n", snd_strerror(err));
		return -1;
	}	
	
	report(DRV_LATENCY, realmsec);
	
	return realmsec;
}

int alsa_init_pcm(unsigned int rate, int channels)
{
	snd_pcm_sw_params_t *swparams;
	int cards, err, dir;
	unsigned int v;
	
	/* init PCM support */ 
	cards = -1;		
	err =  snd_card_next(&cards);
	if (err < 0)
	{
		fprintf(stderr, "Could not find a suitable ALSA pcm-device: %s\n",
			snd_strerror(err));
		exit(1);
	}  
	
	if (snd_pcm_open(&pcm_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		report(DRV_ERRPCM, pcm_name);
		return -1;
	}	
		
	if (alsa_set_buffer_time(buffermsec) < 1)
		abort();
			
	/* setup ALSA software interface */
	snd_pcm_sw_params_alloca(&swparams);	
	err = snd_pcm_sw_params_current(pcm_handle, swparams);
	if (err < 0) {
		printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
		return -1;
	}
	
	/* allow the transfer when at least PERIOD_SIZE samples can be processed */
	err = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, PERIOD_SIZE >> 2);
	if (err < 0) {
		printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
		return -1;
	}
	
	/* align all transfers to 1 sample */
	err = snd_pcm_sw_params_set_xfer_align(pcm_handle, swparams, 1);
	if (err < 0) {
		printf("Unable to set transfer align for playback: %s\n", snd_strerror(err));
		return -1;
	}
	
	/* write the parameters to the playback device */
	err = snd_pcm_sw_params(pcm_handle, swparams);
	if (err < 0) {
		printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
		return -1;
	}

	snd_pcm_nonblock(pcm_handle, 1);
	
	return 0;
}

int alsa_setup_midi()
{
	int port_in_mt;
	
	/* open sequencer interface for input */
	if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		return -1;
	}
	
	snd_seq_client_id(seq_handle);
	snd_seq_set_client_name(seq_handle, "MT-32");
	port_in_mt = snd_seq_create_simple_port(seq_handle, "Standard",
						SND_SEQ_PORT_CAP_SUBS_WRITE |
						SND_SEQ_PORT_CAP_WRITE,
						SND_SEQ_PORT_TYPE_MIDI_MT32 |
						SND_SEQ_PORT_TYPE_SYNTH);		
	if (port_in_mt < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		return -1;
	}	

	port_in_gm = snd_seq_create_simple_port(seq_handle, "GM Emulation",
						SND_SEQ_PORT_CAP_SUBS_WRITE |
						SND_SEQ_PORT_CAP_WRITE,
						SND_SEQ_PORT_TYPE_MIDI_GM |
						SND_SEQ_PORT_TYPE_SYNTH);		
	if (port_in_gm < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		return -1;
	}	
	
	printf("MT-32 emulator ALSA address is %d:0\n", snd_seq_client_id(seq_handle));
	
	return port_in_mt;
}

void attempt_realtime()
{
	int status;
	
	status = nice(-20);	
	if (status != -1)
		printf("Set thread priority to -20\n");
}

extern unsigned char wav_header[];
void write_wav_header(FILE *f)
{
	// At position 40 is the 4 bytes describing the length 
	fwrite(wav_header, 1, 44, f);
}

/*
static inline int time_to_offset(struct timeval ot, struct timeval nt)
{
	double ct;
	int offset;
	
	timersub(&nt, &ot, &nt);
	
	ct = (double)nt.tv_sec * 1000000.0 + (double)nt.tv_usec;
	
	offset = (int)(ct * bytespermsec);
	return offset;
}
*/

static inline struct timeval get_time()
{
	struct timeval nt;
	
	gettimeofday(&nt, NULL);
	return nt;
}

static inline void get_null_event(midiev_t *newev)
{
	newev->type = EVENT_NONE;
	gettimeofday(&newev->stamp, NULL);
}

int remap(int channel)
{
	int i, t, max;
		
	/* update usage info */
	for (i = 1; i < PERC_CHANNEL; i++)
		if (channeluse[i] < 0x0FFFFFFF)
			channeluse[i]++;
		
	t = channelmap[channel];
	if (t != -1)
	{
		channeluse[t] = 0;
		return t;
	}
	
	/* channel 10 (perc) */
	if (channel == PERC_CHANNEL)
	{
		channelmap[channel] = PERC_CHANNEL;
		return PERC_CHANNEL;
	}
	
	/* look for last used channel */
	for (i = 1, max = 1; i < PERC_CHANNEL; i++)
		if (channeluse[max] < channeluse[i])
			max = i;
	
	channelmap[channel] = max;
	channeluse[max] = 0;

	return max;
}

void send_rvmode_sysex(int newmode)
{
	midiev_t newev;	
	newev.type = EVENT_SET_RVMODE; newev.msg = newmode;
	write(eventpipe[1], &newev, sizeof(newev));
}
void send_rvtime_sysex(int newtime)
{
	midiev_t newev;	
	newev.type = EVENT_SET_RVTIME; newev.msg = newtime;
	write(eventpipe[1], &newev, sizeof(newev));
}
void send_rvlevel_sysex(int newlevel)
{
	midiev_t newev;	
	newev.type = EVENT_SET_RVLEVEL; newev.msg = newlevel;
	write(eventpipe[1], &newev, sizeof(newev));
}

	
void flush_mt32_emu()
{
	midiev_t newev;
	unsigned int msg;
	int i, j;
	
	/* flush out events */
	while(1)
	{
		if (read(eventpipe[0], &newev, sizeof(newev)) != sizeof(newev))
			break;
	}
	
	/* push note flush events */
	for (i = 0; i < 16; i++)
	{
		msg  = 0xB0 | i;
		msg |= 0x7b << 8;
		mt32->playMsg(msg);
	}
	
	return;
	
	for (i = 0; i < 16; i++)
		for (j = 0; j < 127; j++)
		{
			msg  = 0x80 | i;
			msg |= j   << 8;
			msg |= 127 << 16;
			mt32->playMsg(msg);
		}
}

void client_subscribe_message(snd_seq_event_t *seq_ev)
{
	snd_seq_client_info_t *cinfo;
	const char *name;
	int status, client, port;	
	
	client = seq_ev->data.connect.sender.client;
	port = seq_ev->data.connect.sender.port;
	
	snd_seq_client_info_malloc(&cinfo);
	status = snd_seq_get_any_client_info(seq_handle, client, cinfo);
	
	if (status >= 0)
	{
		name = snd_seq_client_info_get_name(cinfo);
		if (seq_ev->dest.port == 0)
			report(DRV_SUBMT32,  name);
		else
			report(DRV_SUBGMEMU, name);
	} else 
		report(DRV_UNSUB);
	
	snd_seq_client_info_free(cinfo);
}

static inline void get_msg(snd_seq_event_t *seq_ev, midiev_t *ev)
{
	unsigned int channel;
	int v;
		
	switch(seq_ev->type)
	{	
	    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
		client_subscribe_message(seq_ev);
		ev->type = EVENT_NONE;
		return;
		
	    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		client_subscribe_message(seq_ev);
		ev->type = EVENT_UNSUBSCRIBED;
		flush_events = 1;
		return;
		
	    case SND_SEQ_EVENT_CLIENT_START:
		ev->type = EVENT_NONE;
		return;
		
	    case SND_SEQ_EVENT_TEMPO:
		debug_msg("TEMPO\n");
		ev->type = EVENT_NONE;
		return;
		
	    case SND_SEQ_EVENT_QFRAME:
		debug_msg("QFRAME\n");
		ev->type = EVENT_NONE;
		return;
		
	    case SND_SEQ_EVENT_TIMESIGN:
		debug_msg("TIMESIGN\n");
		ev->type = EVENT_NONE;
		return;
		
	    case SND_SEQ_EVENT_NOTE:
		debug_msg("Plain note\n");
		ev->type = EVENT_NONE;
		return;
		
	    case SND_SEQ_EVENT_NOTEON:				
		channel = seq_ev->data.note.channel;
		
		ev->msg = 0x90 | channel;
		ev->msg |= (unsigned long)seq_ev->data.note.note << 8;
		ev->msg |= (unsigned long)seq_ev->data.note.velocity << 16;			
		
//		assert(seq_ev->data.note.note < 128);
//		assert(seq_ev->data.note.velocity < 128);
		
		debug_msg("Note ON, channel:%d note:%d velocity:%d\n",
			  seq_ev->data.note.channel, seq_ev->data.note.note,
			  seq_ev->data.note.velocity);
		ev->type = EVENT_MIDI;
		return;
		
	    case SND_SEQ_EVENT_NOTEOFF:
		channel = seq_ev->data.note.channel;

		ev->msg = 0x80 | channel;
		ev->msg |= (unsigned long)seq_ev->data.note.note << 8;
		ev->msg |= (unsigned long)seq_ev->data.note.velocity << 16;			
//		assert(seq_ev->data.note.note < 128);
		
		debug_msg("Note OFF, channel:%d note:%d velocity:%d\n",
			  seq_ev->data.note.channel, seq_ev->data.note.note,
			  seq_ev->data.note.velocity);
		ev->type = EVENT_MIDI;
		return;
		
	    case SND_SEQ_EVENT_CONTROLLER:	
		channel = seq_ev->data.control.channel;
		
		ev->msg= 0xB0 | channel;
		ev->msg|= (unsigned long)seq_ev->data.control.param << 8;
		ev->msg|= (unsigned long)seq_ev->data.control.value << 16;			
		debug_msg("Controller, channel:%d param:%d value:%d\n",
		       seq_ev->data.control.channel, seq_ev->data.control.param,
		       seq_ev->data.control.value);
//		assert(seq_ev->data.control.param < 128);
//		assert(seq_ev->data.control.value < 128);			
		ev->type = EVENT_MIDI;
		return;
		
	    case SND_SEQ_EVENT_CONTROL14:
		// The real hardware units don't support any of LSB controllers,
		// so we just send the MSB silently ignoring the LSB
		if ((seq_ev->data.control.param & 0xE0) == 0x20) {
			ev->type = EVENT_NONE;
			return;
		}

		channel = seq_ev->data.control.channel;

		ev->msg= 0xB0 | channel;
		ev->msg|= seq_ev->data.control.param << 8;
		ev->msg|= (seq_ev->data.control.value >> 7) << 16;
		debug_msg("Controller 14-bit, channel:%d param:%d value:%d\n",
		       seq_ev->data.control.channel, seq_ev->data.control.param,
		       seq_ev->data.control.value);
		ev->type = EVENT_MIDI;
		return;

	    case SND_SEQ_EVENT_NONREGPARAM:
		// The real hardware units don't support NRPNs
		ev->type = EVENT_NONE;
		return;

	    case SND_SEQ_EVENT_REGPARAM:
		// The real hardware units support only RPN 0 (pitch bender range) and only MSB matters
		if (seq_ev->data.control.param == 0) {
			unsigned int *msg_buffer = new unsigned int[3];
			unsigned int msg = (0x0000B0 | seq_ev->data.control.channel);
			msg_buffer[0] = msg | 0x006400;
			msg_buffer[1] = msg | 0x006500;
			msg_buffer[2] = msg | 0x000600 | ((seq_ev->data.control.value >> 7) & 0x7F) << 16;

			debug_msg("RPN: channel:%d param:%d value:%d\n",
				   seq_ev->data.control.channel, seq_ev->data.control.param,
				   seq_ev->data.control.value);

			ev->type = EVENT_MIDI_TRIPLET;
			ev->sysex_len = 3 * sizeof(unsigned int);
			ev->sysex = msg_buffer;
		} else {
			ev->type = EVENT_NONE;
		}
		return;

	    case SND_SEQ_EVENT_PGMCHANGE:
		channel = seq_ev->data.control.channel;
		
		ev->msg= 0xC0 | channel;
		ev->msg|= (unsigned long)seq_ev->data.control.value << 8;
		debug_msg("Program change: channel:%d value:%d\n", 
		       seq_ev->data.control.channel,
		       seq_ev->data.control.value);
		ev->type = EVENT_MIDI;
		
/*		if (seq_ev->data.control.value == 61)
		{
			printf("PROG CUT\n");
			ev->msg = 0;
			ev->type = EVENT_NONE;
		}
*/		return;
		
	    case SND_SEQ_EVENT_PITCHBEND:
		channel = seq_ev->data.control.channel;
		
		ev->msg= 0xE0 | channel;
		v = (int)seq_ev->data.control.value + (int)0x2000;
		
//		assert(v >= 0);
		debug_msg("Pitch bend: channel:%d value:%d\n", channel, (int)seq_ev->data.control.value);
		
		//((long)data[i] + ((long)((uint32)data[i+1]) << 7)) - 0x2000
		ev->msg |= ((unsigned long)v & 0x7F) << 8;
		ev->msg |= (((unsigned long)v & 0x3F80) >> 7) << 16;
		ev->type = EVENT_MIDI;
		return;
		
	    case SND_SEQ_EVENT_SYSEX:
		debug_msg("SysEx (fragment) of size %d\n", seq_ev->data.ext.len);

		ev->type = EVENT_SYSEX;
		ev->sysex_len = seq_ev->data.ext.len;
		ev->sysex = malloc(seq_ev->data.ext.len);
		memcpy(ev->sysex, seq_ev->data.ext.ptr, ev->sysex_len);
		
		return;
		
	    default:
		debug_msg("Unknown message: %d\n", seq_ev->type);
		break;
	}

	/* default midi event */
	ev->type = EVENT_NONE;		
}

static inline int convert_to_mt(snd_seq_event_t *seq_ev)
{	
	unsigned char *channel;
	int n, nch;
	
	switch(seq_ev->type)
	{
	    case SND_SEQ_EVENT_PGMCHANGE:
		/* remap ins */
		n = seq_ev->data.control.value;		
		seq_ev->data.control.value = midi_gm2mt[n + 1] - 1;		
		break;			
		
	    case SND_SEQ_EVENT_NOTEON:
		if (seq_ev->data.note.channel == PERC_CHANNEL)
		{
			/* remap perc */
			n = seq_ev->data.note.note;
			seq_ev->data.note.note = perc_gm2mt[n + 1] - 1;
			break;
		}
		break;
		
	    case SND_SEQ_EVENT_SYSEX:
		report(DRV_SYSEXGM);
		return 1;
	}
	
	switch(seq_ev->type)
	{
	    case SND_SEQ_EVENT_NOTE:
	    case SND_SEQ_EVENT_NOTEON:
	    case SND_SEQ_EVENT_NOTEOFF:
		channel = &seq_ev->data.note.channel;
		break;
		
	    case SND_SEQ_EVENT_PGMCHANGE:
	    case SND_SEQ_EVENT_CONTROLLER:
	    case SND_SEQ_EVENT_PITCHBEND:
		channel = &seq_ev->data.control.channel;
		break;
				
	    default:
		return 0;
	}	
	
	nch = remap(*channel);	
	*channel = nch;		

	return 0;
}

char *create_filename(const char *base, const char *ext)
{
	char fullname[128], *n;
	struct stat buf;
	int i, si;
	
	for (i = 0; i < 999; i++)
	{
		sprintf(fullname, "%s%d%s", base, i, ext);
		if (stat(fullname, &buf) != -1)
			continue;	      
		if (errno == ENOENT)
		{
			n = (char *)malloc(strlen(fullname) + 1);
			strcpy(n, fullname);
			return n;
		}
	}
	return NULL;
}

void start_recordsyx()
{		
	recsyx_filename = create_filename("timbres", ".syx");
	if (recsyx_filename == NULL)
		return;
	recsyx_file = fopen(recsyx_filename, "w");
	if (recsyx_file == NULL)
	{
		free(recsyx_filename);
		return;
	}

	consumer_types |= CONSUME_SYSEX;
	report(DRV_SYXOUTPUT, 1);	
}
void start_recordwav()
{
	recwav_filename = create_filename("output", ".wav");
	if (recwav_filename == NULL)
		return;
	recwav_file = fopen(recwav_filename, "w");
	if (recwav_file == NULL)
	{
		report(DRV_ERRWAVOUT, errno);
		free(recwav_filename);
		return;
	}	
	write_wav_header(recwav_file);
	
	consumer_types |= CONSUME_WAVOUT;
	report(DRV_WAVOUTPUT, 1);	
}

static inline void underrun()
{	
	report(DRV_UNDERRUN);
	num_underruns++;
	
	/* enlarge buffer if it is not at maximum */
	if ((buffermsec + BUFFER_SIZE_INC <= maximum_msec) && 
	    (num_underruns >= URUN_MAX) &&
	    (buffer_mode == BUFFER_AUTO) && 0)
	{
		buffermsec += BUFFER_SIZE_INC;
		
		// CHECK if prepare is required here
		snd_pcm_drop(pcm_handle);		
		
		snd_pcm_hw_params_free(pcm_hwparams);
		buffermsec = alsa_set_buffer_time(buffermsec);
		
		num_underruns = 0;
	}
}

/* flushes events from an event pipe. tries to recycle proper notes */
void flush_event_pipe(int epipe[2])
{	
	midiev_t newev;
	int status;
	
	while(1)
	{
		status = read(eventpipe[0], &newev, sizeof(newev));
		if (status != sizeof(newev))
			break;
	}
}

/* A main loop that reads events from ALSA, time stamps them and then passes them on */
void * event_startup(void *arg_data)
{
	snd_seq_event_t *seq_ev = NULL;
	midiev_t newev;
	int status;
	
	events_qd = 0;	
	attempt_realtime();
		
	while(1)
	{		
		status = snd_seq_event_input(seq_handle, &seq_ev); 
		if (status < 0)
			continue;
		
		/* Does it need GM->MT mapping */
		if (seq_ev->dest.port == 1)
			if (convert_to_mt(seq_ev))
				continue; /* skip event */
		
		get_msg(seq_ev, &newev);
		gettimeofday(&newev.stamp, NULL);		
				
		status = write(eventpipe[1], &newev, sizeof(newev));
		if ((unsigned int)status != sizeof(newev))
		{
			if (errno == EAGAIN)
			{
				flush_event_pipe(eventpipe);
				report(DRV_NOTEDROP);
			} else
				fprintf(stderr, "stderr: could not sub pipe\n");
		}
		events_qd++;
	}

	return NULL;
}

int init_alsadrv()
{
	pthread_t event_thread;
	int i;
			
	/* initialise buffering */
	buffermsec = minimum_msec;
		
	/* flush the channel map */
	for (i = 0; i < 16; i++)
	{
		channelmap[i] = -1;
		channeluse[i] = 0x0FFFFFFF;
	}
	
	if (recwav_file != NULL)
		write_wav_header(recwav_file);
	
	/* create midi port */
	port_in_mt = alsa_setup_midi();
	if (port_in_mt < 0)
		exit(1);
			
	/* create pcm thread if needed */
	alsa_init_pcm(sample_rate, 2);
		
	/* create communication pipe from alsa reader to processor */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, eventpipe))
	{
		fprintf(stderr, "Could not open IPC socket pair\n");
		fprintf(stderr, "%s\n\n", strerror(errno)); 
		exit(1);
	}
	
	/* Enable non-blocking operation on pipe in both directions */
	if(fcntl(eventpipe[0], F_SETFL, O_NONBLOCK) == -1)
	{
		fprintf(stderr, "Unable to set non-blocking operation on pipe: %s\n",
			strerror(errno));		
	} 
	if(fcntl(eventpipe[1], F_SETFL, O_NONBLOCK) == -1)
	{
		fprintf(stderr, "Unable to set non-blocking operation on pipe: %s\n",
			strerror(errno));		
	}

	pthread_create(&event_thread, NULL, event_startup, NULL);	
	
	/* attempt to increase priority, will only work if user is root */
	attempt_realtime();
	
	/* Create UI command pipe */
	pipe(uicmd_pipe);
	if(fcntl(uicmd_pipe[0], F_SETFL, O_NONBLOCK) == -1)
	{
		fprintf(stderr, "Unable to set non-blocking operation on pipe: %s\n",
			strerror(errno));		
	}
		
	return 0;
}

static bool tryROMFile(const char romDir[], const char filename[], MT32Emu::FileStream &romFile) {
	static const int MAX_PATH_LENGTH = 4096;

	if ((strlen(romDir) + strlen(filename) + 1) > MAX_PATH_LENGTH) {
		return false;
	}
	char romPathName[MAX_PATH_LENGTH];
	strcpy(romPathName, romDir);
	strcat(romPathName, filename);
	return romFile.open(romPathName);
}

static void openROMFile(const char romDir[], const char romFile1[], const char romFile2[], MT32Emu::FileStream &romFile, const char romType[]) {
	switch (rom_search_type) {
	case ROM_SEARCH_TYPE_CM32L_ONLY:
		if (!tryROMFile(romDir, romFile1, romFile)) {
			report(DRV_MT32ROMFAIL, romType);
			exit(1);
		}
		break;
	case ROM_SEARCH_TYPE_MT32_ONLY:
		if (!tryROMFile(romDir, romFile2, romFile)) {
			report(DRV_MT32ROMFAIL, romType);
			exit(1);
		}
		break;
	default:
		if (!tryROMFile(romDir, romFile1, romFile)) {
			if (!tryROMFile(romDir, romFile2, romFile)) {
				report(DRV_MT32ROMFAIL, romType);
				exit(1);
			}
		}
		break;
	}
}

void reload_mt32_core(int rv)
{
	/* delete core if there is already an instance of it */
	if (mt32 != NULL)
	{
		delete sysexHandler;
		delete mt32;
		printf("Restarting MT-32 core\n");
		report(DRV_M32RESET);
	} else 
		printf("Starting MT-32 core\n");
	
	// create ROM images
	MT32Emu::FileStream controlROMFile;
	MT32Emu::FileStream pcmROMFile;

	char romDir[4096];
	if (rom_dir != NULL) {
		strcpy(romDir, rom_dir);
		strcat(romDir, "/");
	} else {
		strcpy(romDir, default_rom_dir);
	}

	openROMFile(romDir, "CM32L_CONTROL.ROM", "MT32_CONTROL.ROM", controlROMFile, "Control");
	openROMFile(romDir, "CM32L_PCM.ROM", "MT32_PCM.ROM", pcmROMFile, "PCM");

	const MT32Emu::ROMImage *controlROMImage = MT32Emu::ROMImage::makeROMImage(&controlROMFile);
	const MT32Emu::ROMImage *pcmROMImage = MT32Emu::ROMImage::makeROMImage(&pcmROMFile);

	/* create MT32Synth object */
	mt32 = new MT32Emu::Synth(mt32ReportHandler);
	if (mt32->open(*controlROMImage, *pcmROMImage, analog_output_mode) == false) {
		report(DRV_MT32FAIL);
		exit(1);
	}
	sysexHandler = new SysexHandler(*mt32);

	MT32Emu::ROMImage::freeROMImage(controlROMImage);
	MT32Emu::ROMImage::freeROMImage(pcmROMImage);

	send_rvmode_sysex(rv_type);
	send_rvtime_sysex(rv_time);
	send_rvlevel_sysex(rv_level);

	mt32->setReverbEnabled(rv);
	mt32->setOutputGain(gain_multiplier);
	mt32->setReverbOutputGain(gain_multiplier);
}

int process_loop(int rv)
{
	unsigned char processbuffer[FRAGMENT_SIZE];
	unsigned int msg;
	struct timeval ot;
	int i, n, pos, csize, size, state;
	signed int total_bytes, bufferused;
	midiev_t newev;
	struct pollfd event_poll;
	int status, cmdid;
	snd_pcm_uframes_t frames;
	snd_pcm_state_t pcmstate;
	
	mt32 = NULL;
	sysexHandler = NULL;
	rv_type = 0;
	rv_time = 5;
	rv_level = 3;
	consumer_types = 0;
	
	reload_mt32_core(rv);
	
	/* setup poll info */
	event_poll.fd = eventpipe[0];
	event_poll.events = POLLIN | POLLPRI;
	
	/* init variables */
	ot = get_time();
	total_bytes = 0;
	csize = 0;

	report(DRV_READY);
	
	/* the pcm output will usually underrun at this point because of the long running
	 * time for the initialisation of the ClassicOpen call */
	bufferused = 0;
	memset(processbuffer, 0, FRAGMENT_SIZE);
		
	/* setup consumers */
	if (recwav_file != NULL) 
	{
		consumer_types |= CONSUME_WAVOUT;
		report(DRV_WAVOUTPUT, 1);
	}
	if (recsyx_file != NULL) 
	{
		consumer_types |= CONSUME_SYSEX;
		report(DRV_SYXOUTPUT, 1);
	}
	
	consumer_types |= CONSUME_PLAYING;
	report(DRV_PLAYING, 1);
	
	snd_pcm_prepare(pcm_handle);	

	while (1) 
	{		
		n = poll(&event_poll, 1, 20);
		if (n < 0)
			return -1;

		if (n > 0)
			read(eventpipe[0], &newev, sizeof(newev));
		else
			get_null_event(&newev);
					
		
		/* flush events till an unsubscribe event is found */
		if (flush_events)
		{
			flush_events = 0;			
			flush_mt32_emu();
			
			/* restart decode cycle */
			continue;
		}
		
		/* check for underrun */
		pcmstate = snd_pcm_state(pcm_handle);
		if (pcmstate == SND_PCM_STATE_XRUN)
		{
			underrun();
			snd_pcm_prepare(pcm_handle);
		}
		
		/* get new time */
		frames = snd_pcm_avail_update(pcm_handle);
		csize = frames << 2;
				
		/* process data till offset */
		while(csize > 0)
		{				
			/* check for commands from the gui */
			status = read(uicmd_pipe[0], &cmdid, sizeof(int));
			if (status == sizeof(int))
				switch(cmdid)
				{
				    case DRVCMD_CLEAR:
					flush_mt32_emu();
					break;
				}			

			size = csize;
			if (size > FRAGMENT_SIZE)
				size = FRAGMENT_SIZE;

			mt32->render((MT32Emu::Bit16s *)processbuffer, size >> 2);

			/* output to WAV file */
			if (consumer_types & CONSUME_WAVOUT)
			{
				total_bytes += size;
				fwrite(processbuffer, 1, size, recwav_file); 	
				
				pos = ftell(recwav_file);									
				fseek(recwav_file, 0x28, SEEK_SET);
				fwrite(&total_bytes, 1, 4, recwav_file);
				fseek(recwav_file, pos, SEEK_SET);
			} 
					      			
			/* output data to sound card buffer */
			if (consumer_types & CONSUME_PLAYING)
				snd_pcm_writei(pcm_handle, processbuffer, size >> 2);
			
			csize -= size;			
		}
		
		/* decide what to do */
		switch(newev.type)
		{
		    case EVENT_MIDI:
			mt32->playMsg(newev.msg);    
			break;
			
		    case EVENT_MIDI_TRIPLET: {
			unsigned int *msg_buffer = (unsigned int *)newev.sysex;
			for(int i = 0; i < 3; i++) {
				mt32->playMsg(msg_buffer[i]);
			}
			delete[] msg_buffer;
			break;
		    }

		    case EVENT_SYSEX:
			/* record it if needed */
			if (consumer_types & CONSUME_SYSEX)
			{
				fwrite((unsigned char *)newev.sysex, 1, newev.sysex_len, recsyx_file);
				fflush(recsyx_file);
			}			
			sysexHandler->parseStream((MT32Emu::Bit8u *) newev.sysex, newev.sysex_len);
			free(newev.sysex);
			break;
		
		    case EVENT_SET_RVMODE:			
			rvsysex[7]  = 1;
			rvsysex[8] = newev.msg;	
			rvsysex[9] = 128-((rvsysex[5]+rvsysex[6]+rvsysex[7]+rvsysex[8])&127);
			mt32->playSysex(rvsysex, 11);
			break;
		    case EVENT_SET_RVTIME:			
			rvsysex[7]  = 2;
			rvsysex[8] = newev.msg;	
			rvsysex[9] = 128-((rvsysex[5]+rvsysex[6]+rvsysex[7]+rvsysex[8])&127);
			mt32->playSysex(rvsysex, 11);
			break;
		    case EVENT_SET_RVLEVEL:			
			rvsysex[7]  = 3;
			rvsysex[8] = newev.msg;	
			rvsysex[9] = 128-((rvsysex[5]+rvsysex[6]+rvsysex[7]+rvsysex[8])&127);
			mt32->playSysex(rvsysex, 11);
			break;
		
		    case EVENT_RESET:
			reload_mt32_core(rv);
			break;
			
		    case EVENT_WAVREC_ON:
			start_recordwav();
			if (recwav_filename != NULL)
				report(DRV_NEWWAV, recwav_filename);
			break;
		    case EVENT_WAVREC_OFF:
			if (recwav_filename != NULL)
			{
				fclose(recwav_file); free(recwav_filename); 
				recwav_file = NULL; recwav_filename = NULL;
				report(DRV_WAVOUTPUT, 0);
				consumer_types ^= CONSUME_WAVOUT;
			}
			break;			

		    case EVENT_SYXREC_ON:
			start_recordsyx();
			if (recsyx_filename != NULL)
				report(DRV_NEWSYX, recsyx_filename);
			break;
		    case EVENT_SYXREC_OFF:
			if (recsyx_filename != NULL)
			{
				consumer_types ^= CONSUME_SYSEX;
				fclose(recsyx_file); free(recsyx_filename);
				recsyx_file = NULL; recsyx_filename = NULL;
				report(DRV_SYXOUTPUT, 0);
			}
			break;			
		}		
	}
	
	return 0;
}
