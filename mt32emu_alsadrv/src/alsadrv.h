#ifndef MT32_ALSA_DRV_H
#define MT32_ALSA_DRV_H

#include <mt32emu/mt32emu.h>

typedef struct {
	unsigned char type;	
	
	unsigned int msg;
	struct timeval stamp;

	void *sysex;
	unsigned short sysex_len;	
} midiev_t;


/* Buffering info */
#define BUFFER_AUTO      0
#define BUFFER_MANUAL    1

/* Consumer info */
#define CONSUME_PLAYING      1
#define CONSUME_WAVOUT       2
#define CONSUME_SYSEX        4

/* Event packages */
#define EVENT_MIDI           0
#define EVENT_NONE           1
#define EVENT_SYSEX          2
#define EVENT_UNSUBSCRIBED   3
#define EVENT_SET_RVMODE     4
#define EVENT_SET_RVTIME     5
#define EVENT_SET_RVLEVEL    6
#define EVENT_RESET          7
#define EVENT_WAVREC_ON      8
#define EVENT_WAVREC_OFF     9
#define EVENT_SYXREC_ON      10
#define EVENT_SYXREC_OFF     11

extern int minimum_msec;
extern int maximum_msec;
extern int buffer_mode;
extern int buffermsec;

extern int consumer_types;

/* Reverb info */
extern int rv_type;
extern int rv_time;
extern int rv_level;

/* aux-output info */
extern char *recsyx_filename;
extern FILE *recsyx_file;
extern char *recwav_filename;
extern FILE *recwav_file;


void send_rvmode_sysex(int newmode);
void send_rvtime_sysex(int newtime);
void send_rvlevel_sysex(int newlevel);

extern int alsa_buffer_size;
extern char rom_path[];
extern int eventpipe[];

int init_alsadrv();
int process_loop(int rv);

extern MT32Emu::Synth *mt32;



#endif


