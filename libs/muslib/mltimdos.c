/*
 *	Name:		DOS Timer Module
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
 *		DOS-related code moved to this module (MLTIMDOS.C)
 */

#if defined(__WINDOWS__) || defined(_Windows)
  #error This module may be compiled for DOS only
#endif

#include <dos.h>
#include <mem.h>
#include <malloc.h>
#include "muslib.h"

#if defined(__WATCOMC__) && defined(FARDATAPTR)
  #define STATIC_STACK
#endif

#ifdef __cplusplus
    #define __CPPARGS ...
#else
    #define __CPPARGS void
#endif

#ifdef __cplusplus
static void interrupt far (*oldint70h)(__CPPARGS);
static void interrupt far (*oldint08h)(__CPPARGS);
#else
static void (interrupt far *oldint70h)(__CPPARGS);
static void (interrupt far *oldint08h)(__CPPARGS);
#endif

#define STACK_SIZE 0x200		/* 512 words of stack */

#ifdef STATIC_STACK
static uint	timerstack[STACK_SIZE];
#else
static uint	*timerstack = NULL;
#endif

static uint far *timerstackend = NULL;
static volatile uint far *timersavestack = NULL;

static uint	timersOn = 0;
static uint	timerMode = TIMER_RTC1024;
static uint	timerRate = 1024;


/*
 * Perform one timer tick using 18.2 Hz clock
 */
#define RATE140HZ	8523			/* 1193180 / 140 */

static void playMusic18_2Hz(void)
{
    static DWORD ticks = 0;

    while ( !((WORD *)&ticks)[1] )		/* while (ticks < 65536) */
    {
	MLplayerInterrupt();
	ticks += RATE140HZ;
    }
    ((WORD *)&ticks)[1] = 0;			/* ticks &= 0xFFFF */
}


#ifdef __WATCOMC__

/*
 * Watcom C specific code
 */
void CMOSwrite(BYTE reg, BYTE value);
BYTE CMOSread(BYTE reg);
void EOIsignal1(void);				/* end-of-interrupt signal */
void EOIsignal2(void);
void setEStoDS(void);
void enterStack(void);
void restoreStack(void);

#define DELAY 0xEB 0x00  			/* jmp $+2 */

#pragma aux CMOSwrite =		\
	"out	70h,al"		\
	DELAY			\
	DELAY			\
	"mov	al,ah"		\
	"out	71h,al"		\
	parm [AL][AH] nomemory	\
	modify exact [AL] nomemory;

#pragma aux CMOSread =		\
	"out	70h,al"		\
	DELAY			\
	DELAY			\
	"in	al,71h"		\
	parm [AL] nomemory	\
	modify exact [AL] nomemory;	

#pragma aux EOIsignal1 =	\
	"mov	al,20h"		\
	"out	20h,al"		\
	modify exact [AL] nomemory;

#pragma aux EOIsignal2 =	\
	"mov	al,20h"		\
	"out	0A0h,al"	\
	DELAY			\
	"out	20h,al"		\
	modify exact [AL] nomemory;

#pragma aux setEStoDS =	\
	"mov	ax,ds"		\
	"mov	es,ax"		\
	modify exact [AX] nomemory;

#ifdef __386__
#pragma aux enterStack =	\
	"mov	WORD PTR timersavestack[4],ss"	\
	"mov	DWORD PTR timersavestack[0],esp" \
	"lss	esp,timerstackend"		\
	"pushad";
#else
#pragma aux enterStack =	\
	"mov	WORD PTR timersavestack[2],ss"	\
	"mov	WORD PTR timersavestack[0],sp"	\
	"lss	sp,timerstackend"		\
	"pushad";
#endif

#ifdef __386__
#pragma aux restoreStack =	\
	"popad"			\
	"lss	esp,timersavestack";
#else
#pragma aux restoreStack =	\
	"popad"			\
	"lss	sp,timersavestack";
#endif

#else /* __WATCOMC__ */

/*
 * Borland C specific code
 */
#define DELAY db 0EBh,00h  			/* jmp $+2 */

static void FASTCALL CMOSwrite(BYTE reg, BYTE value)
{
    asm {
	mov	al,reg
	out	70h,al
	DELAY
	DELAY
	mov	al,value
	out	71h,al
    }
}

static int FASTCALL CMOSread(BYTE reg)
{
    asm {
	mov	al,reg
	out	70h,al
	DELAY
	DELAY
	in	al,71h
	xor	ah,ah
    }
    return _AX;
}

#define EOIsignal1()		\
    asm {			\
	mov	al,20h;		\
	out	20h,al		\
    }

