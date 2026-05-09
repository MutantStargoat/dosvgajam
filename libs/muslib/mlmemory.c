/*
 *	Name:		Memory access routines
 *	Project:	MUS File Player Library
 *	Version:	1.20
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Sep-4-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Jul-4-1995	V1.00	V.Arnost
 *		Module created and written from scratch
 *	Jul-29-1995	V1.10	V.Arnost
 *		Ported to Watcom C
 *	Sep-4-1995	V1.20	V.Arnost
 *		Ported to Windows
 */

#include <dos.h>
#include <mem.h>
#include <stdlib.h>
#include <malloc.h>
#include "muslib.h"

#ifdef __WATCOMC__
  #define farmalloc	_fmalloc
  #define farfree	_ffree
#endif /* __WATCOMC__ */

#if !defined(__386__) && !defined(__WINDOWS__)
  #define __DOSMEMORY__
#endif

#if defined (__BORLANDC__)

#ifdef __cplusplus

  int min (int value1, int value2);

  int min(int value1, int value2)
  {
     return ( (value1 < value2) ? value1 : value2);
  }

#endif

#endif


/*
 * EMS memory management routines
 */

#ifdef __DOSMEMORY__

static BYTE EMSpresent = 0;
static BYTE EMSversion = 0;
static WORD EMSframe   = 0;

#define EMS_VECT	0x67

#define EMS_PAGESIZE	16384U
#define EMS_PAGESHIFT	14
#define EMS_FRAME(page) ((BYTE far *)MK_FP(EMSframe, ((uint)(page)) << EMS_PAGESHIFT)) /* page: 0..3 */

static uint detectEMS(void)
{
    char far *ptr = (char far *)_dos_getvect(EMS_VECT);
    if (ptr)
    {
	static char EMSsign[8] = "EMMXXXX0";
	if (!_fmemcmp(MK_FP(FP_SEG(ptr), 10), EMSsign, 8))
	{
#ifdef __WATCOMC__

#ifdef __SW_ZDF		/* DS register is floating, SS points to DGROUP */
  #define DGRP 0x36
#else
  #define DGRP
#endif /* __SW_ZDF */

uint _detectEMS(void);
#pragma aux _detectEMS =	\
	"mov	ah,40h"		/* get EMM status */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"or	ah,ah"		\
	"jnz	noEMS"		/* AH=0 - EMM driver OK */	\
	"mov	ah,41h"		/* get EMM page frame */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"or	ah,ah"		\
	"jnz	noEMS"		/* AH=0 - no error */		\
	DGRP			\
	"mov	EMSframe,bx"	\
	"mov	ah,46h"		/* get EMM version */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"or	ah,ah"		\
	"jnz	noEMS"		/* AH=0 - no error */	\
	DGRP			\
	"mov	EMSversion,al"	\
	"mov	ax,1"		\
	"jmp	short end"	\
"noEMS:	 xor	ax,ax"		\
"end:	"			\
	parm nomemory		\
	modify exact [AX BX];
		
	    return EMSpresent = _detectEMS();
	}
    }
    return EMSpresent = 0;
#else
	    asm {
		mov	ah,40h		/* get EMM status */
		int	EMS_VECT
		or	ah,ah
		jnz	noEMS		/* AH=0 - EMM driver OK */

		mov	ah,41h		/* get EMM page frame */
		int	EMS_VECT
		or	ah,ah
		jnz	noEMS		/* AH=0 - no error */
		mov	EMSframe,bx

		mov	ah,46h		/* get EMM version */
		int	EMS_VECT
		or	ah,ah
		jnz	noEMS		/* AH=0 - no error */
//		cmp	al,40h
//		jb	noEMS		/* we require at least version 4.0 */
		mov	EMSversion,al
	    }
	    return EMSpresent = 1;
	}
    }
noEMS:
    return EMSpresent = 0;
#endif /* __WATCOMC__ */
}

#ifdef __WATCOMC__

WORD _allocEMS(WORD pages);
int freeEMS(WORD handle);
int mapEMSpage(WORD handle, WORD logPage, BYTE physPage);

