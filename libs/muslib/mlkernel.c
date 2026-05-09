/*
 *	Name:		MUS Playing kernel
 *	Project:	MUS File Player Library
 *	Version:	1.70
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Oct-28-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-8-1994	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-9-1994	V1.10	V.Arnost
 *		Some minor changes to improve sound quality. Tried to add
 *		stereo sound capabilities, but failed to -- my SB Pro refuses
 *		to switch to stereo mode.
 *	Aug-13-1994	V1.20	V.Arnost
 *		Stereo sound fixed. Now works also with Sound Blaster Pro II
 *		(chip OPL3 -- gives 18 "stereo" (ahem) channels).
 *		Changed code to handle properly notes without volume.
 *		(Uses previous volume on given channel.)
 *		Added cyclic channel usage to avoid annoying clicking noise.
 *	Aug-17-1994	V1.30	V.Arnost
 *		Completely rewritten time synchronization. Now the player runs
 *		on IRQ 8 (RTC Clock - 1024 Hz).
 *	Aug-28-1994	V1.40	V.Arnost
 *		Added Adlib and SB Pro II detection.
 *		Fixed bug that caused high part of 32-bit registers (EAX,EBX...)
 *		to be corrupted.
 *	Oct-30-1994	V1.50	V.Arnost
 *		Tidied up the source code
 *		Added C key - invoke COMMAND.COM
 *		Added RTC timer interrupt flag check (0000:04A0)
 *		Added BLASTER environment variable parsing
 *		FIRST PUBLIC RELEASE
 *	Apr-16-1995	V1.60	V.Arnost
 *		Moved into separate source file MUSLIB.C
 *	May-01-1995	V1.61	V.Arnost
 *		Added system timer (IRQ 0) support
 *	Jul-12-1995	V1.62	V.Arnost
 *		OPL2/OPL3-specific code moved to module MLOPL.C
 *		Module MUSLIB.C renamed to MLKERNEL.C
 *	Aug-04-1995	V1.63	V.Arnost
 *		Fixed stack-related bug occuring in big-code models in Watcom C
 *	Aug-16-1995	V1.64	V.Arnost
 *		Stack size changed from 256 to 512 words because of stack
 *		underflows caused by AWE32 driver
 *	Aug-28-1995	V1.65	V.Arnost
 *		Fixed a serious bug that caused the player to generate an
 *		exception in AWE32 driver under DOS/4GW: Register ES contained
 *		garbage instead of DGROUP. The compiler-generated prolog of
 *		interrupt handlers doesn't set ES register at all, thus any
 *		STOS/MOVS/SCAS/CMPS instruction used within the int. handler
 *		crashes the program.
 *	Oct-28-1995	V1.70	V.Arnost
 *		System-specific timer code moved separate modules
 */

#include <dos.h>
#include "muslib.h"


char MLversion[] = "MUS Lib V"MLVERSIONSTR;
char MLcopyright[] = "Copyright (c) 1994-1996 QA-Software";

volatile ulong	MLtime;
volatile uint	playingChannels = 0;


/* Program */
static void playNote(struct musicBlock *mus, uint channel, uchar note, int volume)
{
    if (mus->channelMask & (1 << channel))
	mus->driver->playNote(mus, channel, note, volume);
}

static void releaseNote(struct musicBlock *mus, uint channel, uchar note)
{
    mus->driver->releaseNote(mus, channel, note);
}

static void pitchWheel(struct musicBlock *mus, uint channel, int pitch)
{
    mus->driver->pitchWheel(mus, channel, pitch);
}

static void changeControl(struct musicBlock *mus, uint channel, uchar controller, int value)
{
    mus->driver->changeControl(mus, channel, controller, value);
}

static int playTick(struct musicBlock *mus)
{
    for(;;)
    {
	uint data = MEMgetchar(&mus->score);
	uint command = (data >> 4) & 7;
	uint channel = data & 0x0F;
	uint last = data & 0x80;

	switch (command) {
	    case 0:	// release note
		mus->playingcount--;
		releaseNote(mus, channel, MEMgetchar(&mus->score));
		break;
	    case 1: {	// play note
		uchar note = MEMgetchar(&mus->score);
		mus->playingcount++;
		if (note & 0x80)	// note with volume
		    playNote(mus, channel, note & 0x7F, MEMgetchar(&mus->score));
		else
		    playNote(mus, channel, note, -1);
		} break;
	    case 2:	// pitch wheel
		pitchWheel(mus, channel, MEMgetchar(&mus->score) - 0x80);
		break;
	    case 3:	// system event (valueless controller)
		changeControl(mus, channel, MEMgetchar(&mus->score), 0);
		break;
	    case 4: {	// change control
		uchar ctrl = MEMgetchar(&mus->score);
		uchar value = MEMgetchar(&mus->score);
		changeControl(mus, channel, ctrl, value);
		} break;
	    case 6:	// end
		return -1;
	    case 5:	// ???
	    case 7:	// ???
		break;
	}
	if (last)
	    break;
    }
    return 0;
}

static ulong delayTicks(struct musicBlock *mus)
{
    ulong time = 0;
    uchar data;

    do {
	time <<= 7;
	time += (data = MEMgetchar(&mus->score)) & 0x7F;
    } while (data & 0x80);

    return time;
}


/*
 * Perform one timer tick using 140 Hz clock
 */
#ifdef __WINDOWS__
  #define PLAYFAST 0
#elif __386__
  #define PLAYFAST (*((BYTE *)0x00000418) & 4) /* Alt-SysRq pressed */
#else
  #define PLAYFAST (*(BYTE far *)MK_FP(0x0000, 0x0418) & 4) /* Alt-SysRq pressed */
#endif /* __386__ */

void MLplayerInterrupt(void)
{
    uint i;
    for (i = 0; i < MAXMUSBLOCK; i++)
    {
	register struct musicBlock *mus = MLmusicBlocks[i];
	if (mus && mus->state == ST_PLAYING)
	{
	    if (!mus->ticks || PLAYFAST)
	    {
		if (playTick(mus))
		{					// end of song
		    if (mus->loopcount &&
		       (mus->loopcount == -1U || --mus->loopcount)) // -1: loop forever
			MEMrewind(&mus->score);
		    else
			mus->state = ST_STOPPED;
		    continue;
		}
		mus->time += mus->ticks = delayTicks(mus);
	    }
	    mus->ticks--;
	}
    }
    MLtime++;
}

