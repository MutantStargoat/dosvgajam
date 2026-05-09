/*
 *	Name:		Sound Blaster MIDI port music driver
 *	Project:	MUS File Player Library
 *	Version:	1.10
 *	Author:		Vladimir Arnost (QA-Software)
 *			SB MIDI port code from SoundBlaster Freedom Project
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
 *		Probably solved the hanging problem by disabling interrupts
 *		during I/O port activity in SBMIDIsendByte()
 */

#include <dos.h>
#include <conio.h>
#include "muslib.h"


/* Driver descriptor */
static char SBMIDIname[] = "SB MIDI OUT";

struct driverBlock SBMIDIdriver = {
	NULL,				// next
	DRV_SBMIDI,			// driverID
	SBMIDIname,			// name
	sizeof(struct MIDIdata),	// datasize
	MIDIinitDriver,
	MIDIdeinitDriver,
	SBMIDIdriverParam,
	SBMIDIloadBank,
	SBMIDIdetectHardware,
	SBMIDIinitHardware,
	SBMIDIdeinitHardware,

	MIDIplayNote,
	MIDIreleaseNote,
	MIDIpitchWheel,
	MIDIchangeControl,
	MIDIplayMusic,
	MIDIstopMusic,
	MIDIchangeVolume,
	MIDIpauseMusic,
	MIDIunpauseMusic,
	SBMIDIsendMIDI};


static uint SBMIDIport = SBMIDIPORT;
static uint runningStatus = 0;

/* I/O addresses (default base = 220h) */
#define DSP_READ_DATA       0x0A
#define DSP_WRITE_DATA      0x0C
#define DSP_WRITE_STATUS    0x0C
#define DSP_DATA_AVAIL      0x0E

/* DSP commands */
#define MIDI_READ_POLL	    0x30
#define MIDI_READ_IRQ	    0x31
#define MIDI_WRITE_POLL     0x38

/* write data to the DAC */
#define writedac(x)						\
{								\
    uint t = -1U;						\
    while (inp(SBMIDIport + DSP_WRITE_STATUS) & (uchar)0x80 &&	\
	   --t);						\
    if (!t)							\
	return -1;						\
    outp(SBMIDIport + DSP_WRITE_DATA, (x));			\
}

/* write one byte to SB MIDI data port */
static FASTCALL int SBMIDIsendByte(uchar value)
{
    writedac(MIDI_WRITE_POLL);
    writedac(value);
    return 0;
}

/* write block of bytes to MPU-401 port */
static int SBMIDIsendBlock(uchar *block, uint length)
{
    runningStatus = 0;			/* clear the running status byte */

    _disable();
    while (length--)
	SBMIDIsendByte(*block++);
    _enable();
    return 0;
}


/* send MIDI command */
int SBMIDIsendMIDI(uint command, uint par1, uint par2)
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
	SBMIDIsendByte(runningStatus = command);
    SBMIDIsendByte(par1);
    if (event != MIDI_PATCH && event != MIDI_CHAN_TOUCH)
	SBMIDIsendByte(par2);
    _enable();
    return 0;
}

int SBMIDIdriverParam(uint message, uint param1, void *param2)
{
    switch (message) {
	case DP_SYSEX:
	    SBMIDIsendBlock((uchar *)param2, param1);
	    break;
    }
    return 0;
}

#pragma argsused
int SBMIDIloadBank(int fd, uint bankNumber)
{
    return 0;
}

#pragma argsused
int SBMIDIdetectHardware(uint port, uchar irq, uchar dma)
{
    runningStatus = 0;
    return 1;				/* always present */
}

#pragma argsused
int SBMIDIinitHardware(uint port, uchar irq, uchar dma)
{
    SBMIDIport = port;
    runningStatus = 0;
    return 0;
}

int SBMIDIdeinitHardware(void)
{
    runningStatus = 0;
    return 0;
}