#pragma aux _allocEMS =	\
	"mov	ah,43h"		/* open handle and allocate memory */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"or	ah,ah"		\
	"jnz	err"		/* AH=0 - no error */	\
	"mov	ax,dx"		\
	"jmp	short end"	\
"err:	 mov	ax,-1"		\
"end:"				\
	parm [BX] nomemory	\
	modify exact [AX DX] nomemory;

static WORD allocEMS(ulong size)
{
    return _allocEMS((size + (EMS_PAGESIZE - 1)) >> EMS_PAGESHIFT);
}

#pragma aux freeEMS =	\
	"mov	ah,45h"		/* close handle and release memory */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"mov	al,ah"		\
	"xor	ah,ah"		\
	parm [DX] nomemory	\
	modify exact [AX] nomemory;

#pragma aux mapEMSpage = \
	"mov	ah,44h"		/* map EMS page into physical memory */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"mov	al,ah"		\
	"xor	ah,ah"		\
	parm [DX][BX][AL] nomemory	\
	modify exact [AX] nomemory;

#else /* __WATCOMC__ */

static WORD allocEMS(ulong size)
{
    _BX = (size + (EMS_PAGESIZE - 1)) >> EMS_PAGESHIFT;
    asm {
	mov	ah,43h			/* open handle and allocate memory */
    /* 	mov	bx,pages  */
	int	EMS_VECT
	or	ah,ah
	jnz	err			/* AH=0 - no error */
    }
    return _DX;
err:
    return -1;
}

static int freeEMS(WORD handle)
{
    asm {
	mov	ah,45h			/* close handle and release memory */
	mov	dx,handle
	int	EMS_VECT
    }
    return _AH;
}

static int mapEMSpage(WORD handle, WORD logPage, BYTE physPage)
{
    asm {
	mov	ah,44h			/* map EMS page into physical memory */
	mov	al,physPage
	mov	bx,logPage
	mov	dx,handle
	int	EMS_VECT
    }
    return _AH;
}

#endif /* __WATCOMC__ */


struct EMSmoveInfo {
	DWORD	length;
	BYTE	srcType;		/* 0=conv, 1=EMS */
	WORD	srcHandle;
	WORD	srcOffset;
	WORD	srcSegment;
	BYTE	dstType;		/* 0=conv, 1=EMS */
	WORD	dstHandle;
	WORD	dstOffset;
	WORD	dstSegment;
};

#ifdef __WATCOMC__

int moveEMS(struct EMSmoveInfo near *move);
#ifdef __SW_ZDP		/* DS register is pegged to DGROUP */
#pragma aux moveEMS =		\
	"mov	ax,5700h"	/* move memory block, DS:SI - move info */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
	"mov	al,ah"		\
	"xor	ah,ah"		\
	parm [SI]		\
	modify exact [AX] nomemory;
#else /* __SW_ZDP */
#pragma aux moveEMS =		\
/*	"push	ds"	*/	\
	"mov	ax,ss"		\
	"mov	ds,ax"		\
	"mov	ax,5700h"	/* move memory block, DS:SI - move info */	\
	0xCD	EMS_VECT	/* int EMS_VECT */	\
/*	"pop	ds"	*/	\
	"mov	al,ah"		\
	"xor	ah,ah"		\
	parm [SI]		\
	modify exact [AX] nomemory;
#endif /* __SW_ZDP */

#else /* __WATCOMC__ */

#define moveEMS(move)		\
    asm {			\
	push	ds;		\
	mov	ax,ss;		\
	mov	ds,ax;		\
	lea	si,(move);	\
	mov	ax,5700h;	\
	int	EMS_VECT;	\
	pop	ds		\
    }

#endif /* __WATCOMC__ */

static int copyFromEMS(void far *dest, WORD srcHandle, ulong srcOffset, ulong length)
{
    struct EMSmoveInfo move;

    move.length = length;
    move.srcType = 1;
    move.srcHandle = srcHandle;
    move.srcOffset = (WORD)srcOffset & (EMS_PAGESIZE - 1);
    move.srcSegment = srcOffset >> EMS_PAGESHIFT;

    move.dstType = 0;
    move.dstHandle = 0;
    move.dstOffset = FP_OFF(dest);
    move.dstSegment = FP_SEG(dest);

#ifdef __WATCOMC__
    return moveEMS(&move);
#else
    moveEMS(move);
    return _AH;
#endif /* __WATCOMC__ */
}

