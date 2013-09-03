/* Copyright (C) 2003 Tristan
 * Copyright (C) 2004, 2005 Tristan, Jerome Fisher
 * Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
 * Copyright (C) 2013 Tristan, Jerome Fisher, Jörg Walter, Sergey V. Mikayev
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

#ifndef DRIVER_REPORTING_H
#define DRIVER_REPORTING_H

void report(int type, ...);

#define DRIVER_REPORT_START 1

/* Driver specific reporting codes */
#define DRV_SUBMT32   DRIVER_REPORT_START + 1
#define DRV_SUBGMEMU  DRIVER_REPORT_START + 2
#define DRV_UNSUB     DRIVER_REPORT_START + 3
	
#define DRV_SYSEXGM   DRIVER_REPORT_START + 4
#define DRV_NOTEDROP  DRIVER_REPORT_START + 5

#define DRV_ERRWAVOUT DRIVER_REPORT_START + 6
#define DRV_MT32FAIL  DRIVER_REPORT_START + 7
#define DRV_ERRPCM    DRIVER_REPORT_START + 8
#define DRV_READY     DRIVER_REPORT_START + 9

#define DRV_UNDERRUN  DRIVER_REPORT_START + 10
#define DRV_M32RESET  DRIVER_REPORT_START + 11
#define DRV_LATENCY   DRIVER_REPORT_START + 12

#define DRV_WAVOUTPUT DRIVER_REPORT_START + 13
#define DRV_PLAYING   DRIVER_REPORT_START + 14
#define DRV_SYXOUTPUT DRIVER_REPORT_START + 15

#define DRV_NEWWAV    DRIVER_REPORT_START + 16
#define DRV_NEWSYX    DRIVER_REPORT_START + 17

#define DRV_MT32ROMFAIL DRIVER_REPORT_START + 18

/* Commands from the front-end to the alsadrv */
extern int uicmd_pipe[2];

extern MT32Emu::ReportHandler *mt32ReportHandler;

#define DRVCMD_NONE   0
#define DRVCMD_CLEAR  1


#endif
	
	
	
	
