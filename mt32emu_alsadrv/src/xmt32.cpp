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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

/* Front-end stuff */
#include "keypad.h"
#include "widgets.h"
#include "lcd.h"

/* Emulator stuff */
#include <mt32emu/mt32emu.h>
#include "alsadrv.h"
#include "drvreport.h"


/* General X variables */
Display  *dpy = NULL;
Window   rootWindow, mainWindow;
GC       mainGC;
Colormap mainCM;
int      mainScreen;
int      rootDepth;
Atom     atomDel, atomProtocols;


/* Colours */
XColor LCDColour;
XColor LCDTextColour;
XColor LCDSoftColour, LCDHardColour;
XColor edgeColour;
XColor buttonColour;


#define SETCOLOUR(name, cr, cg, cb) (name).red = cr << 8; \
	(name).green = cg << 8; \
	(name).blue  = cb << 8; \
	XAllocColor(dpy, mainCM, &(name))


/* message pipe */
int reportpipe[2];


/* Some settings */
int xreverb_switch = 1;


void setup_wm()
{
	Atom winType, winTypeDock, winPid;
	XClassHint wmClass;
	pid_t xguipid;
		
	wmClass.res_name  = "mt32emu";
	wmClass.res_class = "mt32emu";
	XSetClassHint(dpy, mainWindow, &wmClass);
	XStoreName(dpy, mainWindow, "MT-32 Emulator");
	
	xguipid = getpid();
	winPid = XInternAtom(dpy, "_NET_WM_PID", True);
	if (winPid != None)
		XChangeProperty(dpy, mainWindow, winPid, XA_CARDINAL, 32, 
				PropModeReplace, (unsigned char *)&xguipid, 1);
	
	atomDel = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	atomProtocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	XSetWMProtocols(dpy, mainWindow, &atomDel, 1);
	
	
/*	
	winType = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", True);
	winTypeDock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", True);
	if ((winType != None) && (winTypeDock != None))
	{		
		XChangeProperty(dpy, mainWindow, winType, 
				XA_ATOM, 32, PropModeReplace, 
				(unsigned char *)&winTypeDock, 1);
	}	
	
	winType = XInternAtom(dpy, "_NET_WM_STATE", True);
	winTypeDock = XInternAtom(dpy, "_NET_WM_STATE_STICKY", True);
	if ((winType != None) && (winTypeDock != None))
	{		
		XChangeProperty(dpy, mainWindow, winType, 
				XA_ATOM, 32, PropModeReplace, 
				(unsigned char *)&winTypeDock, 1);
		printf("Set window state atom\n");
	}	
*/			
}

void alloc_colours()
{
	SETCOLOUR(LCDColour,     0xED, 0x7D, 0x0C);
	SETCOLOUR(LCDTextColour, 0x00, 0x00, 0x00);
	SETCOLOUR(LCDSoftColour, 0xC6, 0x69, 0x0A);
	SETCOLOUR(LCDHardColour, 0x76, 0x3E, 0x06);
	SETCOLOUR(edgeColour,    0x27, 0x2D, 0x35);
	SETCOLOUR(buttonColour,  0xC1, 0xC1, 0xC1);
}

int create_display()
{
	XSetWindowAttributes attrib;
	unsigned long atmask;
	XGCValues gcvalues;
	Pixmap backpix;
	
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		return -1;
		
	rootWindow = DefaultRootWindow(dpy);
	create_pixmaps();
	
	/* setup window stuff */
	atmask = CWBackPixmap | CWBorderPixel | CWEventMask; // | CWOverrideRedirect | CWBackingStore;
	
	attrib.background_pixmap = pix_stripe;
	attrib.border_pixel = 0;
	attrib.override_redirect = True;
	attrib.backing_store = 1;	
	attrib.event_mask = KeyPressMask | ButtonPressMask | ExposureMask | PropertyChangeMask;
	
	mainWindow = XCreateWindow(dpy, rootWindow, 
				   0, 0, WINDOW_W, WINDOW_H, 0, CopyFromParent,
				   InputOutput, CopyFromParent, 
				   atmask, &attrib);
			
	setup_wm();
	XMapWindow(dpy, mainWindow);		
	XSync(dpy, 0);
	
	
	/* create a new Graphical Context */
	gcvalues.foreground = 0xFF;
	gcvalues.background = 0x0;
			
	mainGC = XCreateGC(dpy, mainWindow, GCForeground | GCBackground, &gcvalues);
	mainScreen = DefaultScreen(dpy);
	rootDepth = DefaultDepth(dpy, mainScreen);
	
	/* Setup colours */
	mainCM = DefaultColormap(dpy, mainScreen);
	alloc_colours();
	
	return 0;
}

