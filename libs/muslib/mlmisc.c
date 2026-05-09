/*
 *	Name:		Miscellaneous functions
 *	Project:	MUS File Player Library
 *	Version:	1.10
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Aug-24-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Jul-24-1995	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-24-1995	V1.10	V.Arnost
 *		Added MLparseBlaster()
 */

#include <stdlib.h>
#include <stdarg.h>
#include "muslib.h"

/*
 * Parse BLASTER environment variable
 */
/*
int MLdetectBlaster(uint *port, ushort *irq, ushort *dma, ushort *type)
{
    const uchar *blaster = getenv("BLASTER");
    if (!blaster) return 0;

    while (*blaster)
    {
	char *endpos = NULL;
	long value;

	switch (*blaster++) {
	    case 'A':	// base I/O address
		value = strtol(blaster, &endpos, 16);
		if (port) *port = value;
		break;
	    case 'I':	// IRQ number
		value = strtol(blaster, &endpos, 0);
		if (irq) *irq = value;
		break;
	    case 'D':	// DMA number
		value = strtol(blaster, &endpos, 0);
		if (dma) *dma = value;
		break;
	    case 'T':	// card type
		value = strtol(blaster, &endpos, 0);
		if (type) *type = value;
		break;
	    // ignore anything else (spaces, ...)
	}
	if (endpos)
	    blaster = endpos;
    }
    return 1;
}
*/

uint MLparseBlaster(const char *format, ...)
{
    char *blaster;
    uint mask = 0;

    if (!(blaster = getenv("BLASTER")))
	return 0;

    while (*blaster)
    {
	char type = *blaster++;

	if (type >= 'a' && type <= 'z')	/* convert to uppercase */
	    type -= 'a' - 'A';

	if (type >= 'A' && type <= 'Z')	/* only letters are accepted */
	{
	    const char *form;
	    uint radix, index;
	    va_list ap;
	    uint *value;

	    if (*blaster == ':')	/* ignore colon (if any) */
		blaster++;

	    va_start(ap, format);
	    value = va_arg(ap, uint *);
	    radix = 16;
	    index = 1;

	    for (form = format; *form; form++)
	    {
		if (*form == type)
		{
		    char *endpos;
		    *value = strtol(blaster, &endpos, radix);
		    blaster = endpos;
		    mask |= index;
		    break;
		} else
		    switch (*form) {
			case '#':
			    radix = 10;
			    break;
			case '$':
			    radix = 16;
			    break;
			case ':':
			    index <<= 1;
			    value = va_arg(ap, uint *);
			    break;
		    }
	    }
	    va_end(ap);
	}
    }
    return mask;
}
