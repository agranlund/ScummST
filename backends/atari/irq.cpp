//-------------------------------------------------------------------------
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------
#include "irq.h"

//-------------------------------------------------------------------------------------

// investigate TT and timerA

//-------------------------------------------------------------------------------------

void mfp_soft_end_mode(bool onoff);



//-------------------------------------------------------------------------------------
volatile int16 		_irq_enable_sem = 1;
volatile int16		__irqdisable = 0;
volatile int16		__irqdisablesr = 0;

uint32*				_old_vblq = 0;
uint32*				_new_vblq = 0;
uint16				_old_vbls = 0;
uint16				_new_vbls = 0;

static byte _timerA_ctrl = 0;
static byte _timerB_ctrl = 0;
static byte _timerC_ctrl = 0;
static byte _timerD_ctrl = 0;


//-------------------------------------------------------------------------------------
void irq_init()
{
	__irqdisable = 0;
	__irqdisablesr = 0;
	irq_disable();
	mfp_soft_end_mode(true);
	irq_enable();
}


//-------------------------------------------------------------------------------------
void irq_install_vbl(void (*func)())
{
	irq_disable();
	// vbl queue list and size
	_old_vblq = (uint32*) *((volatile uint32*)0x00000456);
	_old_vbls = *((volatile uint16*)0x00000454);

	// find an empty slot
	for (int i=0; i<_old_vbls; i++)
	{
		if (_old_vblq[i] == 0)
		{
			// claim this slot, restore vbl processing and return
			_old_vblq[i] = (uint32) func;
			irq_enable();
			return;
		}
	}

	// list was full so install a new one
	_new_vbls = _old_vbls + 1;
	_new_vblq = (uint32*) malloc((_new_vbls) * sizeof(uint32*));
	memcpy(_new_vblq, _old_vblq, _old_vbls * sizeof(uint32*));
	_new_vblq[_old_vbls] = (uint32)func;
	*((volatile uint32*)0x00000456) = (uint32) _new_vblq;
	*((volatile uint16*)0x00000454) = _new_vbls;
	irq_enable();
}

//-------------------------------------------------------------------------------------
void irq_remove_vbl(void (*func)())
{
	irq_disable();
	// vbl queue list and size
	uint32* vblq = (uint32*) *((volatile uint32*)0x00000456);
	uint16 vbls  = *((volatile uint16*)0x00000454);

	// find func
	for (int i=0; i<vbls; i++)
		if (vblq[i] == (uint32)func)
			vblq[i] = 0;

	if (_new_vblq)
	{
		*((volatile uint32*)0x00000456) = (uint32) _old_vblq;
		*((volatile uint16*)0x00000454) = _old_vbls;
		free(_new_vblq);
		_new_vblq = 0;
		_new_vbls = 0;
		_old_vblq = 0;
		_old_vbls = 0;
	}

	irq_enable();
	return;
}


bool irq_enable_vbl(bool onoff)
{
	irq_disable();
	volatile int16* vblsem = (volatile int16*)0x00000452;
	int16 oldsem = *vblsem;
	(*vblsem) = onoff ? 1 : 0;
	irq_enable();
	return (oldsem > 0) ? true : false;
}

//-------------------------------------------------------------------------------------
void mfp_soft_end_mode(bool onoff)
{
	if (onoff)	*MFP(0x17) |=  (1<<3);
	else		*MFP(0x17) &= ~(1<<3);
}

void mfp_install_timer(uint16 idx, uint16 ctrl, uint16 data, uint32 vec)
{
	irq_disable();
	switch(idx)
	{
		case 13:		// timer A
			_timerA_ctrl = ctrl;
			*MFP(0x19) = 0;
			*MFP(0x07) |= (1 << 5);
			*MFP(0x13) |= (1 << 5);
			*MFP(0x1F) = data;
			break;
		case 8:			// timer B
			_timerB_ctrl = ctrl;
			*MFP(0x1B) = 0;
			*MFP(0x07) |= (1 << 0);
			*MFP(0x13) |= (1 << 0);
			*MFP(0x21) = data;
			break;
		case 5:			// timer C
			_timerC_ctrl = ctrl;
			*MFP(0x1D) &= 0x0F;
			*MFP(0x09) |= (1 << 5);
			*MFP(0x15) |= (1 << 5);			
			*MFP(0x21) = data;
			break;
		case 4:			// timer D
			_timerD_ctrl = ctrl;
			*MFP(0x1D) &= 0xF0;
			*MFP(0x09) |= (1 << 4);
			*MFP(0x15) |= (1 << 4);			
			*MFP(0x21) = data;
			break;
		default:		// invalid
			irq_enable();
			return;
	}

	uint32 base = *MFP(0x17) & 0xF0; 
	*(volatile uint32*)((base + idx) * 4) = vec;
	irq_enable();
}


void mfp_enable(uint16 idx, bool onoff)
{
	irq_disable();
	switch (idx)
	{
		case 13:
			*MFP(0x19) = onoff ? _timerA_ctrl : 0;
			break;

		case 8:
			*MFP(0x1B) = onoff ? _timerB_ctrl : 0;
			break;

		case 5:
			if (onoff)
				*MFP(0x1D) |= (_timerC_ctrl << 4);
			else
				*MFP(0x1F) &= 0x8F;
			break;

		case 4:
			if (onoff)
				*MFP(0x1D) |= _timerD_ctrl;
			else
				*MFP(0x1F) &= 0xF8;
			break;
	}
	irq_enable();
}


//-------------------------------------------------------------------------------------
void irq_install_timerA(void (*func)(), int hz)
{
	const uint32 dividers[7] = {4, 10, 16, 50, 64, 100, 200};
	const uint32 baseclk = 2457600;

	int requested = (int) hz;
	int bestDiff = 0xFFFFFF;
	int bestVal;
	uint16 bestCtrl = 1;
	uint16 bestData = 1;


	for (int i=7; i!=0; i--)
	{
		uint32 val0 = baseclk / dividers[i];
		for (int j=1; j<256; j++)
		{
			int val = val0 / j;
			int diff = (hz > val) ? (hz - val) : (val - hz);
			if (diff < bestDiff)
			{
				bestDiff = diff;
				bestVal = val;
				bestCtrl = (i+1);
				bestData = j;
			}
			if (val < hz)
				break;
		}
	}
	irq_install_timerA(func, bestCtrl, bestData);
}

void irq_install_timerA(void (*func)(), uint16 ctrl, uint16 data) {
	mfp_install_timer(13, ctrl, data, (uint32)func);
}

void irq_enable_timerA(bool onoff) {
	mfp_enable(13, onoff);
}

void irq_install_timerB(void (*func)(), uint16 ctrl, uint16 data) {
	mfp_install_timer(8, ctrl, data, (uint32)func);
}

void irq_enable_timerB(bool onoff) {
	return mfp_enable(8, onoff);
}




