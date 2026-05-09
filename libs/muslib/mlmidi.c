/*
 *	Name:		Generic MIDI music driver
 *	Project:	MUS File Player Library
 *	Version:	1.02
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Mar-9-1996
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-14-1995	V1.00	V.Arnost
 *		Module created
 *	Sep-8-1995	V1.01	V.Arnost
 *		Minor changes
 *	Mar-9-1996	V1.02	V.Arnost
 *		Added controller cache initialization upon receiving
 *		"Reset All Controllers" command
 */

#include <mem.h>
#include "muslib.h"

/* MUS -> MIDI controller number conversion */
static const uchar MUS2MIDIctrl[] = {	/* MUS controller: */
	0xFF,				/*  0 - instrument--event 0xC0 */
	0,				/*  1 - bank select */
	1,				/*  2 - modulation pot */
	7,				/*  3 - volume */
	10,				/*  4 - pan (balance) pot */
	11,				/*  5 - expression pot */
	91,				/*  6 - reverb depth */
	93,				/*  7 - chorus depth */
	64,				/*  8 - sustain pedal (hold) */
	67,				/*  9 - soft pedal */
	120,				/* 10 - all sounds off */
	123,				/* 11 - all notes off */
	126,				/* 12 - mono */
	127,				/* 13 - poly */
	121};				/* 14 - reset all controllers */

/* MIDI channel occupation map */
static uint MIDIchannels[CHANNELS];

/* last MIDI channel access time */
static long MIDItime[CHANNELS];

/* send a MIDI event to driver */
#define SENDMIDI(mus, chn, cmd, par1, par2)	\
	((mus)->driver->sendMIDI((chn) | (cmd), (par1), (par2)))

/* touch MIDI channel timestamp */
#define TOUCH(channel)	MIDItime[channel] = MLtime

#define MIDI_PERC	9	/* standard MIDI percussion channel */

/* calculate MIDI channel volume */
static uint calcVolume(uint MUSvolume, uint noteVolume)
{
    noteVolume = ((ulong)MUSvolume * noteVolume) >> 8;
    if (noteVolume > 127)
	return 127;
    else
	return noteVolume;
}

#if 0
#include <dos.h>
#define showChannels() 				\
{						\
    WORD __FAR *scr = MK_FP(0xB800, 80);	\
    uint i;					\
    for (i = 0; i < CHANNELS; i++)		\
	*scr++ = MIDIchannels[i] | 0x2F30;	\
}
#else
#define showChannels()
#endif

static void stopChannel(struct musicBlock *mus, uint MIDIchannel)
{
    SENDMIDI(mus, MIDIchannel, MIDI_CONTROL, 120, 127); /* all sounds off */
    SENDMIDI(mus, MIDIchannel, MIDI_CONTROL, 121, 127); /* reset all controllers */
}

/* find free MIDI output channel */
static int findFreeChannel(struct musicBlock *mus, uint channel)
{
    uint i, oldest;
    long time;

    if (mus->percussMask & (1 << channel))
	return MIDI_PERC;

    /* find unused MIDI i */
    for (i = 0; i < CHANNELS; i++)
	if (MIDIchannels[i] == -1U)
	{
	    MIDIchannels[i] = mus->number | (channel << 8);
	    showChannels();
	    return i;
	}

    /* find the longest time untouched i */
    time = MLtime;
    oldest = -1U;

    for (i = 0; i < CHANNELS; i++)
	if (i != MIDI_PERC && MIDItime[i] < time)
	{
	    time = MIDItime[i];
	    oldest = i;
	}

    if ( (i = oldest) != -1U)
    {
	struct musicBlock *oldmus = MLmusicBlocks[(BYTE)MIDIchannels[i]];
	struct MIDIdata *olddata = (struct MIDIdata *)oldmus->driverdata;
	olddata->realChannels[MIDIchannels[i] >> 8] = -1;
	stopChannel(oldmus, i);
	MIDIchannels[i] = mus->number | (channel << 8);
	showChannels();
/*	{
	    WORD __FAR *scr = MK_FP(0xB800, 80);
	    scr[i] |= 0x4000;
	    scr[i+80] = (scr[i+80]+1) | 0x5830;
	} */
    }
    return i;
}

