/*
 *	Name:		Sound Blaster AWE32 Music driver
 *	Project:	MUS File Player Library
 *	Version:	1.11
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Aug-23-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-12-1995	V1.00	V.Arnost
 *		Module created
 *	Aug-14-1995	V1.10	V.Arnost
 *		Generic MIDI functions separated and put to module MLMIDI.C
 *	Aug-23-1995	V1.11	V.Arnost
 *		Added countChannels()
 */

#include <malloc.h>
#include <mem.h>
#include <io.h>
#include <dos.h>
#include "awe32/ctaweapi.h"
#define __NOBYTE__	/* disable conflicting BYTE,WORD,DWORD declarations */
#include "muslib.h"

#ifdef __WATCOMC__
  #ifdef __386__
    #pragma library ("pawe32.lib")
    #define farmalloc	malloc
    #define farfree	free
  #else
    #if defined(__SMALL__)
      #pragma library ("rawe32s.lib")
    #elif defined(__MEDIUM__)
      #pragma library ("rawe32m.lib")
    #elif defined(__COMPACT__)
      #pragma library ("rawe32c.lib")
    #elif defined(__LARGE__) || defined(__HUGE__)
      #pragma library ("rawe32l.lib")
    #endif
    #define farmalloc	_fmalloc
    #define farfree	_ffree
    /* force the compiler to load valid DS segment value before calling */
    /* the AWE32 API functions (in far data models, where DS is floating) */
    #pragma aux __pascal "^" parm loadds reverse routine [] \
			     value struct float struct caller [] \
			     modify [ax bx cx dx es];
  #endif /* __386__ */
#endif /* __WATCOMC__ */


/* Driver descriptor */
static char AWE32name[] = "AWE32 SYNTH";

struct driverBlock AWE32driver = {
	NULL,				// next
	DRV_AWE32,			// driverID
	AWE32name,			// name
	sizeof(struct MIDIdata),	// datasize
	MIDIinitDriver,
	MIDIdeinitDriver,
	AWE32driverParam,
	AWE32loadBank,
	AWE32detectHardware,
	AWE32initHardware,
	AWE32deinitHardware,

	MIDIplayNote,
	MIDIreleaseNote,
	MIDIpitchWheel,
	MIDIchangeControl,
	MIDIplayMusic,
	MIDIstopMusic,
	MIDIchangeVolume,
	MIDIpauseMusic,
	MIDIunpauseMusic,
	AWE32sendMIDI};

/* SoundFont variables -- should be zero upon startup */
static SOUND_PACKET spSound;
static char __FAR * pPresets[MAXBANKS];
static long lBankSizes[MAXBANKS];


/* compute active EMU8000 channels count */
/* Reads from undocumented internal structures, may not work in newer */
/* AWE32 Development Pack versions */
static void countChannels(void)
{
    uint i;
    GCHANNEL *channel = &awe32GChannel[0];

    playingChannels = 0;
    for (i = 0; i < awe32NumG; i++, channel++)
	if ( *(WORD *)channel != 0xFFFF )
	    playingChannels++;
}

/* send MIDI command */
int AWE32sendMIDI(uint command, uint par1, uint par2)
{
    uint result;
    uint MIDIchannel = command & 0x0F;
    switch ( (command & 0xF0) >> 4) {
	case 0x8:
	    result = awe32NoteOff(MIDIchannel, par1, par2);
	    break;
	case 0x9:
	    result = awe32NoteOn(MIDIchannel, par1, par2);
	    break;
	case 0xA:
	    return awe32PolyKeyPressure(MIDIchannel, par1, par2);
	case 0xB:
	    return awe32Controller(MIDIchannel, par1, par2);
	case 0xC:
	    return awe32ProgramChange(MIDIchannel, par1);
	case 0xD:
	    return awe32ChannelPressure(MIDIchannel, par1);
	case 0xE:
	    return awe32PitchBend(MIDIchannel, par1, par2);
	default:
	    return -1;
    }
    countChannels();
    return result;
}


#pragma argsused
int AWE32driverParam(uint message, uint param1, void *param2)
{
#if 0 /* disable non-functional AWE32 reverb/chorus control */
    static uchar sysex[] = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01,
	0x30 /* type */, 0x00 /* value */, 0x00 /* checksum */, 0xF7};
    uint i;

    switch (message) {
	case DP_AWE32_REVERB:
	    sysex[7] = 0x30;		/* type: reverb */
	    break;
	case DP_AWE32_CHORUS:
	    sysex[7] = 0x38;		/* type: chorus */
	    break;
	default:
	    return 0;			/* ignore anything else */
    }

    if (param1 >= 8)
	return -1;			/* invalid value */
    sysex[8] = param1;

    _disable();
//    for(i = 0; i < CHANNELS; i++)
    awe32Sysex(0, sysex, sizeof sysex);
    _enable();
#endif
    switch (message) {
	case DP_SYSEX:
	    _disable();
	    awe32Sysex(0, (BYTE _FAR_*)param2, param1);
	    _enable();
	    break;
    }
    return 0;
}


