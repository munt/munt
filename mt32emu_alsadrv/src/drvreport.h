#ifndef DRIVER_REPORTING_H
#define DRIVER_REPORTING_H

void report(int type, ...);

int MT32Emu_Report(void *userData, MT32Emu::ReportType type, const void *reportData);

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


/* Commands from the front-end to the alsadrv */
extern int uicmd_pipe[2];

#define DRVCMD_NONE   0
#define DRVCMD_CLEAR  1


#endif
	
	
	
	