/* release all channels belonging to *mus */
static int releaseChannels(struct musicBlock *mus)
{
    uint channel;

    for (channel = 0; channel < CHANNELS; channel++)
	if ((BYTE)MIDIchannels[channel] == mus->number)
	    MIDIchannels[channel] = -1U;
    showChannels();
    return 0;
}

/* send all tracked controller values to the MIDI output */
static void updateControllers(struct musicBlock *mus, uint channel)
{
    uint i, value;
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    int MIDIchannel;

    if ( (MIDIchannel = data->realChannels[channel]) >= 0)
    {
	SENDMIDI(mus, MIDIchannel, MIDI_PATCH, data->controllers[ctrlPatch][channel], 0);
	for (i = ctrlPatch + 1; i < _ctrlCount_; i++)
	{
	    value = data->controllers[i][channel];
	    if (i == ctrlVolume)
		if (MIDIchannel == MIDI_PERC)
		    continue;
		else
		    value = calcVolume(mus->volume, value);
	    SENDMIDI(mus, MIDIchannel, MIDI_CONTROL, MUS2MIDIctrl[i], value);
	}
	value = data->pitchWheel[channel] + 0x80;
	SENDMIDI(mus, MIDIchannel, MIDI_PITCH_WHEEL, (value & 1) << 6, (value >> 1) & 0x7F);
    }
}

/* send updated volume */
static void sendVolume(struct musicBlock *mus, uint volume)
{
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    uint i;

    for(i = 0; i < CHANNELS; i++)
    {
	int MIDIchannel;
	if ( (MIDIchannel = data->realChannels[i]) >= 0)
	    if (MIDIchannel != MIDI_PERC)
		SENDMIDI(mus, MIDIchannel, MIDI_CONTROL, MUS2MIDIctrl[ctrlVolume],
		     calcVolume(volume, data->controllers[ctrlVolume][i]));
    }
}


// code 1: play note
void MIDIplayNote(struct musicBlock *mus, uint channel, uchar note, int volume)
{
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    int MIDIchannel;

    if (volume == -1)
	volume = data->channelLastVolume[channel];
    else
	data->channelLastVolume[channel] = volume;

    if ( (MIDIchannel = data->realChannels[channel]) < 0)
    {
	if ( (MIDIchannel = findFreeChannel(mus, channel)) < 0)
	    return;
	data->realChannels[channel] = MIDIchannel;
	updateControllers(mus, channel);
    }

    if (MIDIchannel == MIDI_PERC)
    {
	data->percussions[note >> 3] |= 1 << (note & 7);
	volume = (calcVolume(mus->volume, data->controllers[ctrlVolume]
					  [channel]) * volume) / 127;
    }

    TOUCH(MIDIchannel);
    SENDMIDI(mus, MIDIchannel, MIDI_NOTE_ON, note, volume);
}

// code 0: release note
void MIDIreleaseNote(struct musicBlock *mus, uint channel, uchar note)
{
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    int MIDIchannel;

    if ( (MIDIchannel = data->realChannels[channel]) >= 0)
    {
	if (MIDIchannel == MIDI_PERC)
	    data->percussions[note >> 3] &= ~(1 << (note & 7));

	TOUCH(MIDIchannel);
	SENDMIDI(mus, MIDIchannel, MIDI_NOTE_OFF, note, 127);
    }
}

// code 2: change pitch wheel (bender)
void MIDIpitchWheel(struct musicBlock *mus, uint channel, int pitch)
{
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    int MIDIchannel;

    data->pitchWheel[channel] = pitch;

    if ( (MIDIchannel = data->realChannels[channel]) >= 0)
    {
	TOUCH(MIDIchannel);
	pitch += 0x80;
	SENDMIDI(mus, MIDIchannel, MIDI_PITCH_WHEEL, (pitch & 1) << 6, (pitch >> 1) & 0x7F);
    }
}