struct AWEbank {
	char	ID[8];			/* "#AWE_32#" */
	DWORD	size;			/* file size */
	DWORD	offset;			/* file data offset */
	WORD	padoffset[8];		/* pad offsets in the block */
//	BYTE	data[...]
};

#pragma argsused
int AWE32loadBank(int fd, uint bankNumber)
{
    static uchar masterhdr[8] = "#AWE_32#";
    uint i, bytesread;
    char *packet = (char *)malloc(PACKETSIZE);
    BYTE __FAR *presets;

    /* check bank number */
    if (bankNumber >= MAXBANKS)
	return -5;

    /* allocate ram */
    if (!packet)
	return -3;
    spSound.bank_no = bankNumber;               /* load as Bank # */
    if (bankNumber >= spSound.total_banks)
	spSound.total_banks = bankNumber + 1;	/* adjust bank count */
    lBankSizes[bankNumber] = spSound.total_patch_ram;/* use all available RAM */
    spSound.banksizes = lBankSizes;
    awe32DefineBankSizes(&spSound);

    /* read file header */
    spSound.data = packet;
    read(fd, packet, PACKETSIZE);

    /* check bank format */
    if (!memcmp(packet, masterhdr, sizeof masterhdr))
    {
	/* read bank in GENMIDI.AWE format */
	struct AWEbank *hdr = (struct AWEbank *)packet;

	lseek(fd, hdr->offset, SEEK_SET);
	if (!(presets = (BYTE __FAR *)farmalloc(hdr->size)))
	    return -3;
	pPresets[bankNumber] = (char __FAR *)presets;
	_dos_read(fd, presets, hdr->size, &bytesread);
	if (bytesread != hdr->size)
	{
	    farfree(presets);
	    return -1;
	}

#define PAD(num) &(presets[hdr->padoffset[(num) - 1]])
	awe32SoundPad.SPad1 = PAD(1);
	awe32SoundPad.SPad2 = PAD(2);
	awe32SoundPad.SPad3 = PAD(3);
	awe32SoundPad.SPad4 = PAD(4);
	awe32SoundPad.SPad5 = PAD(5);
	awe32SoundPad.SPad6 = PAD(6);
	awe32SoundPad.SPad7 = PAD(7);
#undef PAD

	lBankSizes[bankNumber] = 0;	/* no sample in SBK file */
    } else {
	/* read bank in .SBK file format */
	/* request to load */
	if (awe32SFontLoadRequest(&spSound))
	{
	    free(packet);
	    return -2;
	}

	/* stream samples */
	lseek(fd, spSound.sample_seek, SEEK_SET);
	for (i = 0; i < spSound.no_sample_packets; i++)
	{
	    read(fd, packet, PACKETSIZE);
	    awe32StreamSample(&spSound);
	}
	free(packet);

	/* setup SoundFont preset objects */
	if (!(presets = (BYTE __FAR *)farmalloc((unsigned) spSound.preset_read_size)))
	    return -3;
	lseek(fd, spSound.preset_seek, SEEK_SET);
	_dos_read(fd, presets, (unsigned) spSound.preset_read_size, &bytesread);
	if (bytesread != (unsigned) spSound.preset_read_size)
	{
	    farfree(presets);
	    return -1;
	}

	pPresets[bankNumber] = (char __FAR *)presets;
	spSound.presets = (char __FAR *)presets;
	if (awe32SetPresets(&spSound))
	    return -2;			/* invalid SoundFont file */

	/* calculate actual ram used */
	if (spSound.no_sample_packets)
	{
	    lBankSizes[bankNumber] = spSound.preset_seek - spSound.sample_seek + 160;
	    spSound.total_patch_ram -= lBankSizes[bankNumber];
	} else
	    lBankSizes[bankNumber] = 0;	/* no sample in SBK file */
    }

/*
    awe32SoundPad.SPad1 = awe32SPad1Obj;
    awe32SoundPad.SPad2 = awe32SPad2Obj;
    awe32SoundPad.SPad3 = awe32SPad3Obj;
    awe32SoundPad.SPad4 = awe32SPad4Obj;
    awe32SoundPad.SPad5 = awe32SPad5Obj;
    awe32SoundPad.SPad6 = awe32SPad6Obj;
    awe32SoundPad.SPad7 = awe32SPad7Obj;
*/

    /* initialize MIDI engine */
    if (awe32InitMIDI())
	return -4;

    return 0;
}

#pragma argsused
int AWE32detectHardware(uint port, uchar irq, uchar dma)
{
    return !awe32Detect(port);
}

#pragma argsused
int AWE32initHardware(uint port, uchar irq, uchar dma)
{
    if (awe32Detect(port))
	return -1;

    if (awe32InitHardware())
	return -1;

    awe32TotalPatchRam(&spSound);

    /* NOTE: loadBank must be called to properly initialize the MIDI engine */

    return 0;
}

int AWE32deinitHardware(void)
{
    uint i;

    /* free allocated memory */
    awe32ReleaseAllBanks(&spSound);

    for (i = 0; i < spSound.total_banks; i++)
	if (pPresets[i])
	    farfree(pPresets[i]);

    _disable();				/* interrupts should be off during */
    awe32InitMIDI();			/* the shutdown */
    i = awe32Terminate();
    _enable();

    return i;
}