void blank_button_set(button_set_t *bs)
{
	int i;
	
	for (i = 0; i < 9; i++)
	{
		bs->buttons[i].p  = pix_button;
		bs->buttons[i].w  = button_width;
		bs->buttons[i].h  = button_height;
		bs->buttons[i].cb = NULL;
	}
}

void redraw_keypad()
{
	int i, x, y;
	
	/* draw button pad */
	for (y = 0; y < 3; y++)
		for (x = 0; x < 3; x++, i++)
		{
			i = x * 3 + y;
			
			XCopyArea(dpy, cset->buttons[i].p, mainWindow, mainGC, 
				  0, 0, BUTTON_W, BUTTON_H,		
				  BUTTONPAD_X + x * BUTTON_W, 
				  BUTTONPAD_Y + y * BUTTON_H); 
		}	
}

void redraw_display()
{		
	/* Draw border */
	XSetForeground(dpy, mainGC, edgeColour.pixel);
	XDrawLine(dpy, mainWindow, mainGC, 0,   0, 0,   WINDOW_H);
	XDrawLine(dpy, mainWindow, mainGC, WINDOW_W - 1, 0, WINDOW_W - 1, WINDOW_H);
	XDrawLine(dpy, mainWindow, mainGC, 0,   0, WINDOW_W,   0);
	XDrawLine(dpy, mainWindow, mainGC, 0,  WINDOW_H - 1, WINDOW_W,  WINDOW_H - 1);
		
	/* draw display border */
	XFillRectangle(dpy, mainWindow, mainGC, 
		       DISPLAY_X - 2, DISPLAY_Y - 2, 
		       DISPLAY_W + 4, DISPLAY_H + 4);

	/* draw display */
	lcd_redraw();
	
	/* put in logo */
	XCopyArea(dpy, pix_logo, mainWindow, mainGC, 
		  0, 0, logo_width, logo_height, 
		  (DISPLAY_X + DISPLAY_W) + (WINDOW_W - (DISPLAY_X + DISPLAY_W) - logo_width) / 2, 4);
	
	redraw_keypad();
}

static class : public MT32Emu::ReportHandler {
protected:
	virtual void onErrorControlROM() {
		fprintf(stderr, "Unable to open MT32 Control ROM file");
		notify();
	}

	virtual void onErrorPCMROM() {
		fprintf(stderr, "Unable to open MT32 PCM ROM file");
		notify();
	}

	virtual void showLCDMessage(const char *message) {
		sysex_lcd_message(message);
		notify();
	}

	//virtual void printDebug(const char *fmt, va_list list) {}

	virtual void onNewReverbMode(MT32Emu::Bit8u mode) {
		rv_type = mode;
		redraw_keypad();
		notify();
	}

	virtual void onNewReverbTime(MT32Emu::Bit8u time) {
		rv_time = time;
		redraw_keypad();
		notify();
	}

	virtual void onNewReverbLevel(MT32Emu::Bit8u level) {
		rv_level = level;
		redraw_keypad();
		notify();
	}

private:
	void notify() {
		/* send notification */
		char c = 1;
		write(reportpipe[1], &c, 1);
	}
} mt32ReportHandlerImpl;
MT32Emu::ReportHandler *mt32ReportHandler = &mt32ReportHandlerImpl;

