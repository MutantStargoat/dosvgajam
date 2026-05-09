/*
 *	Name:		MPU-401 (UART mode) General MIDI music driver
 *	Project:	MUS File Player Library
 *	Version:	1.10
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Mar-08-1996
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-14-1995	V1.00	V.Arnost
 *		Module created
 *	Mar-08-1996	V1.10	V.Arnost
 *		Added running status handling to save MIDI-port bandwidth
 */

#include <dos.h>
#include <conio.h>
#include "muslib.h"


/* Driver descriptor */
static char MPU401name[] = "MPU-401 MIDI";

struct driverBlock MPU401driver = {
	NULL,				// next
	DRV_MPU401,			// driverID
	MPU401name,			// name
	sizeof(struct MIDIdata),	// datasize
	MIDIinitDriver,
	MIDIdeinitDriver,
	MPU401driverParam,
	MPU401loadBank,
	MPU401detectHardware,
	MPU401initHardware,
	MPU401deinitHardware,

	MIDIplayNote,
	MIDIreleaseNote,
	MIDIpitchWheel,
	MIDIchangeControl,
	MIDIplayMusic,
	MIDIstopMusic,
	MIDIchangeVolume,
	MIDIpauseMusic,
	MIDIunpauseMusic,
	MPU401sendMIDI};


static uint MPU401port = MPU401PORT;
static uint runningStatus = 0;

/* MPU401 commands: (out: port 331h) */
#define MPU401_SET_UART	(uchar)0x3F	// set UART mode
#define MPU401_RESET	(uchar)0xFF	// reset MIDI device

// MPU401 status codes: (in: port 331h)
#define MPU401_BUSY	(uchar)0x40	// 1:busy, 0:ready to receive a command
#define MPU401_EMPTY	(uchar)0x80	// 1:empty, 0:buffer is full
#define MPU401_ACK	(uchar)0xFE	// command acknowledged


/* write one byte to MPU-401 data port */
static FASTCALL int MPU401sendByte(uchar value)
{
    register uint timeout = 10000;
    register uint statusport = MPU401port + 1;

    /* wait until the device is ready */
    for(;;)
    {
	uint delay;
	uchar status = inp(statusport);	/* read status port */
	if ((status & MPU401_BUSY) == 0)
	    break;
	if (status & MPU401_EMPTY)
	    continue;
	_enable();
	for (delay = 100; delay; delay--);
	inp(MPU401port);		/* discard incoming data */
	if (--timeout == 0)
	    return -1;			/* port is not responding: timeout */
    }

    outp(MPU401port, value);		/* write value to data port */
    return 0;
}

/* write block of bytes to MPU-401 port */
static int MPU401sendBlock(uchar *block, uint length)
{
    runningStatus = 0;			/* clear the running status byte */

    _disable();
    while (length--)
	MPU401sendByte(*block++);
    _enable();
    return 0;
}

/* write one byte to MPU-401 command port */
static FASTCALL int MPU401sendCommand(uchar value)
{
    register uint timeout;
    register uint statusport = MPU401port + 1;

    runningStatus = 0;			/* clear the running status byte */

    /* wait until the device is ready */
    for(timeout = 0xFFFF;;)
    {
	if ((inp(statusport) & MPU401_BUSY) == 0) /* read status port */
	    break;
	if (--timeout == 0)
	    return -1;			/* port is not responding: timeout */
    }

    outp(statusport, value);		/* write value to command port */

    /* wait for acknowledging the command */
    for(timeout = 0xFFFF;;)
    {
	if ((inp(statusport) & MPU401_EMPTY) == 0) /* read status port */
	    if (inp(MPU401port) == MPU401_ACK)
		break;
	if (--timeout == 0)
	    return -1;			/* port is not responding: timeout */
    }

    return 0;
}

/* reset MPU-401 port */
static int MPU401reset(void)
{
    runningStatus = 0;			/* clear the running status byte */

    if (!MPU401sendCommand(MPU401_RESET))	/* first trial */
	return 0;
    return MPU401sendCommand(MPU401_RESET);	/* second trial */
}


/* send MIDI command */
int MPU401sendMIDI(uint command, uint par1, uint par2)
{
    register uint event = command & MIDI_EVENT_MASK;

    if (event == MIDI_NOTE_ON)
	playingChannels++;
    else if (event == MIDI_NOTE_OFF)
    {
	playingChannels--;
	/* convert NOTE_OFF to NOTE_ON with zero velocity */
	command = (command & 0x0F) | MIDI_NOTE_ON;
	par2 = 0; /* velocity */
    }

    _disable();
    if (runningStatus != command)
	MPU401sendByte(runningStatus = command);
    MPU401sendByte(par1);
    if (event != MIDI_PATCH && event != MIDI_CHAN_TOUCH)
	MPU401sendByte(par2);
    _enable();
    return 0;
}

int MPU401driverParam(uint message, uint param1, void *param2)
{
    switch (message) {
	case DP_SYSEX:
	    MPU401sendBlock((uchar *)param2, param1);
	    break;
    }
    return 0;
}

#pragma argsused
int MPU401loadBank(int fd, uint bankNumber)
{
    return 0;
}

#pragma argsused
int MPU401detectHardware(uint port, uchar irq, uchar dma)
{
    register uint savedMPU401port = MPU401port;
    register uint result;

    MPU401port = port;
    result = MPU401reset() + 1;
    MPU401port = savedMPU401port;

    return result;
}

#pragma argsused
int MPU401initHardware(uint port, uchar irq, uchar dma)
{
    MPU401port = port;
    if (MPU401reset())
	return -1;
    return MPU401sendCommand(MPU401_SET_UART);	/* set UART mode */
}

int MPU401deinitHardware(void)
{
    return MPU401sendCommand(MPU401_RESET);
}