#define EOIsignal2()		\
    asm {			\
	mov	al,20h;		\
	out	0A0h,al;	\
	DELAY;			\
	out	20h,al		\
    }

#define setEStoDS()		\
    asm {			\
	mov	ax,ds;		\
	mov	es,ax		\
    }

#define enterStack()		\
    asm {			\
	mov	WORD PTR timersavestack[2],ss;	\
	mov	WORD PTR timersavestack[0],sp;	\
	mov	ss,WORD PTR timerstackend[2];	\
	mov	sp,WORD PTR timerstackend[0];	\
	db 66h; pusha	/* *386* pushad */	\
    }

#define restoreStack()		\
    asm {			\
	db 66h; popa;	/* *386* popad */	\
	mov	ss,WORD PTR timersavestack[2];	\
	mov	sp,WORD PTR timersavestack[0]	\
    }

#endif /* __WATCOMC__ */


/*
 * System timer (IRQ 0, INT 08h) handler
 */
static void interrupt newint08h_handler(__CPPARGS)
{
    static volatile uint count = 0;
    static volatile ulong ticks = 0;

    setEStoDS();
    if (!count)					/* reentrancy safeguard */
    {
	count++;
	enterStack();				/* swap stacks */
	if (timerMode == TIMER_CNT18_2)
	    playMusic18_2Hz();
	else
	    MLplayerInterrupt();
	restoreStack();
	count--;
    }

    if (timerMode == TIMER_CNT18_2)
	(*oldint08h)();				/* invoke original handler */
    else {
	ticks += RATE140HZ;
	if ( ((WORD *)&ticks)[1] )		/* if (ticks >= 65536L) */
	{
	    ((WORD *)&ticks)[1] = 0;		/* ticks &= 0xFFFF */
	    (*oldint08h)();
	} else
	    EOIsignal1();			/* end-of-interrupt signal */
    }
}


/*
 * Real-time clock (IRQ 8, INT 70h) handler
 */
#define RTC_RATE	0x0A
#define RTC_REG		0x0B
#define RTC_STATUS	0x0C
#define RTC_MASK	0x40

static void interrupt newint70h_handler(__CPPARGS)
{
    static volatile uint count = 0;
    static volatile int ticks = 0;

    setEStoDS();
    if ( !(CMOSread(RTC_STATUS) & RTC_MASK) )	/* not periodic interrupt */
    {
	(*oldint70h)();				/* invoke original handler */
	return;
    }

    if (!count)					/* reentrancy safeguard */
    {
	count++;
	enterStack();				/* swap stacks */
	ticks += 140;
	while (ticks > 0)
	{
	    ticks -= timerRate;
	    MLplayerInterrupt();
	}
	restoreStack();
	count--;
    }
    EOIsignal2();				/* end-of-interrupt signal */
}


#ifdef __WATCOMC__

void PITset(WORD value);
void PITreset(void);

#pragma aux PITset =		\
	"cli"			\
	"mov	al,36h"		\
	"out	43h,al"		\
	"mov	ax,dx"		\
	"out	40h,al"		\
	"xchg	al,ah"		\
	"out	40h,al"		\
	"sti"			\
	parm [DX]		\
	modify exact [AX] nomemory;

#pragma aux PITreset =		\
	"cli"			\
	"mov	al,36h"		\
	"out	43h,al"		\
	"xor	al,al"		\
	"out	40h,al"		\
	"out	40h,al"		\
	"sti"			\
	modify exact [AL] nomemory;

#else /* __WATCOMC__ */

#define PITset(value)		\
    asm {			\
	cli;			\
	mov	al,36h;		\
	out	43h,al;		\
	mov	ax,value;	\
	out	40h,al;		\
	xchg	al,ah;		\
	out	40h,al;		\
	sti			\
    }

#define PITreset()		\
    asm {			\
	cli;			\
	mov	al,36h;		\
	out	43h,al;		\
	xor	al,al;		\
	out	40h,al;		\
	out	40h,al;		\
	sti			\
    }

#endif /* __WATCOMC__ */


/*
 * Start system timer
 */
static int SetupTimer1(uint divisor)
{
    if (timersOn & 1)				/* already initialized? */
	return 0;
#ifndef STATIC_STACK
    if ( (timerstack = (uint *)calloc(STACK_SIZE, sizeof(int))) == NULL)
	return -1;
#endif
    timerstackend = &timerstack[STACK_SIZE];
    oldint08h = _dos_getvect(0x08);
    _dos_setvect(0x08, newint08h_handler);
    PITset(divisor);
    timersOn |= 1;
    return 0;
}

