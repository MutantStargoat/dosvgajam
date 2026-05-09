/*
 *	Name:		Sound Blaster Pro/16 Mixer interface
 *	Project:	MUS File Player Library
 *	Version:	1.00
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Aug-08-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-08-1995	V1.00	V.Arnost
 *		Mixer-related functions moved to this module
 */

#include "muslib.h"

/*
 * Write to a mixer register -- SB Pro, SB16 only
 */
void SBsetMixer(uint SBport, uchar index, uchar data)
{
#ifdef __WATCOMC__

void _SBsetMixer(ushort port, uchar index, uchar data);
#pragma aux _SBsetMixer =	\
	"out	dx,al"		\
	"inc	dx"		\
	"mov	al,ah"		\
	"out	dx,al"		\
	parm [DX][AL][AH] nomemory	\
	modify exact [AL DX] nomemory;

/*    if (OPL3mode) */
    _SBsetMixer(SBport+4, index, data);
#else
/*    if (OPL3mode) */
    asm {
	mov	dx,SBport
	add	dx,4			// port 224h - Mixer register index
	mov	al,index
	out	dx,al
	inc	dx			// port 225h - Mixer data
	mov	al,data
	out	dx,al
    }
#endif
}

/*
 * Read from a mixer register -- SB Pro, SB16 only
 */
int SBgetMixer(uint SBport, uchar index)
{
#ifdef __WATCOMC__

uchar _SBgetMixer(ushort port, uchar index);
#pragma aux _SBgetMixer =		\
	"out	dx,al"		\
	"inc	dx"		\
	"in	al,dx"		\
	parm [DX][AL] nomemory	\
	modify exact [AL DX] nomemory \
	value [AL];

/*    if (OPL3mode) */
	return _SBgetMixer(SBport+4, index);
/*    else
	return -1; */
#else
/*    if (OPL3mode) */
    asm {
	mov	dx,SBport
	add	dx,4			// port 224h - Mixer register index
	mov	al,index
	out	dx,al
	inc	dx			// port 225h - Mixer data
	in	al,dx
	xor	ah,ah
    } /* else
	_AX = -1; */
    return _AX;
#endif
}

/*
 * Detect SB Pro mixer
 */
int SBdetectMixer(uint port)
{
    uint origvolume, volume1, volume2;

    origvolume = SBgetMixer(port, 0x26);	// FM Volume
    SBsetMixer(port, 0x26, origvolume ^ 0xAA);
    volume1 = SBgetMixer(port, 0x26) & 0xEE;
    SBsetMixer(port, 0x26, origvolume ^ 0x44);
    volume2 = SBgetMixer(port, 0x26) & 0xEE;
    SBsetMixer(port, 0x26, origvolume);

    return ((volume1 ^ 0xAA) == (volume2 ^ 0x44));
}
