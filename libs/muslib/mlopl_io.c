/*
 *	Name:		Low-level OPL2/OPL3 I/O interface
 *	Project:	MUS File Player Library
 *	Version:	1.64
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Mar-1-1996
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-8-1994	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-9-1994	V1.10	V.Arnost
 *		Added stereo capabilities
 *	Aug-13-1994	V1.20	V.Arnost
 *		Stereo capabilities made functional
 *	Aug-24-1994	V1.30	V.Arnost
 *		Added Adlib and SB Pro II detection
 *	Oct-30-1994	V1.40	V.Arnost
 *		Added BLASTER variable parsing
 *	Apr-14-1995	V1.50	V.Arnost
 *              Some declarations moved from adlib.h to deftypes.h
 *	Jul-22-1995	V1.60	V.Arnost
 *		Ported to Watcom C
 *		Simplified WriteChannel() and WriteValue()
 *	Jul-24-1995	V1.61	V.Arnost
 *		DetectBlaster() moved to MLMISC.C
 *	Aug-8-1995	V1.62	V.Arnost
 *		Module renamed to MLOPL_IO.C and functions renamed to OPLxxx
 *		Mixer-related functions moved to module MLSBMIX.C
 *	Sep-8-1995	V1.63	V.Arnost
 *		OPLwriteReg() routine sped up on OPL3 cards
 *	Mar-1-1996	V1.64	V.Arnost
 *		Cleaned up the source
 */

#include <dos.h>
#include <conio.h>
#include "muslib.h"

uint OPLport = ADLIBPORT;
uint OPLchannels = OPL2CHANNELS;
uint OPL3mode = 0;

/*
 * Direct write to any OPL2/OPL3 FM synthesizer register.
 *   reg - register number (range 0x001-0x0F5 and 0x101-0x1F5). When high byte
 *         of reg is zero, data go to port OPLport, otherwise to OPLport+2
 *   data - register value to be written
 */
#ifdef __WATCOMC__

/* Watcom C */
uchar _OPL2writeReg(uint port, uint reg, uchar data);
uchar _OPL3writeReg(uint port, uint reg, uchar data);

#ifdef __386__
#pragma aux _OPL2writeReg =	\
	"out	dx,al"		\
	"mov	ecx,6"		\
"loop1:	 in	al,dx"		\
	"loop	loop1"		\
	"inc	edx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	"dec	edx"		\
	"mov	ecx,36"		\
"loop2:	 in	al,dx"		\
	"loop	loop2"		\
	parm [EDX][EAX][BL]	\
	modify exact [AL ECX EDX] nomemory	\
	value [AL];

#pragma aux _OPL3writeReg =	\
	"or	ah,ah"		\
	"jz	bank0"		\
	"inc	edx"		\
	"inc	edx"		\
"bank0:	 out	dx,al"		\
	"in	al,dx"		\
	"mov	ah,al"		\
	"inc	edx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	parm [EDX][EAX][BL]	\
	modify exact [AX EDX] nomemory	\
	value [AH];
#else /* !__386__ */
#pragma aux _OPL2writeReg =	\
	"out	dx,al"		\
	"mov	cx,6"		\
"loop1:	 in	al,dx"		\
	"loop	loop1"		\
	"inc	dx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	"dec	dx"		\
	"mov	cx,36"		\
"loop2:	 in	al,dx"		\
	"loop	loop2"		\
	parm [DX][AX][BL]	\
	modify exact [AL CX DX] nomemory	\
	value [AL];

#pragma aux _OPL3writeReg =	\
	"or	ah,ah"		\
	"jz	bank0"		\
	"inc	dx"		\
	"inc	dx"		\
"bank0:	 out	dx,al"		\
	"in	al,dx"		\
	"mov	ah,al"		\
	"inc	dx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	parm [DX][AX][BL]	\
	modify exact [AX DX] nomemory	\
	value [AH];
#endif

uint OPLwriteReg(uint reg, uchar data)
{
    if (OPL3mode)
	return _OPL3writeReg(OPLport, reg, data);
    else
	return _OPL2writeReg(OPLport, reg, data);
}

#else

