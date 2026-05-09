/*
 *	Name:		Dummy music driver
 *	Project:	MUS File Player Library
 *	Version:	1.00
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Jul-13-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Jul-13-1995	V1.00	V.Arnost
 *		Module created
 */

#include "muslib.h"

/* Driver descriptor */
struct driverBlock DUMMYdriver = {
	NULL,				// next
	DRV_DUMMY,			// driverID
	"",				// name
	0,				// datasize
	DUMMYinitDriver,
	DUMMYdeinitDriver,
	DUMMYdriverParam,
	DUMMYloadBank,
	DUMMYdetectHardware,
	DUMMYinitHardware,
	DUMMYdeinitHardware,

	DUMMYplayNote,
	DUMMYreleaseNote,
	DUMMYpitchWheel,
	DUMMYchangeControl,
	DUMMYplayMusic,
	DUMMYstopMusic,
	DUMMYchangeVolume,
	DUMMYpauseMusic,
	DUMMYunpauseMusic,
	DUMMYsendMIDI};


#pragma warn -par	// disable `Parameter xxx is never used' warning
void DUMMYplayNote(struct musicBlock *mus, uint channel, uchar note, int volume)
{}

void DUMMYreleaseNote(struct musicBlock *mus, uint channel, uchar note)
{}

void DUMMYpitchWheel(struct musicBlock *mus, uint channel, int pitch)
{}

void DUMMYchangeControl(struct musicBlock *mus, uint channel, uchar controller, int value)
{}

void DUMMYplayMusic(struct musicBlock *mus)
{}

void DUMMYstopMusic(struct musicBlock *mus)
{}

void DUMMYchangeVolume(struct musicBlock *mus, uint volume)
{}

void DUMMYpauseMusic(struct musicBlock *mus)
{}

void DUMMYunpauseMusic(struct musicBlock *mus)
{}


int DUMMYinitDriver(void)
{
    return 0;
}

int DUMMYdeinitDriver(void)
{
    return 0;
}

int DUMMYdriverParam(uint message, uint param1, void *param2)
{
    return 0;
}

int DUMMYloadBank(int fd, uint bankNumber)
{
    return 0;
}

int DUMMYdetectHardware(uint port, uchar irq, uchar dma)
{
    return -1;
}

int DUMMYinitHardware(uint port, uchar irq, uchar dma)
{
    return 0;
}

int DUMMYdeinitHardware(void)
{
    return 0;
}

int DUMMYsendMIDI(uint command, uint par1, uint par2)
{
    return 0;
}