#if 0					/* exclude from compilation */

static int copyToEMS(WORD dstHandle, ulong dstOffset, void far *src, ulong length)
{
    struct EMSmoveInfo move;

    move.length = length;
    move.srcType = 0;
    move.srcHandle = 0;
    move.srcOffset = FP_OFF(src);
    move.srcSegment = FP_SEG(src);

    move.dstType = 1;
    move.dstHandle = dstHandle;
    move.dstOffset = (WORD)dstOffset & (EMS_PAGESIZE - 1);
    move.dstSegment = dstOffset >> EMS_PAGESHIFT;

#ifdef __WATCOMC__
    return moveEMS(&move);
#else
    moveEMS(move);
    return _AH;
#endif /* __WATCOMC__ */
}

#endif /* 0 */

#define EMS_BLOCKSIZE 1024U
static int loadtoEMS(int fd, ulong length, struct memoryBlock *block)
{
    WORD handle;
    void far *buffer;
    uint buflen = min(length, EMS_BLOCKSIZE);
    if ( (handle = allocEMS(length)) != (WORD)-1 )
    {
	if ( (buffer = farmalloc(buflen)) != NULL )
	{
	    uint page = 0;
	    uint size, sizeread;

	    block->bufferType = BT_EMS;
	    block->handle = handle;
	    block->buffer = (BYTE __FAR *)buffer;
	    block->size = length;

	    while ( (size = min(length, EMS_PAGESIZE)) != 0)
	    {
		mapEMSpage(handle, page, 0);
		if (_dos_read(fd, EMS_FRAME(0), size, &sizeread) ||
		    sizeread != size)
		{
		    farfree(buffer);
		    freeEMS(handle);
		    return -1;
		}
		page++;
		length -= size;
	    }
	} else {
	    freeEMS(handle);
	    return -1;
	}
    } else
	return -1;

    block->bufAt = 0;
    block->bufLen = buflen;
    block->bufPos = 0;
    return copyFromEMS(buffer, handle, 0, buflen);
}

static int getbufEMS(struct memoryBlock *block)
{
    block->bufLen = min(block->size - block->bufAt, EMS_BLOCKSIZE);
    copyFromEMS(block->buffer, block->handle, block->bufAt, block->bufLen);
    return block->bufPos = 0;
}

#endif /* __DOSMEMORY__ */


/*
 * XMS memory management routines
 */

#ifdef __DOSMEMORY__

static BYTE XMSpresent = 0;
static void far *XMSentry = abort;

#define XMS_VECT	0x2F
#define XMS_DETECT	0x4300
#define XMS_INSTALLED	0x80
#define XMS_GETENTRY	0x4310

#define XMS_PAGESIZE	1024U
#define XMS_PAGESHIFT	10

#ifdef __WATCOMC__
uint _detectXMS(void);
#pragma aux _detectXMS =	\
	"mov	ax,4300h"	/* mov ax,XMS_DETECT */		\
	0xCD	XMS_VECT	/* int XMS_VECT */		\
	"cmp	al,80h"		/* cmp al,XMS_INSTALLED */	\
	"jne	noXMS"		\
	"mov	ax,4310h"	/* mov ax,XMS_GETENTRY */	\
	0xCD	XMS_VECT	/* int XMS_VECT */		\
	DGRP			\
	"mov	WORD PTR XMSentry[0],bx"	\
	DGRP			\
	"mov	WORD PTR XMSentry[2],es"	\
	"mov	ax,1"		\
	"jmp	short end"	\
"noXMS:	 xor	ax,ax"		\
"end:	"			\
	parm nomemory		\
	modify exact [AX BX ES];
		
static uint detectXMS(void)
{
    return XMSpresent = _detectXMS();
}

#else

static uint detectXMS(void)
{
    asm {
	mov	ax,XMS_DETECT
	int	XMS_VECT
	cmp	al,XMS_INSTALLED
	jne	noXMS

	mov	ax,XMS_GETENTRY
	int	XMS_VECT
    }
    XMSentry = MK_FP(_ES,_BX);
    return XMSpresent = 1;
noXMS:
    return XMSpresent = 0;
}