/* message system */
void report(int type, ...)
{
	char tmpstr[128], c;
	va_list ap;
	
	va_start(ap, type);
	
	switch(type)
	{
	case DRV_SUBMT32:
		sprintf(tmpstr, "MT-32: %s subscribed", va_arg(ap, char *));
		general_lcd_message(tmpstr);
		break;

	case DRV_SUBGMEMU:
		sprintf(tmpstr, "GM: %s subscribed", va_arg(ap, char *));
		general_lcd_message(tmpstr);
		break;
	    
	case DRV_UNSUB:
		general_lcd_message("Client unsubscribed");
		break;

	case DRV_NOTEDROP:
		general_lcd_message("Note dropped, queue overfull");
		break;

	case DRV_SYSEXGM:
		printf("Sysex message recieved on GM emulation port\n");
		break;

	case DRV_ERRWAVOUT:
		fprintf(stderr, "Could not open wave file: %s", va_arg(ap, char *));
		break;
		
	case DRV_MT32FAIL:
		fprintf(stderr, "MT-32 failed to load");
		break;

	case DRV_MT32ROMFAIL:
		fprintf(stderr, "Unable to open MT32 %s ROM file\n", va_arg(ap, char *));
		break;

	case DRV_ERRPCM:
		fprintf(stderr, "Could not open pcm device: %s", va_arg(ap, char *));
		break;
	    
	case DRV_READY:
		general_lcd_message("MT-32 emulator ready");
		break;
		
	case DRV_UNDERRUN:
		general_lcd_message("Output buffer underrun");
		break;
		
	case DRV_LATENCY:
		lcd_msec = va_arg(ap, int); lcd_redraw();
		break;
	
	case DRV_WAVOUTPUT: lcd_flags[0] = (va_arg(ap, int)) ? 'W'|(1<<7) : 'W'; lcd_redraw(); break;
	case DRV_PLAYING:   lcd_flags[1] = (va_arg(ap, int)) ? 'P'|(1<<7) : 'P'; lcd_redraw(); break;
	case DRV_SYXOUTPUT: lcd_flags[2] = (va_arg(ap, int)) ? 'X'|(1<<7) : 'X'; lcd_redraw(); break;
		
	case DRV_NEWWAV:
		sprintf(tmpstr, "Wave output in %s", va_arg(ap, char *));
		general_lcd_message(tmpstr);
		break;
	case DRV_NEWSYX:
		sprintf(tmpstr, "Sysex output in %s", va_arg(ap, char *));
		general_lcd_message(tmpstr);
		break;		
	}
	
	va_end(ap);
	
	/* send notification */
	c = 1; write(reportpipe[1], &c, 1);
}

void quit()
{
	printf("Bye\n");
	exit(0);
}

int display_up = 0;       

void *xdisplay_main(void *indata)
{
	XEvent event;
	XClientMessageEvent *cmev = (XClientMessageEvent *)&event;
	struct pollfd evs[2];
	int x, y, status, age;
	char c;
		
	if (create_display() == -1)
	{
		fprintf(stderr, "Could not open X display\n");
		exit(1);
	}
		
	lcd_setup();
	create_buttonsets();
		
	/* setup initial button set */
	cset = &mainmenu;
	redraw_display();
	
	/* setup to poll for both an X-event as well as a message pipe */
	evs[0].fd = XConnectionNumber(dpy);
	evs[0].events = POLLIN | POLLPRI | POLLHUP;
	evs[1].fd = reportpipe[0];
	evs[1].events = POLLIN | POLLPRI | POLLHUP;
	
	/* annouce that the gui is up */
	display_up = 1;		
	
	while(1)
	{		
		/* do LCD aging */
		age = lcd_age();
		if (age != -1) age *= 1000;
		
		XFlush(dpy);		
		status = poll((struct pollfd *)evs, 2, age);
		
		switch(status)
		{
		    case 0:
			continue;
		    case -1:
			// fprintf(stderr, "Could not poll (this is badness:( ): %s\n", 
			//	strerror(errno));
			continue;
		}
		
		// check message pipe 
		if (evs[1].revents)
			read(reportpipe[0], &c, 1);
		
		if (!evs[0].revents)
			continue;
		
		while (XCheckMaskEvent(dpy, ~0, &event)) {
			switch(event.type)
			{
			    case ButtonPress:

				x = event.xbutton.x;
				y = event.xbutton.y;
						
				/* is it the button pad */
				if ((x > BUTTONPAD_X) && (x < BUTTONPAD_W + BUTTONPAD_X) &&
				    (y > BUTTONPAD_Y) && (y < BUTTONPAD_H + BUTTONPAD_Y))
				{
					x -= BUTTONPAD_X;
					y -= BUTTONPAD_Y;

					/* which button */
					x = x / BUTTON_W;
					y = y / BUTTON_H;

					if (cset->buttons[x * 3 + y].cb != NULL)
						cset->buttons[x * 3 + y].cb(event.xbutton.button);
					break;
				}

				break;

			    case Expose:
				redraw_display();
				break;		

			    case ClientMessage:
				if (atomProtocols == cmev->message_type)
					if(cmev->data.l[0] == atomDel)
						quit();

				break;			
			}
		}
	}	
	
	return NULL;
}