/* Borland C */
uint OPLwriteReg(uint reg, uchar data)
{
#define I asm
    if (OPL3mode)		/* OPL3 mode: no delay loops */
    {
I	mov	dx,OPLport
I	mov	ax,reg
I	or	ah,ah
I	jz	bank0
I	inc	dx
I	inc	dx
bank0:
I	out	dx,al
I	in	al,dx		/* short delay */
I	mov	ah,al
I	inc	dx
I	mov	al,data
I	out	dx,al
I	mov	al,ah
    } else {			/* OPL2 mode: with delays, first bank only */
I	mov	dx,OPLport
I	mov	ax,reg
I	out	dx,al
I	mov	cx,6
loop1:
I	in	al,dx
I	loop	loop1

I	inc	dx
I	mov	al,data
I	out	dx,al
I	dec	dx
I	mov	cx,36
loop2:
I	in	al,dx
I	loop	loop2
    }
    return _AL;
}
#endif

/*
 * Write to an operator pair. To be used for register bases of 0x20, 0x40,
 * 0x60, 0x80 and 0xE0.
 */
void OPLwriteChannel(uint regbase, uint channel, uchar data1, uchar data2)
{
    static uint op_num[] = {
	0x000, 0x001, 0x002, 0x008, 0x009, 0x00A, 0x010, 0x011, 0x012,
	0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112};

    register uint reg = regbase+op_num[channel];
    OPLwriteReg(reg, data1);
    OPLwriteReg(reg+3, data2);
}

/*
 * Write to channel a single value. To be used for register bases of
 * 0xA0, 0xB0 and 0xC0.
 */
void OPLwriteValue(uint regbase, uint channel, uchar value)
{
    static uint reg_num[] = {
	0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008,
	0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108};

    OPLwriteReg(regbase + reg_num[channel], value);
}

/*
 * Write frequency/octave/keyon data to a channel
 */
void OPLwriteFreq(uint channel, uint freq, uint octave, uint keyon)
{
    OPLwriteValue(0xA0, channel, (BYTE)freq);
    OPLwriteValue(0xB0, channel, (freq >> 8) | (octave << 2) | (keyon << 5));
}

/*
 * Adjust volume value (register 0x40)
 */
uint OPLconvertVolume(uint data, uint volume)
{
    static uchar volumetable[128] = {
	  0,   1,   3,   5,   6,   8,  10,  11,
	 13,  14,  16,  17,  19,  20,  22,  23,
	 25,  26,  27,  29,  30,  32,  33,  34,
	 36,  37,  39,  41,  43,  45,  47,  49,
	 50,  52,  54,  55,  57,  59,  60,  61,
	 63,  64,  66,  67,  68,  69,  71,  72,
	 73,  74,  75,  76,  77,  79,  80,  81,
	 82,  83,  84,  84,  85,  86,  87,  88,
	 89,  90,  91,  92,  92,  93,  94,  95,
	 96,  96,  97,  98,  99,  99, 100, 101,
	101, 102, 103, 103, 104, 105, 105, 106,
	107, 107, 108, 109, 109, 110, 110, 111,
	112, 112, 113, 113, 114, 114, 115, 115,
	116, 117, 117, 118, 118, 119, 119, 120,
	120, 121, 121, 122, 122, 123, 123, 123,
	124, 124, 125, 125, 126, 126, 127, 127};

#if 0
    uint n;

    if (volume > 127)
	volume = 127;
    n = 0x3F - (data & 0x3F);
    n = (n * (uint)volumetable[volume]) >> 7;
    return (0x3F - n) | (data & 0xC0);
#else
    return 0x3F - (((0x3F - data) *
	(uint)volumetable[volume <= 127 ? volume : 127]) >> 7);
#endif
}

uint OPLpanVolume(uint volume, int pan)
{
    if (pan >= 0)
	return volume;
    else
	return (volume * (pan + 64)) / 64;
}

/*
 * Write volume data to a channel
 */
void OPLwriteVolume(uint channel, struct OPL2instrument *instr, uint volume)
{
    OPLwriteChannel(0x40, channel, ((instr->feedback & 1) ?
	OPLconvertVolume(instr->level_1, volume) : instr->level_1) | instr->scale_1,
	OPLconvertVolume(instr->level_2, volume) | instr->scale_2);
}

/*
 * Write pan (balance) data to a channel
 */
void OPLwritePan(uint channel, struct OPL2instrument *instr, int pan)
{
    uchar bits;
    if (pan < -36) bits = 0x10;		// left
    else if (pan > 36) bits = 0x20;	// right
    else bits = 0x30;			// both

    OPLwriteValue(0xC0, channel, instr->feedback | bits);
}