#endif /* __WATCOMC__ */


#ifdef __WATCOMC__

WORD _allocXMS(WORD pages);
int freeXMS(WORD handle);

#pragma aux _allocXMS =	\
	"mov	ah,09h"		/* open handle and allocate memory */	\
	DGRP			\
	"call	XMSentry"	\
	"dec	ax"		/* AX=1 - no error */	\
	"jnz	end"		\
	"mov	ax,dx"		\
"end:	"			\
	parm [DX] nomemory	\
	modify exact [AX DX] nomemory;

static WORD allocXMS(ulong size)
{
    return _allocXMS((size + (XMS_PAGESIZE - 1)) >> XMS_PAGESHIFT);
}

#pragma aux freeXMS =	\
	"mov	ah,0Ah"		/* close handle and release memory */	\
	DGRP			\
	"call	XMSentry"	\
	"dec	ax"		/* AX=1 - no error */	\
	parm [DX] nomemory	\
	modify exact [AX] nomemory;

#else /* __WATCOMC__ */

static WORD allocXMS(ulong size)
{
    uint pages = (size + (XMS_PAGESIZE - 1)) >> XMS_PAGESHIFT;
    asm {
	mov	ah,09h			/* open handle and allocate memory */
	mov	dx,pages
	call	XMSentry
	dec	ax			/* AX=1 - no error */
	jnz	err
    }
    return _DX;
err:
    return _AX;
}

static int freeXMS(WORD handle)
{
    asm {
	mov	ah,0Ah			/* close handle and release memory */
	mov	dx,handle
	call	XMSentry
	dec	ax			/* AX=1 - no error */
    }
    return _AX;
}

#endif /* __WATCOMC__ */

struct XMSmoveInfo {
	DWORD	length;
	WORD	srcHandle;
	DWORD	srcOffset;
	WORD	dstHandle;
	DWORD	dstOffset;
};

#ifdef __WATCOMC__

int performMoveXMS(struct XMSmoveInfo *move);

#ifdef NEARDATAPTR
#pragma aux performMoveXMS =	\
	"mov	ah,0Bh"		/* move memory block */	\
	DGRP			\
	"call	XMSentry"	\
	"dec	ax"		/* AX=1 - no error */	\
	parm [SI]		\
	modify exact [AX] nomemory;
#else
#pragma aux performMoveXMS =	\
	"push	ds"		\
	"mov	ds,dx"		\
	"mov	ah,0Bh"		/* move memory block */	\
	"call	ss:XMSentry"	\
	"pop	ds"		\
	"dec	ax"		/* AX=1 - no error */	\
	parm [DX SI]		\
	modify exact [AX] nomemory;
#endif /* NEARDATAPTR */

#else /* __WATCOMC__ */

static int performMoveXMS(struct XMSmoveInfo *move)
{
#ifdef NEARDATAPTR
    asm {
	mov	si,move
	mov	ah,0Bh			/* move memory block */
	call	XMSentry
	dec	ax			/* AX=1 - no error */
    }
#else
    asm {
	push	ds
	mov	ax,ds
	mov	es,ax
	lds	si,move			/* DS:SI - move info */
	mov	ah,0Bh			/* move memory block */
	call	es:XMSentry
	dec	ax			/* AX=1 - no error */
	pop	ds
    }
#endif /* NEARDATAPTR */
    return _AX;
}

#endif /* __WATCOMC__ */

static int copyFromXMS(void far *dest, WORD srcHandle, ulong srcOffset, ulong length)
{
    struct XMSmoveInfo move;

    move.length = (length + 1) & ~1L;	/* length must be even */
    move.srcHandle = srcHandle;
    move.srcOffset = srcOffset;

    move.dstHandle = 0;
    move.dstOffset = (DWORD)dest;

    return performMoveXMS(&move);
}

static uint copyToXMS(WORD dstHandle, ulong dstOffset, void far *src, ulong length)
{
    struct XMSmoveInfo move;

    move.length = (length + 1) & ~1L;
    move.srcHandle = 0;
    move.srcOffset = (DWORD)(void far *)src;

    move.dstHandle = dstHandle;
    move.dstOffset = dstOffset;

    return performMoveXMS(&move);
}