// code 4: change control
void MIDIchangeControl(struct musicBlock *mus, uint channel, uchar controller, int value)
{
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    int MIDIchannel;

    if (controller < _ctrlCount_)
	data->controllers[controller][channel] = value;

    if ( (MIDIchannel = data->realChannels[channel]) < 0)
	return;

    TOUCH(MIDIchannel);

    if (!controller)			/* 0 - instrument */
	SENDMIDI(mus, MIDIchannel, MIDI_PATCH, value, 0);
    else if (controller <= 14)
    {
	switch (controller) {
	    case ctrlVolume:		/* change volume */
		if (MIDIchannel == MIDI_PERC)
		    return;
		value = calcVolume(mus->volume, value);
		break;
	    case ctrlResetCtrls:	/* Reset All Controllers */
		/* Perhaps, some controllers should be added or removed,
		   I don't know the exact implementation of this command */
		data->controllers[ctrlBank	  ][channel] = 0;
		data->controllers[ctrlModulation  ][channel] = 0;
		data->controllers[ctrlPan	  ][channel] = 64;
		data->controllers[ctrlExpression  ][channel] = 127;
		data->controllers[ctrlSustainPedal][channel] = 0;
		data->controllers[ctrlSoftPedal	  ][channel] = 0;
		data->pitchWheel		   [channel] = 0;
		break;
	}
	SENDMIDI(mus, MIDIchannel, MIDI_CONTROL, MUS2MIDIctrl[controller], value);
    }
}

void MIDIplayMusic(struct musicBlock *mus)
{
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;
    uint i;

    for (i = 0; i < CHANNELS; i++)
    {
	data->controllers[ctrlPatch	  ][i] = 0;
	data->controllers[ctrlBank	  ][i] = 0;
	data->controllers[ctrlModulation  ][i] = 0;
	data->controllers[ctrlVolume	  ][i] = 127;
	data->controllers[ctrlPan	  ][i] = 64;
	data->controllers[ctrlExpression  ][i] = 127;
	data->controllers[ctrlReverb	  ][i] = 0;
	data->controllers[ctrlChorus	  ][i] = 0;
	data->controllers[ctrlSustainPedal][i] = 0;
	data->controllers[ctrlSoftPedal	  ][i] = 0;
	data->channelLastVolume            [i] = 0; /* last volume--anything */
	data->pitchWheel		   [i] = 0;
	data->realChannels		   [i] = -1; /* no assignment */
    }
    memset(data->percussions, 0, sizeof data->percussions);
    SENDMIDI(mus, MIDI_PERC, MIDI_CONTROL, MUS2MIDIctrl[ctrlVolume], 127);
    SENDMIDI(mus, MIDI_PERC, MIDI_CONTROL, MUS2MIDIctrl[ctrlResetCtrls], 0);
}

void MIDIstopMusic(struct musicBlock *mus)
{
    uint i;
    struct MIDIdata *data = (struct MIDIdata *)mus->driverdata;

    for (i = 0; i < CHANNELS; i++)
    {
	int MIDIchannel;
	if ( (MIDIchannel = data->realChannels[i]) < 0)
	    continue;
	if (MIDIchannel != MIDI_PERC)
	    stopChannel(mus, MIDIchannel);
	else {
	    uint j;
	    for(j = 0; j < 128; j++)
		if (data->percussions[j >> 3] & (1 << (j & 7)))
		    SENDMIDI(mus, MIDI_PERC, MIDI_NOTE_OFF, j, 127);
	}
    }
    releaseChannels(mus);
}

void MIDIchangeVolume(struct musicBlock *mus, uint volume)
{
    if (mus->state == ST_PLAYING)
	sendVolume(mus, volume);
}

void MIDIpauseMusic(struct musicBlock *mus)
{
    sendVolume(mus, 0);
}

void MIDIunpauseMusic(struct musicBlock *mus)
{
    sendVolume(mus, mus->volume);
}


int MIDIinitDriver(void)
{
    memset(MIDIchannels, 0xFF, sizeof MIDIchannels);
    MIDIchannels[MIDI_PERC] = ~(-1U >> 1); /* mark perc. channel as always occupied */
    memset(MIDItime, 0, sizeof MIDItime);
    return 0;
}

int MIDIdeinitDriver(void)
{
    return 0;
}
