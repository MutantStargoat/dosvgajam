/*
 *	Name:		MUS File Player Library API
 *	Project:	MUS File Player Library
 *	Version:	1.02
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Sep-4-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Jul-13-1995	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-12-1995	V1.01	V.Arnost
 *		Added mus->channelcount field in MLloadMUS()
 *	Sep-4-1995	V1.02	V.Arnost
 *		Added MLinit() parameter `instance'
 */

#include <io.h>
#include <stdlib.h>
#include "muslib.h"

struct musicBlock *MLmusicBlocks[MAXMUSBLOCK] = {NULL};
struct driverBlock *MLdriverList = &DUMMYdriver;


#pragma argsused
void MLinit(uint instance)
{
#ifdef __WINDOWS__
    MLinstance = instance;
#endif
    MEMdetect();
    atexit(MLdeinit);
}

void MLdeinit(void)
{
    uint i;
    struct driverBlock *drv;

    MLshutdownTimer();

    for (i = 0; i < MAXMUSBLOCK; i++)
	if (!MLmusicBlocks[i])
	{
//	    MLfreeMUS(i);
	    MLfreeHandle(i);
	}

    drv = MLdriverList;
    while (drv)
    {
	if (drv->state & 2)
	    drv->deinitHardware();
	if (drv->state & 1)
	    drv->deinitDriver();
	drv->state = 0;
	drv = drv->next;
    }
}

struct driverBlock *MLfindDriver(uint driverID)
{
    struct driverBlock *drv = MLdriverList;
    while (drv)
    {
	if (drv->driverID == driverID)
	    return drv;
	drv = drv->next;
    }
    return NULL;
}

int MLaddDriver(struct driverBlock *newDriver)
{
    if (MLfindDriver(newDriver->driverID))
	return -1;			/* driver ID already present */
    newDriver->next = MLdriverList;
    MLdriverList = newDriver;
//    return 0;
    newDriver->state |= 1;
    return newDriver->initDriver();
}

int MLinitDriver(uint driverID)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    drv->state |= 1;
    return drv->initDriver();
}

int MLdeinitDriver(uint driverID)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    drv->state &= ~1;
    return drv->deinitDriver();
}

int MLdriverParam(uint driverID, uint message, uint param1, void *param2)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    return drv->driverParam(message, param1, param2);
}

int MLdetectHardware(uint driverID, short port, short irq, short dma)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    return drv->detectHardware(port, irq, dma);
}

int MLinitHardware(uint driverID, short port, short irq, short dma)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    drv->state |= 2;
    return drv->initHardware(port, irq, dma);
}

int MLdeinitHardware(uint driverID)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    drv->state &= ~2;
    return drv->deinitHardware();
}

int MLloadBank(uint driverID, int fd, uint bankNumber)
{
    struct driverBlock *drv;
    if ((drv = MLfindDriver(driverID)) == NULL)
	return -1;			/* invalid driver ID */
    return drv->loadBank(fd, bankNumber);
}

int MLallocHandle(uint driverID)
{
    struct driverBlock *drv;
    uint i;

    if ((drv = MLfindDriver(driverID)) == NULL)
	return -2;			/* invalid driver ID */
    for (i = 0; i < MAXMUSBLOCK; i++)
	if (!MLmusicBlocks[i])
	{
	    struct musicBlock *mus;
	    if ((mus = (struct musicBlock *)calloc(1, sizeof(struct musicBlock))) == NULL)
		return -3;		/* not enough memory */
	    mus->state = ST_EMPTY;
	    mus->number = i;
	    mus->driver = drv;
	    if ((mus->driverdata = calloc(1, drv->datasize)) == NULL)
	    {
		free(mus);
		return -3;		/* not enough memory */
	    }
	    mus->volume = 256;
	    mus->channelMask = -1U;
	    mus->percussMask = 1 << PERCUSSION;
	    MLmusicBlocks[i] = mus;
	    return i;
	}
    return -1;				/* no free handle */
}

static void _MLstop(struct musicBlock *mus)
{
    if (mus->state >= ST_PLAYING)
    {
	mus->state = ST_STOPPED;
	mus->driver->stopMusic(mus);
    }
}

#define VALIDHANDLE(h,mus) ((h) < MAXMUSBLOCK && ((mus) = MLmusicBlocks[(h)]) != NULL)

int MLfreeHandle(uint musHandle)
{
    struct musicBlock *mus;
    if (VALIDHANDLE(musHandle, mus))
    {
	_MLstop(mus);
	MEMfree(&mus->score);
	free(mus->driverdata);
	free(mus);
	MLmusicBlocks[musHandle] = NULL;
	return 0;
    } else
	return -1;
}

#define CHAR2LONG(a,b,c,d) ((DWORD)(a) | ((DWORD)(b) << 8) | ((DWORD)(c) << 16) | ((DWORD)(d) << 24))

int MLloadMUS(uint musHandle, int fd, uint memoryFlags)
{
    struct musicBlock *mus;
    struct MUSheader header;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    if (read(fd, &header, sizeof header) != sizeof header)
	return -2;			/* file read error */

    if (*((DWORD *)&header.ID) != CHAR2LONG('M','U','S',0x1A))
	return -3;			/* invalid file header */

    mus->channelcount = header.channels;

    lseek(fd, header.scoreStart - sizeof header, SEEK_CUR);

    if (MEMload(fd, header.scoreLen, &mus->score, memoryFlags))
	return -4;			/* not enough memory */

    mus->state = ST_STOPPED;
    return 0;
}

int MLfreeMUS(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    _MLstop(mus);
    if (MEMfree(&mus->score))
	return -2;			/* memory block error */

    mus->state = ST_EMPTY;
    return 0;
}

int MLplay(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    switch (mus->state) {
	case ST_EMPTY:
	    break;
//	case ST_PLAYING:
//	case ST_PAUSED:
	default:
	    mus->state = ST_STOPPED;
	    mus->driver->stopMusic(mus);
	case ST_STOPPED:
	    MEMrewind(&mus->score);
	    mus->driver->playMusic(mus);
	    mus->state = ST_PLAYING;
    }
    return 0;
}

int MLstop(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    _MLstop(mus);
    return 0;
}

int MLsetVolume(uint musHandle, uint volume)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    mus->volume = volume;
    mus->driver->changeVolume(mus, volume);
    return 0;
}

int MLgetVolume(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    return mus->volume;
}

int MLpause(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    switch (mus->state) {
	case ST_EMPTY:
	case ST_STOPPED:
	    break;
	case ST_PLAYING:
	    mus->state++;
	    mus->driver->pauseMusic(mus);
	    break;
//	case ST_PAUSED:
	default:
	    mus->state++;
    }
    return 0;
}

int MLunpause(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */

    switch (mus->state) {
	case ST_EMPTY:
	case ST_STOPPED:
	case ST_PLAYING:
	    break;
	case ST_PAUSED:
	    mus->driver->unpauseMusic(mus);
	default:
	    mus->state--;
    }
    return 0;
}

int MLgetState(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */
    return mus->state;
}

struct musicBlock *MLgetBlock(uint musHandle)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return NULL;			/* invalid music handle */
    return mus;
}

int MLsetLoopCount(uint musHandle, int count)
{
    struct musicBlock *mus;

    if (!VALIDHANDLE(musHandle, mus))
	return -1;			/* invalid music handle */
    mus->loopcount = count;
    return 0;
}