#define XMS_BLOCKSIZE (4*XMS_PAGESIZE)
static int loadtoXMS(int fd, ulong length, struct memoryBlock *block)
{
    WORD handle;
    void far *buffer;
    uint buflen = min(length, XMS_BLOCKSIZE);
    ulong offset = 0;
    if ( (handle = allocXMS(length)) != (WORD)-1 )
    {
	if ( (buffer = farmalloc(buflen)) != NULL )
	{
	    uint size, sizeread;

	    block->bufferType = BT_XMS;
	    block->handle = handle;
	    block->buffer = (BYTE __FAR *)buffer;
	    block->size = length;

	    while ( (size = min(length, buflen)) != 0)
	    {
		if (_dos_read(fd, buffer, size, &sizeread) ||
		    sizeread != size)
		{
		    farfree(buffer);
		    freeXMS(handle);
		    return -1;
		}
		copyToXMS(handle, offset, buffer, size);
		offset += size;
		length -= size;
	    }
	} else {
	    freeXMS(handle);
	    return -1;
	}
    } else
	return -1;

    block->bufAt = 0;
    block->bufLen = buflen;
    block->bufPos = 0;
    return copyFromXMS(buffer, handle, 0, buflen);
}

static int getbufXMS(struct memoryBlock *block)
{
    block->bufLen = min(block->size - block->bufAt, XMS_BLOCKSIZE);
    copyFromXMS(block->buffer, block->handle, block->bufAt, block->bufLen);
    return block->bufPos = 0;
}

#endif /* __DOSMEMORY__ */


/*
 * Conventional memory management routines
 */

#ifdef __386__

#define CONV_MEM(mem)	((BYTE *)(mem))

static DWORD allocConv(ulong size)
{
    void *ptr = malloc(size);
    return ptr ? (DWORD)ptr : -1LU;
}

static int freeConv(DWORD handle)
{
    if (handle != -1LU)
	free((void *)handle);
    return 0;
}

static int loadtoConv(int fd, ulong length, struct memoryBlock *block)
{
    DWORD handle;
    if ( (handle = allocConv(length)) != (DWORD)-1 )
    {
	uint sizeread;

	block->bufferType = BT_CONV;
	block->handle = handle;
	block->buffer = CONV_MEM(handle);
	block->size = length;

	if (_dos_read(fd, CONV_MEM(handle), length, &sizeread) ||
	    sizeread != length)
	{
	    freeConv(handle);
	    return -1;
	}
    } else
	return -1;

    block->bufAt = 0;
    block->bufLen = block->size;
    return block->bufPos = 0;
}

static int getbufConv(struct memoryBlock *block)
{
    block->buffer = CONV_MEM(block->handle);
    block->bufLen = block->size;
    return block->bufPos = 0;
}

#else /* __386__ */

#define CONV_PAGESIZE	16U
#define CONV_PAGESHIFT	4
#define CONV_MEM(mem)	((BYTE huge *)(mem))

static DWORD allocConv(ulong size)
{
#ifdef __WATCOMC__
    void far *ptr = halloc(size, 1);
#else
    void far *ptr = farmalloc(size);
#endif /* __WATCOMC__ */
    return ptr ? (DWORD)ptr : -1LU;
}

static int freeConv(DWORD handle)
{
    if (handle != -1LU)
#ifdef __WATCOMC__
	hfree((void far *)handle);
#else
	farfree((void far *)handle);
#endif /* __WATCOMC__ */
    return 0;
}

#define CONV_BLOCKSIZE 32768U
static int loadtoConv(int fd, ulong length, struct memoryBlock *block)
{
    DWORD handle;
    if ( (handle = allocConv(length)) != (DWORD)-1 )
    {
	ulong offset = 0;
	uint size, sizeread;

	block->bufferType = BT_CONV;
	block->handle = handle;
	block->buffer = (BYTE far *)CONV_MEM(handle);
	block->size = length;

	while ( (size = length > CONV_BLOCKSIZE ? CONV_BLOCKSIZE : length) != 0)
	{
	    if (_dos_read(fd, &CONV_MEM(handle)[offset], size, &sizeread) ||
		sizeread != size)
	    {
		freeConv(handle);
		return -1;
	    }
	    offset += size;
	    length -= size;
	}
    } else
	return -1;

    block->bufAt = 0;
    block->bufLen = min(block->size - block->bufAt, CONV_BLOCKSIZE);
    return block->bufPos = 0;
}

