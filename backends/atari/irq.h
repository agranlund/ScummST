//-------------------------------------------------------------------------
// This file is distributed under the GPL v2, or at your option any
// later version.  See COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------
#ifndef _ATARI_IRQ_
#define _ATARI_IRQ_

#include "common/stdafx.h"
#include "common/scummsys.h"

#define MFP(offs) ((volatile byte*)(0x00FFFA00+offs))


void irq_init();	    // init irq stuff

extern volatile int16 __irqdisable;
extern volatile int16 __irqdisablesr;

inline void irq_disable()
{
    __asm__ volatile ( "    ori.w #0x0700,sr" : : : "cc");
    __asm__ volatile ( "    move.w sr,%0" : "=d"(__irqdisablesr) : : "cc");
    __irqdisablesr &= 0xF0FF;
    __irqdisablesr |= 0x0300;
    __irqdisable++;
}

inline void irq_enable()
{
    if (__irqdisable > 0)
    {
        __irqdisable--;
        if (__irqdisable == 0)
        {
            __asm__ volatile ("     move.w %0,sr" : : "d"(__irqdisablesr) : "cc");
        }
    }
}


void irq_install_vbl(void (*func)());
void irq_remove_vbl(void (*func)());
bool irq_enable_vbl(bool onoff);

void irq_install_timerA(void (*func)(), int hz);
void irq_install_timerA(void (*func)(), uint16 ctrl, uint16 data);
void irq_enable_timerA(bool onoff);
inline void irq_serviced_timerA() { *MFP(0x0F) &= ~(1<<0); }

void irq_install_timerB(void (*func)(), uint16 ctrl, uint16 data);
void irq_enable_timerB(bool onoff);
inline void irq_serviced_timerB() { *MFP(0x0F) &= ~(1<<0); }




#endif //_ATARI_IRQ_