void usage(char *argv[])
{
	printf("Usage: %s [options] \n", argv[0]);
	printf("-w filename  : Wave output (.wav file)\n");
	printf("-e filename  : Sysex patch output (.syx file)\n");
	printf("-r           : Enable reverb (default)\n");
	printf("-n           : Disable reverb \n");
	
	printf("\n");
	printf("-m           : Manual buffering mode (buffer does not grow)\n");
	printf("-a           : Automatic buffering mode (default)\n");
	printf("-x msec      : Maximum buffer size in milliseconds\n");
	printf("-i msec      : Minimum (initial) buffer size in milliseconds\n");
	
	printf("\n");
	printf("-d name      : ALSA PCM device name (default: \"default\")\n");

	printf("\n");
	printf("-g factor    : Gain multiplier (default: 1.0) \n");
	printf("-l mode      : Analog emulation mode (0 - Digital, 1 - Coarse,\n"
	       "               2 - Accurate, 3 - Oversampled 2x, default: 2)\n");

	printf("\n");
	printf("-f romdir    : Directory with ROM files to load\n"
	       "               (default: '/usr/share/mt32-rom-data/')\n");
	printf("-o romsearch : Search algorithm to use when loading ROM files:\n"
	       "               (0 - try both but CM32-L first, 1 - CM32-L only,\n"
	       "                2 - MT-32 only, default: 0)\n");

	printf("\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	pthread_t visthread;
	int i;

	/* parse the options */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
			usage(argv);
		
		switch(argv[i][1])
		{
		    case 'w':
			i++; if (i == argc) usage(argv);
			recwav_filename = (char *)malloc(strlen(argv[i]) + 1);
			strcpy(recwav_filename, argv[i]);
			recwav_file = fopen(recwav_filename, "w");
			if (recwav_file == NULL)
			{
				fprintf(stderr, "Could not open wave output file %s\n", recwav_filename);
				fprintf(stderr, "%s\n\n", strerror(errno));
				exit(1);
			} break;
			
		    case 'e':
			i++; if (i == argc) usage(argv);
			recsyx_filename = (char *)malloc(strlen(argv[i]) + 1);
			strcpy(recsyx_filename, argv[i]);
			recsyx_file = fopen(recsyx_filename, "w");
			if (recsyx_file == NULL)
			{
				fprintf(stderr, "Could not open sysex output file %s\n", recsyx_filename);
				fprintf(stderr, "%s\n\n", strerror(errno));
				exit(1);
			} break;
			
		    case 'r': xreverb_switch = 1; break;
		    case 'n': xreverb_switch = 0; break;
			
		    case 'a': buffer_mode = BUFFER_AUTO;   break;
		    case 'm': buffer_mode = BUFFER_MANUAL; break;
			
		    case 'x': i++; if (i == argc) usage(argv);
			maximum_msec = atoi(argv[i]);
			break;
		    case 'i': i++; if (i == argc) usage(argv);
			minimum_msec = atoi(argv[i]);
			break;
		    case 'd': i++; if (i == argc) usage(argv);
			pcm_name = (char *)malloc(strlen(argv[i]) + 1);
			strcpy(pcm_name, argv[i]);
			break;
		    case 'g': i++; if (i == argc) usage(argv);
			gain_multiplier = atof(argv[i]);
			break;
		    case 'l': i++; if (i == argc) usage(argv);
			analog_output_mode = MT32Emu::AnalogOutputMode(atoi(argv[i]));
			if (analog_output_mode < MT32Emu::AnalogOutputMode_DIGITAL_ONLY
					|| MT32Emu::AnalogOutputMode_OVERSAMPLED < analog_output_mode) usage(argv);
			sample_rate = MT32Emu::Synth::getStereoOutputSampleRate(analog_output_mode);
			break;

		    case 'f': i++; if (i == argc) usage(argv);
			rom_dir = new char[strlen(argv[i]) + 1];
			strcpy(rom_dir, argv[i]);
			break;
		    case 'o': i++; if (i == argc) usage(argv);
			rom_search_type = rom_search_type_t(atoi(argv[i]));
			if (rom_search_type < ROM_SEARCH_TYPE_DEFAULT
					|| ROM_SEARCH_TYPE_MT32_ONLY < rom_search_type) usage(argv);
			break;
			
		    default:
			usage(argv);
		}
	}
		
	/* setup stuff for communication */
	pipe(reportpipe);	

	/* first try and open port */
	if (init_alsadrv() == -1)
	{
		fprintf(stderr, "Could not init!!!\n");
		exit(1);
	}
		
	pthread_create(&visthread, NULL, xdisplay_main, NULL);
	
	/* wait and start process loop */
	while(!display_up)
	{
		usleep(10000);
	}
	
	process_loop(xreverb_switch);
	
	return 0;
}