static int getbufConv(struct memoryBlock *block)
{
    block->buffer = (BYTE far *)&CONV_MEM(block->handle)[block->bufAt];
    block->bufLen = min(block->size - block->bufAt, CONV_BLOCKSIZE);
    return block->bufPos = 0;
}

#endif /* __386__ */


/*
 * Detect memory managers which can be used by MUSLIB
 * Returns:
 *   bit 0 - conventional memory present (always set)
 *   bit 1 - EMS driver present
 *   bit 2 - XMS driver present
 */
uint MEMdetect(void)
{
#ifdef __DOSMEMORY__
    return (detectEMS() << 1) | (detectXMS() << 2) | 0x01;
#else
    return 0x01;
#endif /* __DOSMEMORY__ */
}

/*
 * Load block of data from file to memory
 * Returns:
 *    0 - success
 *   -1 - error
 */
int MEMload(int fd, ulong length, struct memoryBlock *block, uint memory)
{
    memset(block, 0, sizeof *block);	/* initialize block descriptor */
#ifdef __DOSMEMORY__
    if (memory & 2 && EMSpresent && length > EMS_BLOCKSIZE)	/* EMS */
	if (!loadtoEMS(fd, length, block))
	    return 0;
    if (memory & 4 && XMSpresent && length > XMS_BLOCKSIZE)	/* XMS */
	if (!loadtoXMS(fd, length, block))
	    return 0;
    if (memory & 1 || memory & 2 && length <= EMS_BLOCKSIZE
		   || memory & 4 && length <= XMS_BLOCKSIZE)	/* conv */
	if (!loadtoConv(fd, length, block))
	    return 0;
#else
    if (memory & 1)						/* conv */
	if (!loadtoConv(fd, length, block))
	    return 0;
#endif /* __DOSMEMORY__ */
    return -1;
}

/*
 * Release block of data from memory
 * Returns:
 *    0 - success
 *   -1 - error
 */
int MEMfree(struct memoryBlock *block)
{
    switch (block->bufferType) {
	case BT_CONV:
	    return freeConv(block->handle);
#ifdef __DOSMEMORY__
	case BT_EMS:
	    farfree(block->buffer);
	    return freeEMS(block->handle);
	case BT_XMS:
	    farfree(block->buffer);
	    return freeXMS(block->handle);
#endif /* __DOSMEMORY__ */
	default:
	    return -1;
    }
}

/*
 * Get next character from memory block
 * Returns:
 *   0..255 - character
 *   -1 - error
 */
int MEMgetchar(struct memoryBlock *block)
{
    if (!block->buffer || block->bufPos >= block->bufLen)
	if ( (block->bufAt += block->bufPos) < block->size )
	{
	    switch (block->bufferType) {
		case BT_CONV:
		    getbufConv(block);
		    break;
#ifdef __DOSMEMORY__
		case BT_EMS:
		    getbufEMS(block);
		    break;
		case BT_XMS:
		    getbufXMS(block);
		    break;
#endif /* __DOSMEMORY__ */
		default:
		    return -1;
	    }
	} else
	    return -1;
    return block->buffer[block->bufPos++];
}

/*
 * Rewind memory block pointer to the beginning
 * Returns:
 *    0 - success
 *   -1 - error
 */
int MEMrewind(struct memoryBlock *block)
{
    if (block->bufAt)
    {
	block->bufAt = 0;
	switch (block->bufferType) {
	    case BT_CONV:
		getbufConv(block);
		break;
#ifdef __DOSMEMORY__
	    case BT_EMS:
		getbufEMS(block);
		break;
	    case BT_XMS:
		getbufXMS(block);
		break;
#endif /* __DOSMEMORY__ */
	    default:
		return -1;
	}
    }
    return block->bufPos = 0;
}