/*
 * Write an instrument to a channel
 *
 * Instrument layout:
 *
 *   Operator1  Operator2  Descr.
 *    data[0]    data[7]   reg. 0x20 - tremolo/vibrato/sustain/KSR/multi
 *    data[1]    data[8]   reg. 0x60 - attack rate/decay rate
 *    data[2]    data[9]   reg. 0x80 - sustain level/release rate
 *    data[3]    data[10]  reg. 0xE0 - waveform select
 *    data[4]    data[11]  reg. 0x40 - key scale level
 *    data[5]    data[12]  reg. 0x40 - output level (bottom 6 bits only)
 *          data[6]        reg. 0xC0 - feedback/AM-FM (both operators)
 */
void OPLwriteInstrument(uint channel, struct OPL2instrument *instr)
{
    OPLwriteChannel(0x40, channel, 0x3F, 0x3F);		// no volume
    OPLwriteChannel(0x20, channel, instr->trem_vibr_1, instr->trem_vibr_2);
    OPLwriteChannel(0x60, channel, instr->att_dec_1,   instr->att_dec_2);
    OPLwriteChannel(0x80, channel, instr->sust_rel_1,  instr->sust_rel_2);
    OPLwriteChannel(0xE0, channel, instr->wave_1,      instr->wave_2);
    OPLwriteValue  (0xC0, channel, instr->feedback | 0x30);
}

/*
 * Stop all sounds
 */
void OPLshutup(void)
{
    uint i;

    for(i = 0; i < OPLchannels; i++)
    {
	OPLwriteChannel(0x40, i, 0x3F, 0x3F);	// turn off volume
	OPLwriteChannel(0x60, i, 0xFF, 0xFF);	// the fastest attack, decay
	OPLwriteChannel(0x80, i, 0x0F, 0x0F);	// ... and release
	OPLwriteValue(0xB0, i, 0);		// KEY-OFF
    }
}

/*
 * Initialize hardware upon startup
 */
void OPLinit(uint port, uint OPL3)
{
    OPLport = port;
    if ( (OPL3mode = OPL3) != 0)
    {
	OPLchannels = OPL3CHANNELS;
	OPLwriteReg(0x105, 0x01);	// enable YMF262/OPL3 mode
	OPLwriteReg(0x104, 0x00);	// disable 4-operator mode
    } else
	OPLchannels = OPL2CHANNELS;
    OPLwriteReg(0x01, 0x20);		// enable Waveform Select
    OPLwriteReg(0x08, 0x40);		// turn off CSW mode
    OPLwriteReg(0xBD, 0x00);		// set vibrato/tremolo depth to low, set melodic mode

    OPLshutup();
}

/*
 * Deinitialize hardware before shutdown
 */
void OPLdeinit(void)
{
    OPLshutup();
    if (OPL3mode)
    {
	OPLwriteReg(0x105, 0x00);		// disable YMF262/OPL3 mode
	OPLwriteReg(0x104, 0x00);		// disable 4-operator mode
    }
    OPLwriteReg(0x01, 0x20);			// enable Waveform Select
    OPLwriteReg(0x08, 0x00);			// turn off CSW mode
    OPLwriteReg(0xBD, 0x00);			// set vibrato/tremolo depth to low, set melodic mode
}

/*
 * Detect Adlib card (OPL2)
 */
int OPL2detect(uint port)
{
    uint origPort = OPLport;
    uint stat1, stat2, i;

    OPLport = port;
    OPLwriteReg(0x04, 0x60);
    OPLwriteReg(0x04, 0x80);
    stat1 = inp(port) & 0xE0;
    OPLwriteReg(0x02, 0xFF);
    OPLwriteReg(0x04, 0x21);
    for (i = 512; --i; )
	inp(port);
    stat2 = inp(port) & 0xE0;
    OPLwriteReg(0x04, 0x60);
    OPLwriteReg(0x04, 0x80);
    OPLport = origPort;

    return (stat1 == 0 && stat2 == 0xC0);
}

/*
 * Detect Sound Blaster Pro II (OPL3)
 *
 * Status register contents (inp(port) & 0x06):
 *   OPL2:	6
 *   OPL3:	0
 *   OPL4:	2
 */
int OPL3detect(uint port)
{
    if (!OPL2detect(port))
	return 0;
#if 0
    if (!SBdetectMixer(port))
	return 0;
    if (OPL2detect(port + 2))
	return 0;
#else
    if (inp(port) & 4)
	return 0;
#endif
    return 1;
}
