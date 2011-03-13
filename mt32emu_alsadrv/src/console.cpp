/* Copyright (C) 2003 Tristan
 * Copyright (C) 2004, 2005 Tristan, Jerome Fisher
 * Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
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
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include <mt32emu/mt32emu.h>

#include "alsadrv.h"
#include "drvreport.h"

int MT32Emu_Report(void *userData, MT32Emu::ReportType type, const void *reportData)
{
	switch (type)
	{
	case MT32Emu::ReportType_errorControlROM:
		fprintf(stderr, "Unable to open %sMT32_Control.ROM: %d\n", rom_path, *((int *)reportData));
		break;

	case MT32Emu::ReportType_errorPCMROM:
		fprintf(stderr, "Unable to open %sMT32_PCM.ROM: %d\n", rom_path, *((int *)reportData));
		break;
	
	case MT32Emu::ReportType_lcdMessage:
		printf("LCD: %s\n", (char *)reportData);
		break;
	
	case MT32Emu::ReportType_newReverbMode:
		rv_type  = *((MT32Emu::Bit8u *)reportData);
		break;
	case MT32Emu::ReportType_newReverbTime:
		rv_time  = *((MT32Emu::Bit8u *)reportData);
		break;
	case MT32Emu::ReportType_newReverbLevel:
		rv_level = *((MT32Emu::Bit8u *)reportData);
		break;

	case MT32Emu::ReportType_usingSSE:
		printf("SSE detected and enabled\n");
		break;
		
	case MT32Emu::ReportType_using3DNow:
		printf("3DNow detected and enabled\n");
		break;
	}
	return 0;
}

void report(int type, ...)
{
	va_list ap;
	
	va_start(ap, type);
	
	switch(type)
	{
	case DRV_SUBMT32:
		printf("%s subscribed to MT-32 port\n", va_arg(ap, char *));
		break;

	case DRV_SUBGMEMU:
		printf("%s subscribed to GM emulation port\n", va_arg(ap, char *));
		break;
	    
	case DRV_UNSUB:
		printf("Client unsubscribed\n");
		break;

	case DRV_NOTEDROP:
		printf("Note dropped, queue overfull\n");
		break;

	case DRV_SYSEXGM:
		printf("Sysex message recieved on GM emulation port\n");
		break;

	case DRV_ERRWAVOUT:
		fprintf(stderr, "Could not open wave file: %s\n", va_arg(ap, char *));
		break;

	case DRV_MT32FAIL:
		fprintf(stderr, "MT-32 failed to load\n");
		break;

	case DRV_ERRPCM:
		fprintf(stderr, "Could not open pcm device: %s\n", va_arg(ap, char *));
		break;

	case DRV_READY:
		printf("MT-32 emulator ready\n");
		break;

	case DRV_UNDERRUN:
		printf("Output buffer underrun\n");
		break;
	}
	
	va_end(ap);
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
	
	printf("\n");
	exit(1);
}

int main(int argc, char **argv) 
{	
	int reverb_switch = 1;
	int i;
	
	/* parse the options */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
			usage(argv);
		
		switch(argv[i][1])
		{
		    case 'w':
			i++;
			if (i == argc) 
				usage(argv);
			recwav_filename = (char *)malloc(strlen(argv[i]) + 1);
			strcpy(recwav_filename, argv[i]);
			recwav_file = fopen(recwav_filename, "w");
			if (recwav_file == NULL)
			{
				fprintf(stderr, "Could not open wave output file %s\n", recwav_filename);
				fprintf(stderr, "%s\n\n", strerror(errno));
				exit(1);
			}
			break;
		    
		    case 'e':
			i++;
			if (i == argc) 
				usage(argv);
			recsyx_filename = (char *)malloc(strlen(argv[i]) + 1);
			strcpy(recsyx_filename, argv[i]);
			recsyx_file = fopen(recsyx_filename, "w");
			if (recsyx_file == NULL)
			{
				fprintf(stderr, "Could not open sysex output file %s\n", recsyx_filename);
				fprintf(stderr, "%s\n\n", strerror(errno));
				exit(1);
			}
			break;
		
		    case 'r': reverb_switch = 1; break;			
		    case 'n': reverb_switch = 0; break;

		    case 'a': buffer_mode = BUFFER_AUTO;   break;
		    case 'm': buffer_mode = BUFFER_MANUAL; break;

		    case 'x': i++; if (i == argc) usage(argv);
			maximum_msec = atoi(argv[i]);
			break;
		    case 'i': i++; if (i == argc) usage(argv);
			minimum_msec = atoi(argv[i]);
			break;
			
		    default:
			usage(argv);
		}
	}
		
	if (init_alsadrv() == -1)
	{
		fprintf(stderr, "Could not init!!!\n");
		exit(1);
	}
	
	process_loop(reverb_switch);
	
	return 0;
}