/*
 * Stop system timer
 */
static int ShutdownTimer1(uint restoreRate)
{
    if (!(timersOn & 1))			/* is initialized? */
	return -1;
    if (restoreRate)
	PITreset();
    _dos_setvect(0x08, oldint08h);
#ifndef STATIC_STACK
    free(timerstack);
#endif
    timersOn &= ~1;
    return 0;
}


#ifdef __WATCOMC__

void IRQ8enable(void);
void IRQ8disable(void);

#pragma aux IRQ8enable =	\
	"in	al,0A1h"	\
	"and	al,0FEh"	\
	"out	0A1h,al"	\
	modify exact [AL] nomemory;
		
#pragma aux IRQ8disable =	\
	"in	al,0A1h"	\
	"or	al,1"		\
	"out	0A1h,al"	\
	modify exact [AL] nomemory;

#else /* __WATCOMC__ */

#define IRQ8enable()		\
    asm {			\
	in	al,0A1h;	\
	and	al,not 1;	\
	out	0A1h,al		\
    }

#define IRQ8disable()		\
    asm {			\
	in	al,0A1h;	\
	or	al,1;		\
	out	0A1h,al		\
    }

#endif /* __WATCOMC__ */


#ifdef __386__
#define RTC_TIMER_FLAG	*((BYTE *)0x0000004A0)
#else
#define RTC_TIMER_FLAG	*(BYTE far *)MK_FP(0x0040, 0x00A0)
#endif /* __386__ */

/*
 * Start RTC timer
 */
static int SetupTimer2(uint rate, uint int_rate)
{
    if (timersOn & 2)				/* already initialized? */
	return 0;
    if (RTC_TIMER_FLAG)				/* is the timer busy? */
	return -1;
#ifndef STATIC_STACK
    if ( (timerstack = (uint	*)calloc(STACK_SIZE, sizeof(int))) == NULL)
	return -1;
#endif
    timerstackend = &timerstack[STACK_SIZE];
    timerRate = int_rate;
    RTC_TIMER_FLAG = 1;				/* mark the timer as busy */
    oldint70h = _dos_getvect(0x70);
    _dos_setvect(0x70, newint70h_handler);
    if (rate)
	CMOSwrite(RTC_RATE, rate);		/* change interrupt rate */
    CMOSwrite(RTC_REG, CMOSread(RTC_REG) | RTC_MASK); /* enable periodic interrupt */
    IRQ8enable();
    timersOn |= 2;
    return 0;
}

/*
 * Stop RTC timer
 */
static int ShutdownTimer2(uint rate)
{
    if (!(timersOn & 2))			/* is initialized? */
	return -1;
    CMOSwrite(RTC_REG, CMOSread(RTC_REG) & ~RTC_MASK); /* disable periodic interrupt */
    if (rate)
	CMOSwrite(RTC_RATE, rate);		/* restore interrupt rate */
    IRQ8disable();
    _dos_setvect(0x70, oldint70h);
#ifndef STATIC_STACK
    free(timerstack);
#endif
    RTC_TIMER_FLAG = 0;				/* mark the timer as unused */
    timersOn &= ~2;
    return 0;
}


/*
 * API: Initialize and start the timer
 */
int MLinitTimer(int mode)
{
    MLtime = 0;
    switch (timerMode = mode) {
	case TIMER_CNT18_2:
	    return SetupTimer1(0);
	case TIMER_CNT140:
	    return SetupTimer1(RATE140HZ);	/* 1193180 / 140 */
	case TIMER_RTC1024:
	    return SetupTimer2(0, 1024);	/* RTC default rate */
	case TIMER_RTC512:
	    return SetupTimer2(0x27, 512);
	case TIMER_RTC256:
	    return SetupTimer2(0x28, 256);
	case TIMER_RTC128:
	    return SetupTimer2(0x29, 128);
	case TIMER_RTC64:
	    return SetupTimer2(0x2A, 64);
	default:
	    return -1;
    }
}

/*
 * API: Deinitialize and stop the timer
 */
int MLshutdownTimer(void)
{
    switch (timerMode) {
	case TIMER_CNT18_2:
	    return ShutdownTimer1(0);
	case TIMER_CNT140:
	    return ShutdownTimer1(1);
	case TIMER_RTC1024:
	    return ShutdownTimer2(0);
	case TIMER_RTC512:
	case TIMER_RTC256:
	case TIMER_RTC128:
	case TIMER_RTC64:
	    return ShutdownTimer2(0x26);
	default:
	    return -1;
    }
}
