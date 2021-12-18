/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2004 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /cvsroot/scummvm/scummvm/scumm/gfx.cpp,v 2.266 2004/02/14 10:43:55 eriktorbjorn Exp $
 *
 */


//
// todo:
//		draw directly to physical screen, get rid of virtual->physical copy
//		double buffering needs to be handled in scummvm layer
//		store data in ST-LOW format and get rid of MOVEP
//

#include "stdafx.h"
#include "scumm/scumm.h"
#include "scumm/actor.h"
#include "scumm/charset.h"
#include "scumm/resource.h"

#define C2PTABLE256SIZE	(256*8)
uint32 c2ptable256[C2PTABLE256SIZE * 2] = { 0 };
uint32 c2pfill256[256] = { 0 };

//#define DEBUG_DIRTY_REGIONS
//#define DEBUG_REDRAWSTRIP
//#define DEBUG_REDRAWBACKBUF
//#define DEBUG_MASKS

namespace Scumm {

#ifdef GAME_SAMNMAX
bool _hack_samnmax_intro = false;
#endif

enum {
	kScrolltime = 500,  // ms scrolling is supposed to take
	kPictureDelay = 20
};

#define MAKE_SCREENPTR(base, pitch, x, y) (base + ((x)>>1) + (((y)*(pitch)) >> 1))


#define NUM_SHAKE_POSITIONS 8
static const int8 shake_positions[NUM_SHAKE_POSITIONS] = {
//	0, 2, 4, 2, 0, 4, 6, 2
//	0, 0, 8, 0, 8, 8, 0, 8
	0, 0, -8, 0, 8, 8, 0, 8
};

/**
 * The following structs define four basic fades/transitions used by 
 * transitionEffect(), each looking differently to the user.
 * Note that the stripTables contain strip numbers, and they assume
 * that the screen has 40 vertical strips (i.e. 320 pixel), and 25 horizontal
 * strips (i.e. 200 pixel). There is a hack in transitionEffect that
 * makes it work correctly in games which have a different screen height
 * (for example, 240 pixel), but nothing is done regarding the width, so this
 * code won't work correctly in COMI. Also, the number of iteration depends
 * on min(vertStrips, horizStrips}. So the 13 is derived from 25/2, rounded up.
 * And the 25 = min(25,40). Hence for Zak256 instead of 13 and 25, the values
 * 15 and 30 should be used, and for COMI probably 30 and 60. 
 */
struct TransitionEffect {
	byte numOfIterations;
	int8 deltaTable[16];	// four times l / t / r / b
	byte stripTable[16];	// ditto
};

static const TransitionEffect transitionEffects[5] = {
	// Iris effect (looks like an opening/closing camera iris)
	{
		13,		// Number of iterations
		{
			1,  1, -1,  1,
		   -1,  1, -1, -1,
			1, -1, -1, -1,
			1,  1,  1, -1
		},
		{
			0,  0, 39,  0,
		   39,  0, 39, 24,
			0, 24, 39, 24,
			0,  0,  0, 24
		}
	},
	
	// Box wipe (a box expands from the upper-left corner to the lower-right corner)
	{
		25,		// Number of iterations
		{
			0,  1,  2,  1,
			2,  0,  2,  1,
			2,  0,  2,  1,
			0,  0,  0,  0
		},
		{
			0,  0,  0,  0,
			0,  0,  0,  0,
			1,  0,  1,  0,
		  255,  0,  0,  0
		}
	},
	
	// Box wipe (a box expands from the lower-right corner to the upper-left corner)
	{
		25,		// Number of iterations
		{
		   -2, -1,  0, -1,
		   -2, -1, -2,  0,
		   -2, -1, -2,  0,
			0,  0,  0,  0
		},
		{
		   39, 24, 39, 24,
		   39, 24, 39, 24,
		   38, 24, 38, 24,
		  255,  0,  0,  0
		}
	},
	
	// Inverse box wipe
	{
		25,		// Number of iterations
		{
			0, -1, -2, -1,
		   -2,  0, -2, -1,
		   -2,  0, -2, -1,
		    0,  0,  0,  0
		},
		{
			0, 24, 39, 24,
		   39,  0, 39, 24,
		   38,  0, 38, 24,
		  255,  0,  0,  0
		}
	},

	// Inverse iris effect, specially tailored for V1/V2 games
	{
		9,		// Number of iterations
		{
			-1, -1,  1, -1,
			-1,  1,  1,  1,
			-1, -1, -1,  1,
			 1, -1,  1,  1
		},
		{
			 7, 7, 32, 7,
			 7, 8, 32, 8,
			 7, 8,  7, 8,
			32, 7, 32, 8
		}
	}
};

#ifdef DEBUG_REDRAWSTRIP
static bool _debugRedrawBitmap = false;
static uint8 _debugStripColor = 0;
#endif

#define CPSTRIPM(x)			case x: mask = rptTable[*(m--)]; d[40 * (x - 1)] = ((d[40 * (x - 1)] & mask) | s[soffs]); soffs -= spitch;
#define CPSTRIPM_DEC_SRC(x)	case x: mask = rptTable[*(m--)]; d[40 * (x - 1)] = ((d[40 * (x - 1)] & mask) | *s--);
#define CPSTRIP(x)			case x: d[40 * (x - 1)] = s[soffs]; soffs -= spitch;
#define CPSTRIP_DEC_SRC(x)	case x: d[40 * (x - 1)] = *s--;
#define FLSTRIP(x)			case x: d[40 * (x - 1)] = c;

#define JMPTABLE200(_mval, _mfunc)	\
	switch(_mval) { \
		_mfunc(200) \
		_mfunc(199) _mfunc(198) _mfunc(197) _mfunc(196) _mfunc(195) _mfunc(194) _mfunc(193) _mfunc(192) _mfunc(191) _mfunc(190) \
		_mfunc(189) _mfunc(188) _mfunc(187) _mfunc(186) _mfunc(185) _mfunc(184) _mfunc(183) _mfunc(182) _mfunc(181) _mfunc(180) \
		_mfunc(179) _mfunc(178) _mfunc(177) _mfunc(176) _mfunc(175) _mfunc(174) _mfunc(173) _mfunc(172) _mfunc(171) _mfunc(170) \
		_mfunc(169) _mfunc(168) _mfunc(167) _mfunc(166) _mfunc(165) _mfunc(164) _mfunc(163) _mfunc(162) _mfunc(161) _mfunc(160) \
		_mfunc(159) _mfunc(158) _mfunc(157) _mfunc(156) _mfunc(155) _mfunc(154) _mfunc(153) _mfunc(152) _mfunc(151) _mfunc(150) \
		_mfunc(149) _mfunc(148) _mfunc(147) _mfunc(146) _mfunc(145) _mfunc(144) _mfunc(143) _mfunc(142) _mfunc(141) _mfunc(140) \
		_mfunc(139) _mfunc(138) _mfunc(137) _mfunc(136) _mfunc(135) _mfunc(134) _mfunc(133) _mfunc(132) _mfunc(131) _mfunc(130) \
		_mfunc(129) _mfunc(128) _mfunc(127) _mfunc(126) _mfunc(125) _mfunc(124) _mfunc(123) _mfunc(122) _mfunc(121) _mfunc(120) \
		_mfunc(119) _mfunc(118) _mfunc(117) _mfunc(116) _mfunc(115) _mfunc(114) _mfunc(113) _mfunc(112) _mfunc(111) _mfunc(110) \
		_mfunc(109) _mfunc(108) _mfunc(107) _mfunc(106) _mfunc(105) _mfunc(104) _mfunc(103) _mfunc(102) _mfunc(101) _mfunc(100) \
		_mfunc( 99) _mfunc( 98) _mfunc( 97) _mfunc( 96) _mfunc( 95) _mfunc( 94) _mfunc( 93) _mfunc( 92) _mfunc( 91) _mfunc( 90) \
		_mfunc( 89) _mfunc( 88) _mfunc( 87) _mfunc( 86) _mfunc( 85) _mfunc( 84) _mfunc( 83) _mfunc( 82) _mfunc( 81) _mfunc( 80) \
		_mfunc( 79) _mfunc( 78) _mfunc( 77) _mfunc( 76) _mfunc( 75) _mfunc( 74) _mfunc( 73) _mfunc( 72) _mfunc( 71) _mfunc( 70) \
		_mfunc( 69) _mfunc( 68) _mfunc( 67) _mfunc( 66) _mfunc( 65) _mfunc( 64) _mfunc( 63) _mfunc( 62) _mfunc( 61) _mfunc( 60) \
		_mfunc( 59) _mfunc( 58) _mfunc( 57) _mfunc( 56) _mfunc( 55) _mfunc( 54) _mfunc( 53) _mfunc( 52) _mfunc( 51) _mfunc( 50) \
		_mfunc( 49) _mfunc( 48) _mfunc( 47) _mfunc( 46) _mfunc( 45) _mfunc( 44) _mfunc( 43) _mfunc( 42) _mfunc( 41) _mfunc( 40) \
		_mfunc( 39) _mfunc( 38) _mfunc( 37) _mfunc( 36) _mfunc( 35) _mfunc( 34) _mfunc( 33) _mfunc( 32) _mfunc( 31) _mfunc( 30) \
		_mfunc( 29) _mfunc( 28) _mfunc( 27) _mfunc( 26) _mfunc( 25) _mfunc( 24) _mfunc( 23) _mfunc( 22) _mfunc( 21) _mfunc( 20) \
		_mfunc( 19) _mfunc( 18) _mfunc( 17) _mfunc( 16) _mfunc( 15) _mfunc( 14) _mfunc( 13) _mfunc( 12) _mfunc( 11) _mfunc( 10) \
		_mfunc(  9) _mfunc(  8) _mfunc(  7) _mfunc(  6) _mfunc(  5) _mfunc(  4) _mfunc(  3) _mfunc(  2) _mfunc(  1) \
	}


static inline void copyStrip(byte* dst, const byte* src, uint16 srcpitch, uint16 height)
{
	const uint32* s = (const uint32*) (((uint32)src) & ~1);
	uint32* d = (uint32*) (((uint32)dst) & ~1);
	uint16 spitch = srcpitch;
	int16 soffs = spitch * (height - 1);
	JMPTABLE200(height,CPSTRIP);
}

static inline void copyStrip1(byte* dst, const byte* src, uint16 height)
{
	const uint32* s = (const uint32*)(((uint32)src) & ~1);
	uint32* d = (uint32*)(((uint32)dst) & ~1);
	s += (height - 1);
	JMPTABLE200(height,CPSTRIP_DEC_SRC);
}

static inline void copyStripMasked(byte* dst, const byte* src, byte* msk, uint32 srcpitch, uint16 height)
{
	const uint32* s = (const uint32*) (((uint32)src) & ~1);
	uint32* d = (uint32*) (((uint32)dst) & ~1);
	byte*   m = msk + (height - 1);
	uint32 mask;
	uint16 spitch = srcpitch;
	int16 soffs = spitch * (height - 1);
	JMPTABLE200(height,CPSTRIPM);
}

static inline void copyStripMasked1(byte* dst, byte* src, byte* msk, uint16 height)
{
	const uint32* s = (const uint32*) (((uint32)src) & ~1);
	uint32* d = (uint32*) (((uint32)dst) & ~1);
	byte*   m = msk + (height - 1);
	s += (height - 1);
	uint32 mask;
	JMPTABLE200(height,CPSTRIPM_DEC_SRC);
}

static inline void fillStrip(byte* ptr, byte color, uint16 height)
{
	uint32* d = (uint32*) (((uint32)ptr) & ~1);
	uint32 c = c2pfill256[color];
	JMPTABLE200(height,FLSTRIP);
}

static inline void fillStrips(byte* ptr, byte color, uint16 strips, uint16 height)
{
	uint32* d = (uint32*) (((uint32)ptr) & ~1);
	uint32 c = c2pfill256[color];
	while(strips > 0)
	{
		JMPTABLE200(height,FLSTRIP);
		d++;
		strips--;
	}
}



#pragma mark -
#pragma mark --- Virtual Screens ---
#pragma mark -



Gdi::Gdi(ScummEngine *vm) {
	memset(this, 0, sizeof(*this));
	_vm = vm;
	_roomPalette = vm->_roomPalette;
	if (vm->_features & GF_AMIGA)
		_roomPalette += 16;
}

void ScummEngine::initScreens(int b, int h) {
	int i;

	for (i = 0; i < 3; i++) {
		nukeResource(rtBuffer, i + 1);
		nukeResource(rtBuffer, i + 5);
	}

	if (!getResourceAddress(rtBuffer, 4)) {
		// Since the size of screen 3 is fixed, there is no need to reallocate
		// it if its size changed.
		// Not sure what it is good for, though. I think it may have been used
		// in pre-V7 for the games messages (like 'Pause', Yes/No dialogs,
		// version display, etc.). I don't know about V7, maybe the same is the
		// case there. If so, we could probably just remove it completely.
		initVirtScreen(kUnkVirtScreen, 0, 80, SCREEN_WIDTH, 13, false, false);
	}
	initVirtScreen(kMainVirtScreen, 0, b, SCREEN_WIDTH, h - b, true, true);
	initVirtScreen(kTextVirtScreen, 0, 0, SCREEN_WIDTH, b, false, false);
	initVirtScreen(kVerbVirtScreen, 0, h, SCREEN_WIDTH, SCREEN_HEIGHT - h, false, false);

	_screenB = b;
	_screenH = h;
	_fullRedraw = true;
}

void ScummEngine::initVirtScreen(VirtScreenNumber slot, int number, int top, int width, int height, bool twobufs,
													 bool scrollable) {
	VirtScreen *vs = &virtscr[slot];
	int size;

	assert(height >= 0);
	assert(slot >= 0 && slot < 4);

	vs->number = slot;
	vs->width = width;
	vs->topline = top;
	vs->height = height;
	vs->hasTwoBuffers = twobufs;
	vs->xstart = 0;
	vs->backBuf = NULL;
	vs->dirty.clear();

	size = (vs->width * vs->height) >> 1;
	if (scrollable) {
		// Allow enough spaces so that rooms can be up to 4 resp. 8 screens
		// wide. To achieve (horizontal!) scrolling, we use a neat trick:
		// only the offset into the screen buffer (xstart) is changed. That way
		// very little of the screen has to be redrawn, and we have a very low
		// memory overhead (namely for every pixel we want to scroll, we need
		// one additional byte in the buffer).
		size += (width * 4) >> 1;
	}

	createResource(rtBuffer, slot + 1, size);
	vs->screenPtr = getResourceAddress(rtBuffer, slot + 1);
	memset(vs->screenPtr, 0, size);			// reset background

	if (twobufs) {
		vs->backBuf = createResource(rtBuffer, slot + 5, size);
	}

	if (slot != 3) {
		vs->setDirtyRange(0, height);
	}
}

VirtScreen *ScummEngine::findVirtScreen(int y) {
	VirtScreen *vs = virtscr;
	int i;

	for (i = 0; i < 3; i++, vs++) {
		if (y >= vs->topline && y < vs->topline + vs->height) {
			return vs;
		}
	}
	return NULL;
}

void ScummEngine::markRectAsDirty(VirtScreenNumber virt, int left, int right, int top, int bottom, int dirtybit) {
	VirtScreen *vs = &virtscr[virt];
	int lp, rp;

	//if (left >= right || top >= bottom)
	if (left > right || top > bottom)
		return;
	if (top > vs->height || bottom < 0)
	//if (top >= vs->height || bottom <= 0)
		return;

	if (top < 0)
		top = 0;
	if (bottom > vs->height)
		bottom = vs->height;
	if (virt == kMainVirtScreen && dirtybit)
	{
		lp = left / 8 + _screenStartStrip;
		if (lp < 0)
			lp = 0;
		rp = right / 8 + _screenStartStrip;
		if (rp > 199)
			rp = 199;
		for (; lp <= rp; lp++)
			setGfxUsageBit(lp, dirtybit);
	}

	// The following code used to be in the separate method setVirtscreenDirty
	lp = SCREEN_TO_STRIP(left);
	rp = SCREEN_TO_STRIP(right);

	//if ((lp >= SCREEN_STRIP_COUNT) || (rp <= 0))
	if ((lp >= SCREEN_STRIP_COUNT) || (rp < 0))
		return;
	if (lp < 0)
		lp = 0;
	if (rp >= SCREEN_STRIP_COUNT)
		rp = SCREEN_STRIP_COUNT - 1;

	while (lp <= rp) {
		vs->updateDirty(lp, top, bottom);
		lp++;
	}
}

/**
 * Update all dirty screen areas. This method blits all of the internal engine
 * graphics to the actual display, as needed. In addition, the 'shaking'
 * code in the backend is controlled from here.
 */
void ScummEngine::drawDirtyScreenParts() {
#ifdef GAME_SAMNMAX
	if(_hack_samnmax_intro)
		return;
#endif

	// Update verbs
	updateDirtyScreen(kVerbVirtScreen);
	
	// Update the conversation area (at the top of the screen)
//	updateDirtyScreen(kTextVirtScreen);

	// Update game area ("stage")
	updateDirtyScreen(kMainVirtScreen);
	/*
#ifdef DEBUG_REDRAWSTRIP
	if (1)
#else
	if (_fullRedraw || (camera._last.x != camera._cur.x))
#endif
	{
		// Camera moved: redraw everything
		VirtScreen *vs = &virtscr[kMainVirtScreen];
		byte* src = vs->screenPtr + (vs->xstart >> 1);
		_system->copy_screen(src, vs->topline, vs->height, 0);
		vs->setDirtyRange(vs->height, 0);
	} else {
		updateDirtyScreen(kMainVirtScreen);
	}
	*/
	// Handle shaking
	if (_shakeEnabled) {
		_shakeFrame = (_shakeFrame + 1) & (NUM_SHAKE_POSITIONS - 1);
		_system->set_shake_pos(shake_positions[_shakeFrame]);
	} else if (!_shakeEnabled &&_shakeFrame != 0) {
		_shakeFrame = 0;
		_system->set_shake_pos(shake_positions[_shakeFrame]);
	}
}

void ScummEngine::updateDirtyScreen(VirtScreenNumber slot)
{
#ifdef DEBUG_DIRTY_REGIONS
	static byte c = 0;
	if (slot == kMainVirtScreen)
	{
		VirtScreen *vs = &virtscr[kMainVirtScreen];

		int16 w = 1;
		int16 start = 0;
		uint16 dirty_restore = vs->height << 8;
		byte* dirt = (byte*)vs->dirty;
		for (int16 i = 0; i < SCREEN_STRIP_COUNT; i++)
		{
			byte top = *dirt; *dirt++ = vs->height;
			byte bottom = *dirt; *dirt++ = 0;
			if (bottom)
			{
				if (i != (SCREEN_STRIP_COUNT - 1)) {
					if (*(dirt+0) == top && *(dirt+1) == bottom) {
						w++;
						continue;
					}
				}
				if ((bottom > top) && (top < vs->height))
				{
					if (bottom > vs->height)
						bottom = vs->height;
					int16 x = STRIP_TO_SCREEN(start);
					int16 x1 = start << 3;
					int16 x2 = x1 + ((w << 3) - 1);
					int16 y1 = top;
					int16 y2 = bottom - 1;
					drawBox(x1, y1, x2, y2, c++);
				}
				w = 1;
			}
			start = i + 1;
		}

		byte* src = vs->screenPtr + (vs->xstart >> 1);
		_system->copy_scroll(src, camera._last.x - camera._cur.x, vs->topline, vs->height, vs->dirty);
		vs->setDirtyRange(vs->height, 0);
	}

#else
	gdi.updateDirtyScreen(&virtscr[slot]);
#endif
}

/**
 * Blit the dirty data from the given VirtScreen to the display. If the camera moved,
 * a full blit is done, otherwise only the visible dirty areas are updated.
 */
void Gdi::updateDirtyScreen(VirtScreen *vs) {
	// Do nothing for unused virtual screens
	if (vs->height == 0)
		return;

	uint16 y = vs->topline - _vm->_screenTop;
	uint16 h = vs->height;
	unsigned char* ptr = vs->screenPtr + (vs->xstart >> 1);

	if (/*_vm->_fullRedraw ||*/ ((vs->number == kMainVirtScreen) && (_vm->camera._last.x != _vm->camera._cur.x))) {
		_vm->_system->copy_screen(ptr, y, h, 0);
	}else {
		_vm->_system->copy_screen(ptr, y, h, vs->dirty.maskBuf());
	}
	vs->dirty.clear();
}


#pragma mark -
#pragma mark --- Background buffers & charset mask ---
#pragma mark -


void ScummEngine::initBGBuffers(int height) {
	const byte *ptr;
	int size, itemsize, i;
	byte *room;

	room = getResourceAddress(rtRoom, _roomResource);
	ptr = findResource(MKID('RMIH'), findResource(MKID('RMIM'), room));
	gdi._numZBuffer = READ_LE_UINT16(ptr + 8) + 1;
	assert(gdi._numZBuffer >= 1 && gdi._numZBuffer <= 8);
	itemsize = MUL_SCREEN_STRIPS(_roomHeight + 4);

	size = itemsize * gdi._numZBuffer;
	memset(createResource(rtBuffer, 9, size), 0, size);

	for (i = 0; i < (int)ARRAYSIZE(gdi._imgBufOffs); i++) {
		if (i < gdi._numZBuffer)
			gdi._imgBufOffs[i] = i * itemsize;
		else
			gdi._imgBufOffs[i] = (gdi._numZBuffer - 1) * itemsize;
	}
}

/**
 * Redraw background as needed, i.e. the left/right sides if scrolling took place etc.
 * Note that this only updated the virtual screen, not the actual display.
 */
void ScummEngine::redrawBGAreas() {
	int i;
	int val;

	if (camera._cur.x != camera._last.x && _charset->_hasMask)
		stopTalk();

	val = 0;

	// Redraw parts of the background which are marked as dirty.
	VirtScreen *vs = &virtscr[0];
	if (!_fullRedraw && _BgNeedsRedraw)
	{
		for (i = 0; i != SCREEN_STRIP_COUNT; i++)
		{
			if (testGfxUsageBit(_screenStartStrip + i, USAGE_BIT_DIRTY))
			{
				redrawBGStrip(i, 1, 0, vs->height);
				/*
				byte* dirt = (byte*)&vs->dirty[i];
				uint16 y = *dirt++;
				uint16 h = *dirt++;
				if (y < vs->height && h > y)
					redrawBGStrip(i, 1, y, h - y);
				else
					redrawBGStrip(i, 1, 0, vs->height);
				*/
			}
		}
	}

	int16 ccx = camera._cur.x >> 3;
	int16 clx = camera._last.x >> 3;

	if (_fullRedraw == 0 && ((ccx - clx) == 1)) {
		val = 2;
		redrawBGStrip(SCREEN_STRIP_COUNT - 1, 1, 0, vs->height);
	} else if (_fullRedraw == 0 && ((ccx - clx) == -1)) {
		val = 1;
		redrawBGStrip(0, 1, 0, vs->height);
	} else if (_fullRedraw != 0 || (ccx != clx)) {
		_BgNeedsRedraw = false;
		_flashlight.isDrawn = false;
		redrawBGStrip(0, SCREEN_STRIP_COUNT, 0, vs->height);
	}

	drawRoomObjects(val);
	_BgNeedsRedraw = false;
}

void ScummEngine::redrawBGStrip(int start, int num, int16 y, int16 h) {
	int s = _screenStartStrip + start;

	VirtScreen *vs = &virtscr[0];
	if (y >= vs->height)
		return;

	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (y + h > vs->height)
		h = vs->height - y;
	if (h <= 0)
		return;

	assert(s >= 0 && (size_t) s < sizeof(gfxUsageBits) / (sizeof(gfxUsageBits[0])));

	for (int i = 0; i < num; i++)
		setGfxUsageBit(s + i, USAGE_BIT_DIRTY);
#ifdef DEBUG_REDRAWSTRIP
	_debugStripColor = (_debugStripColor + 1) & 15;
	_debugRedrawBitmap = true;
#endif



	gdi.drawBitmap(getResourceAddress(rtRoom, _roomResource) + _IM00_offs, vs, s, y, _roomWidth, h, s, num, y, 0);

#ifdef DEBUG_REDRAWSTRIP
	_debugRedrawBitmap = false;
#endif
}



void ScummEngine::restoreBG(Common::Rect rect, byte backColor) {
	VirtScreen *vs;
	byte *backbuff;

	if (rect.top < 0)
		rect.top = 0;
	if (rect.left >= rect.right || rect.top >= rect.bottom)
		return;

	if ((vs = findVirtScreen(rect.top)) == NULL)
		return;

	if (rect.left > vs->width)
		return;

	const int topline = vs->topline;

	// Move rect up
	rect.top -= topline;
	rect.bottom -= topline;

	// snap to 8pixel strips
	rect.left &= ~7;
	rect.right = (rect.right + 7) & ~7;

	rect.clip(vs->width, vs->height);
	markRectAsDirty(vs->number, rect, USAGE_BIT_RESTORED);

	uint32 offset = MAKE_SCREENPTR(0, vs->width, vs->xstart + rect.left, rect.top);
	backbuff = vs->screenPtr + offset;

	int height = rect.height();
	int width = rect.width();

	if (vs->hasTwoBuffers && _currentRoom != 0 && isLightOn())
	{
		blit(backbuff, vs->backBuf + offset, width, height);
		if (vs->number == kMainVirtScreen && _charset->_hasMask && height)
		{
			byte *mask;

			// Move rect back
			rect.top += topline;
			rect.bottom += topline;

			// Note: At first sight it may look as if this could
			// be optimized to (rect.right - rect.left) / 8 and
			// thus to width / 8, but that's not the case since
			// we are dealing with integer math here.
			int mask_width = (rect.right / 8) - (rect.left / 8);

			if (rect.right & 0x07)
				mask_width++;

			mask = getMaskBuffer(rect.left, rect.top + topline, 0);
			do {
				memset(mask, 0, mask_width);
				mask += SCREEN_STRIP_COUNT;
			} while (--height);			
		}
	}
	else
	{
		fillStrips(backbuff, backColor, (width >> 3), height);
	}
}

void ScummEngine::restoreCharsetBG(Common::Rect rect)
{
#if 1
	restoreBG(rect);
#else

	// from v1.2.0 - when we move text to it's own buffer
	// Restore background on the whole text area. This code is based on
	// restoreBackground(), but was changed to only restore those parts which are
	// currently covered by the charset mask.
	VirtScreen *vs;
	if ((vs = findVirtScreen(rect.top)) == NULL)
		return;
	if (!vs->height)
		return;

	int16 w = vs->width;
	int16 h = vs->height;
	markRectAsDirty(vs->number, Common::Rect(w, h), USAGE_BIT_RESTORED);

	uint32 offset = (vs->xstart >> 1) - MUL160(vs->topline);
	byte* screenBuf = vs->screenPtr + offset;

	if (vs->hasTwoBuffers && _currentRoom != 0 && isLightOn()) {
		if (vs->number != kMainVirtScreen) {
			// Restore from back buffer
			const byte* backBuf = vs->backBuf + offset;
			blit(screenBuf, backBuf, w, h);
		}
	} else {
		// Clear area
		memset(screenBuf, 0, (w * h) >> 1);
	}

	if (vs->hasTwoBuffers) {
		// Clean out the charset mask
		//clearTextSurface();
	}
#endif
}

void CharsetRenderer::restoreCharsetBg() {
	if (_hasMask)
	{
		_vm->restoreCharsetBG(_mask);
		_hasMask = false;
		_mask.top = _mask.left = 32767;
		_mask.right = _mask.bottom = 0;
		_str.left = -1;
		_left = -1;
	}
	_nextLeft = _vm->_string[0].xpos;
	_nextTop = _vm->_string[0].ypos;
}

void CharsetRenderer::clearCharsetMask() {
	memset(_vm->getResourceAddress(rtBuffer, 9), 0, _vm->gdi._imgBufOffs[1]);
	_mask.top = _mask.left = 32767;
	_mask.right = _mask.bottom = 0;
}

bool CharsetRenderer::hasCharsetMask(int left, int top, int right, int bottom) {
	Common::Rect rect(left, top, right, bottom);
	
	return _hasMask && rect.intersects(_mask);
}

byte *ScummEngine::getMaskBuffer(int x, int y, int z) {
	return getResourceAddress(rtBuffer, 9)
			+ _screenStartStrip + SCREEN_TO_STRIP(x) + MUL_SCREEN_STRIPS(y) + gdi._imgBufOffs[z];
}

byte *Gdi::getMaskBuffer(int x, int y, int z) {
	return _vm->getResourceAddress(rtBuffer, 9)
			+ x + MUL_SCREEN_STRIPS(y) + _imgBufOffs[z];
}


bool ScummEngine::hasMask(int l, int t, int r, int b, byte* ptr, int zbuf)
{
	if (_charset->hasCharsetMask(l, t, r, b))
		return true;

	if (zbuf == 0)
		return false;

 	l = (SCREEN_TO_STRIP(l) + _screenStartStrip) & ~1;
	uint16* basePtr = (uint16*) (getResourceAddress(rtBuffer, 9) + l + MUL_SCREEN_STRIPS(t) + gdi._imgBufOffs[0]);
	if (!basePtr)
		return false;

	r = SCREEN_TO_STRIP(r) + _screenStartStrip;
	int width = ((r - l) >> 1);
	int height = b - t + 1;

	// skipping check on every second row
	int h = height >> 1;
	uint16* p = basePtr + (gdi._imgBufOffs[zbuf] >> 1);
	do {
		for (int i = 0; i <= width; i++)
			if (p[i]) {
				return true;
			}
		p += 40;
	} while (--h);

	return false;
}


#pragma mark -
#pragma mark --- Misc ---
#pragma mark -

void ScummEngine::blit(byte *dst, const byte *src, int w, int h) {
	assert(h > 0);
	assert(src != NULL);
	assert(dst != NULL);
	
	// TODO: This function currently always assumes that srcPitch == dstPitch
	// and furthermore that both equal SCREEN_WIDTH.

	if (w==SCREEN_WIDTH)
		memcpy (dst, src, (w*h)>>1);
	else
	{
		do {
			memcpy(dst, src, (w>>1));
			dst += (SCREEN_WIDTH>>1);
			src += (SCREEN_WIDTH>>1);
		} while (--h);
	}
}


static uint32 _drawBoxMask[8 * 8] = {
	0x80808080, 0xC0C0C0C0, 0xE0E0E0E0, 0xF0F0F0F0, 0xF8F8F8F8, 0xFCFCFCFC, 0xFEFEFEFE, 0xFFFFFFFF, // xbit 0
	0x40404040, 0x60606060, 0x70707070, 0x78787878, 0x7C7C7C7C, 0x7E7E7E7E, 0x7F7F7F7F, 0x7F7F7F7F, // xbit 1
	0x20202020, 0x30303030, 0x38383838, 0x3C3C3C3C, 0x3E3E3E3E, 0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F, // xbit 2
	0x10101010, 0x18181818, 0x1C1C1C1C, 0x1E1E1E1E, 0x1F1F1F1F, 0x1F1F1F1F, 0x1F1F1F1F, 0x1F1F1F1F, // xbit 3
	0x08080808, 0x0C0C0C0C, 0x0E0E0E0E, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, // xbit 4
	0x04040404, 0x06060606, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, // xbit 5
	0x02020202, 0x03030303, 0x03030303, 0x03030303, 0x03030303, 0x03030303, 0x03030303, 0x03030303, // xbit 6
	0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, // xbit 7
};


void ScummEngine::drawBox(int x, int y, int x2, int y2, int color) {
	VirtScreen *vs;

	if ((vs = findVirtScreen(y)) == NULL)
		return;

	if (x > x2)
		SWAP(x, x2);

	if (y > y2)
		SWAP(y, y2);

	x2++;
	y2++;

	// Adjust for the topline of the VirtScreen
	y -= vs->topline;
	y2 -= vs->topline;

	// Clip the coordinates
	if (x < 0)
		x = 0;
	else if (x >= vs->width)
		return;

	if (x2 < 0)
		return;
	else if (x2 > vs->width)
		x2 = vs->width;

	if (y < 0)
		y = 0;
	else if (y > vs->height)
		return;

	if (y2 < 0)
		return;
	else if (y2 > vs->height)
		y2 = vs->height;

#ifndef DEBUG_DIRTY_REGIONS
	markRectAsDirty(vs->number, x, x2, y, y2);
#endif

	if (color == -1)
	{
		// snap to 8 pixel grid
		x = x & ~7;
		x2 = (x2 + 7) & ~7;
		int width = x2 - x;
		int height = y2 - y;
		if (vs->number != kMainVirtScreen)
			error("can only copy bg to main window");

		byte* backbuff = MAKE_SCREENPTR(vs->screenPtr, vs->width, vs->xstart + x, y);
		byte* bgbuff = MAKE_SCREENPTR(vs->backBuf, vs->width, vs->xstart + x, y);
		blit(backbuff, bgbuff, width, height);
	} else {

		// left
		uint16 len = x2 - x;
		uint16 height = y2 - y;
		byte xbit = x & 7;
		byte bitcount = 8 - xbit;
		byte xlen = len > bitcount ? bitcount : len;
		len -= xlen;
		uint32 mask = _drawBoxMask[(xbit << 3) | (xlen - 1)];
		uint32 c = c2pfill256[color];

		uint32* dstart = (uint32*) vs->screenPtr;
		dstart += MUL40(y) + ((vs->xstart + x) >> 3);
		register uint32* d = dstart;
		register uint32 c32 = c & mask;
		register uint32 mask32 = ~mask;
		register int16 h = height;
		while (h > 7) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d[4*40] = (d[4*40] & mask32) | c32; d[5*40] = (d[5*40] & mask32) | c32; d[6*40] = (d[6*40] & mask32) | c32; d[7*40] = (d[7*40] & mask32) | c32; d += (40 * 8); h-= 8; }		
		while (h > 3) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d += (40 * 4); h-= 4; }		
		while (h != 0)	{ *d = ((*d & mask32) | c32); d += 40; h--; }

		// middle 
		c32 = c;
		while (len > 7)
		{
			dstart++;
			d = dstart;
			h = height;
			while (h > 7) 	{ d[0*40] = c32; d[1*40] = c32; d[2*40] = c32; d[3*40] = c32; d[4*40] = c32; d[5*40] = c32; d[6*40] = c32; d[7*40] = c32; d += (40 * 8); h-= 8; }		
			while (h > 3) 	{ d[0*40] = c32; d[1*40] = c32; d[2*40] = c32; d[3*40] = c32; d += (40 * 4); h-= 4; }		
			while(h != 0) { *d = c32; d+= 40; h--; }
			len-=8;
		}

		// right
		if (len != 0)
		{
			mask = _drawBoxMask[len-1];
			c32 = c & mask;
			mask32 = ~mask;
			dstart++;
			d = dstart;
			h = height;
			while (h > 7) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d[4*40] = (d[4*40] & mask32) | c32; d[5*40] = (d[5*40] & mask32) | c32; d[6*40] = (d[6*40] & mask32) | c32; d[7*40] = (d[7*40] & mask32) | c32; d += (40 * 8); h-= 8; }		
			while (h > 3) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d += (40 * 4); h-= 4; }		
			while (h != 0)	{ *d = (*d & mask32) | c32; d += 40; h--; }
		}

		//fillStrips(backbuff, getSystemPal(color), (width >> 3), height);
	}

}

void ScummEngine::drawFlashlight() {
	// TODO_ATARI: rewrite for planar (do we need this at all?)

/*
	int i, j, offset, x, y;
	VirtScreen *vs = &virtscr[kMainVirtScreen];

	// Remove the flash light first if it was previously drawn
	if (_flashlight.isDrawn) {
		markRectAsDirty(kMainVirtScreen, _flashlight.x, _flashlight.x + _flashlight.w,
										_flashlight.y, _flashlight.y + _flashlight.h, USAGE_BIT_DIRTY);
		
		if (_flashlight.buffer) {
			i = _flashlight.h;
			do {
				memset(_flashlight.buffer, 0, _flashlight.w);
				_flashlight.buffer += vs->width;
			} while (--i);
		}
		_flashlight.isDrawn = false;
	}

	if (_flashlight.xStrips == 0 || _flashlight.yStrips == 0)
		return;

	// Calculate the area of the flashlight
	Actor *a = derefActor(VAR(VAR_EGO), "drawFlashlight");
	x = a->_pos.x;
	y = a->_pos.y;

	_flashlight.w = _flashlight.xStrips * 8;
	_flashlight.h = _flashlight.yStrips * 8;
	_flashlight.x = x - (_flashlight.w / 2) - STRIP_TO_SCREEN(_screenStartStrip);
	_flashlight.y = y - (_flashlight.h / 2);

	// Clip the flashlight at the borders
	if (_flashlight.x < 0)
		_flashlight.x = 0;
	else if (_flashlight.x + _flashlight.w > SCREEN_WIDTH)
		_flashlight.x = SCREEN_WIDTH - _flashlight.w;
	if (_flashlight.y < 0)
		_flashlight.y = 0;
	else if (_flashlight.y + _flashlight.h > vs->height)
		_flashlight.y = vs->height - _flashlight.h;

	// Redraw any actors "under" the flashlight
	for (i = SCREEN_TO_STRIP(_flashlight.x); i < SCREEN_TO_STRIP((_flashlight.x + _flashlight.w)); i++) {
		assert(0 <= i && i < SCREEN_STRIP_COUNT);
		setGfxUsageBit(_screenStartStrip + i, USAGE_BIT_DIRTY);
		vs->setDirty(i, 0, vs->height);
	}

	byte *bgbak;
	offset = _flashlight.y * vs->width + vs->xstart + _flashlight.x;
	_flashlight.buffer = vs->screenPtr + offset;
	bgbak = vs->backBuf + offset;

	blit(_flashlight.buffer, bgbak, _flashlight.w, _flashlight.h);

	// Round the corners. To do so, we simply hard-code a set of nicely
	// rounded corners.
	int corner_data[] = { 8, 6, 4, 3, 2, 2, 1, 1 };
	int minrow = 0;
	int maxcol = _flashlight.w - 1;
	int maxrow = (_flashlight.h - 1) * vs->width;

	for (i = 0; i < 8; i++, minrow += vs->width, maxrow -= vs->width) {
		int d = corner_data[i];

		for (j = 0; j < d; j++) {
			_flashlight.buffer[minrow + j] = 0;
			_flashlight.buffer[minrow + maxcol - j] = 0;
			_flashlight.buffer[maxrow + j] = 0;
			_flashlight.buffer[maxrow + maxcol - j] = 0;
		}
	}
	
	_flashlight.isDrawn = true;
*/
}

bool ScummEngine::isLightOn() const {
	return (VAR_CURRENT_LIGHTS == 0xFF) || (VAR(VAR_CURRENT_LIGHTS) & LIGHTMODE_screen);
}

#pragma mark -
#pragma mark --- Image drawing ---
#pragma mark -




/**
 * Draw a bitmap onto a virtual screen. This is main drawing method for room backgrounds
 * and objects, used throughout all SCUMM versions.
 */
void Gdi::drawBitmap(const byte *ptr, VirtScreen *vs, int16 x, int16 y, const int16 width, const int16 height, int16 stripnr, int16 numstrip, int16 yoffs, byte flag) {
	assert(ptr);
	assert(height > 0);
	byte *backbuff_ptr, *bgbak_ptr;
	const byte *smap_ptr;
	const byte *z_plane_ptr;
	byte *mask_ptr;

	int i;
	const byte *zplane_list[9];

	int bottom;
	int numzbuf;
	int sx;
	bool lightsOn;
	bool useOrDecompress = false;

	debug(2, "drawbitmap %x: %d,%d (%d,%d) s=%d,c=%d,f=%02x", ptr, x,y,width,height, stripnr, numstrip, flag);

	// Check whether lights are turned on or not
	lightsOn = _vm->isLightOn();

	CHECK_HEAP;
	smap_ptr = findResource(MKID('SMAP'), ptr);

	assert(smap_ptr);

	zplane_list[0] = smap_ptr;

	if (_zbufferDisabled)
		numzbuf = 0;
	else if (_numZBuffer <= 1)
		numzbuf = _numZBuffer;
	else {
		numzbuf = _numZBuffer;
		assert(numzbuf <= ARRAYSIZE(zplane_list));
		
		const uint32 zplane_tags[] = {
			MKID('ZP00'),
			MKID('ZP01'),
			MKID('ZP02'),
			MKID('ZP03'),
			MKID('ZP04')
		};
		
		for (i = 1; i < numzbuf; i++) {
			zplane_list[i] = findResource(zplane_tags[i], ptr);
		}
	}
	
	bottom = y + height;
	if (bottom > vs->height) {
		warning("Gdi::drawBitmap, strip drawn to %d below window bottom %d", bottom, vs->height);
	}

	_horizStripNextInc = SCREEN_WIDTH >> 1;
	int y160 = MUL160(y);
	sx = x - SCREEN_TO_STRIP(vs->xstart);

	while (numstrip--) {
		CHECK_HEAP;

		if (sx < 0)
			goto next_iter;

		if (sx >= SCREEN_STRIP_COUNT)
			return;

		vs->updateDirty(sx, y, bottom);

		backbuff_ptr = vs->screenPtr + y160 + (x << 2);
		if (vs->hasTwoBuffers) {
			bgbak_ptr = vs->backBuf + y160 + (x << 2);
		} else {
			bgbak_ptr = backbuff_ptr;
		}

		useOrDecompress = decompressBitmap(bgbak_ptr, smap_ptr + READ_LE_UINT32(smap_ptr + stripnr * 4 + 8), height, yoffs);
		mask_ptr = getMaskBuffer(x, y);

		debug(2, " strip %d,%d,%d : %02x", sx, y, bottom, flag);

		CHECK_HEAP;
		if (vs->hasTwoBuffers) {
			if (_vm->_charset->hasCharsetMask(sx * 8, y, (sx + 1) * 8, bottom)) {
				if (flag & dbClear || !lightsOn)
					clear8ColWithMasking(backbuff_ptr, height, mask_ptr);
				else
					draw8ColWithMasking(backbuff_ptr, bgbak_ptr, height, mask_ptr);
			} else {
				if (flag & dbClear || !lightsOn)
					clear8Col(backbuff_ptr, height);
				else
					draw8Col(backbuff_ptr, bgbak_ptr, height);
			}
		}
		CHECK_HEAP;

		if (flag & dbDrawMaskOnAll)
		{
			// Sam & Max uses dbDrawMaskOnAll for things like the inventory
			// box and the speech icons. While these objects only have one
			// mask, it should be applied to all the Z-planes in the room,
			// i.e. they should mask every actor.
			//
			// This flag used to be called dbDrawMaskOnBoth, and all it
			// would do was to mask Z-plane 0. (Z-plane 1 would also be
			// masked, because what is now the else-clause used to be run
			// always.) While this seems to be the only way there is to
			// mask Z-plane 0, this wasn't good enough since actors in
			// Z-planes >= 2 would not be masked.
			//
			// The flag is also used by The Dig and Full Throttle, but I
			// don't know what for. At the time of writing, these games
			// are still too unstable for me to investigate.

			debug(2,"dbDrawMaskOnAll");
			z_plane_ptr = zplane_list[1] + READ_LE_UINT16(zplane_list[1] + stripnr * 2 + 8);
			for (i = 0; i < numzbuf; i++) {
				mask_ptr = getMaskBuffer(x, y, i);
				if (useOrDecompress && (flag & dbAllowMaskOr))
					decompressMaskImgOr(mask_ptr, z_plane_ptr, height, yoffs);
				else
					decompressMaskImg(mask_ptr, z_plane_ptr, height, yoffs);
			}
		} else {
			for (i = 1; i < numzbuf; i++) {
				uint32 offs;

				if (!zplane_list[i])
					continue;

				offs = READ_LE_UINT16(zplane_list[i] + stripnr * 2 + 8);
				mask_ptr = getMaskBuffer(x, y, i);

				if (offs) {
					z_plane_ptr = zplane_list[i] + offs;

					if (useOrDecompress && (flag & dbAllowMaskOr)) {
						decompressMaskImgOr(mask_ptr, z_plane_ptr, height, yoffs);
					} else {
						decompressMaskImg(mask_ptr, z_plane_ptr, height, yoffs);
					}

				} else {
					if (!(useOrDecompress && (flag & dbAllowMaskOr)))
						for (int h = MUL_SCREEN_STRIPS(height-1); h != 0; h -= SCREEN_STRIP_COUNT)
							mask_ptr[h] = 0;
				}
			}
		}

#ifdef DEBUG_MASKS
		// HACK: blit mask(s) onto normal screen. Useful to debug masking 
		for (i = 0; i < numzbuf; i++)
		{
			uint32 c = c2pfill16[_vm->getSystemPal(12 + i)];

			mask_ptr = getMaskBuffer(x, y, i);
			uint32 *dst = (uint32*)backbuff_ptr;
			uint32 *dst2 = (uint32*)bgbak_ptr;
			for (int h = 0; h < height; h++)
			{
				uint32 m = rptTable[*mask_ptr];
				*dst |= m;
				*dst2 |= m;
				dst += vs->width >> 3;
				dst2 += vs->width >> 3;
				mask_ptr += SCREEN_STRIP_COUNT;
			}
		}
#endif

next_iter:
		CHECK_HEAP;
		x++;
		sx++;
		stripnr++;
	}
}

/**
 * Reset the background behind an actor or blast object.
 */
void Gdi::resetBackground(int top, int bottom, int strip) {
	assert(0 <= strip && strip < SCREEN_STRIP_COUNT);

	int16 y = (int16) top;
	int16 y2 = (int16) bottom;
	int16 height = y2 - y;
	if (height <= 0)
		return;

	int16 xtile = (int16) strip;
	VirtScreen *vs = &_vm->virtscr[0];
	vs->updateDirty(xtile, y, y2);

	uint32 offs = MUL160(y) + (((vs->xstart>>3) + xtile) << 2);
	byte* dst = (byte*) vs->screenPtr + offs;

	if (_vm->isLightOn())
	{
		const byte* src = vs->backBuf + offs;
		
		int16 rx = (strip<<3);
		const Common::Rect& textRect = _vm->_charset->_mask;
		if (_vm->_charset->_hasMask && (rx < textRect.right) && (textRect.left < (rx + 8)) && (y < textRect.bottom) && (textRect.top < y2))
		{
			if (y < textRect.top)
			{
				int16 b = textRect.top;
				if (b > y2) b = y2;
				int16 h = b - y;
				copyStrip(dst, src, SCREEN_WIDTH>>3, h);
				y += h; if (y >= y2) return;
				dst += MUL160(h);
				src += MUL160(h);
			}

			if (y < textRect.bottom)
			{
				int16 b = textRect.bottom;
				if (b > y2) b = y2;
				int16 h = b - y;

				uint32 m;
				uint32* dptr = (uint32*) dst;
				const byte* mptr = _vm->getMaskBuffer(rx, y, 0);
				const uint32* sptr = (const uint32*) src;

				y += h;
				while (h > 7) {
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40;
					h -= 8;
				}
				while (h) {
					m = rptTable[*mptr]; *dptr = (*dptr & m) | (*sptr & ~m); sptr += 40; dptr += 40; mptr += 40; h--;
				}
				if (y >= y2) return;
				dst = (byte*) dptr;
				src = (const byte*) sptr;
			}
			
			copyStrip(dst, src, SCREEN_WIDTH>>3, y2 - y);
		}
		else
		{
			copyStrip(dst, src, SCREEN_WIDTH>>3, height);
		}
	}
	else
	{
		fillStrip(dst, 0, height);
	}
}

void Gdi::draw8ColWithMasking(byte *dst, const byte *src, int height, byte *mask) {
	debug(2,"  d8cm");
	uint32* dptr = (uint32*)dst;
	const byte* mptr = (byte*)mask;
	int16 count = height;
#ifndef DEBUG_REDRAWSTRIP
	const uint32* sptr = (uint32*)src;
	while (count)
	{
		uint32 m = *mptr;
		if (m) {
			m = rptTable[m];
			*dptr = (*dptr & m) | (*sptr & ~m);
		} else {
			*dptr = *sptr;
		}
		sptr += 40; dptr += 40; mptr += 40; count--;
	}
#else
	uint32 px = c2pfill256[_debugStripColor & 15];
	while (count)
	{
		uint32 m = *mptr;
		if (m) { m = rptTable[m]; *dptr = (*dptr & m) | (px & ~m); }
		else { *dptr = px; }
		dptr += 40; mptr += 40; count--;
	}
#endif
}

void Gdi::clear8ColWithMasking(byte *dst, int height, byte *mask) {
	debug(2,"  c8cm");
	uint32* dptr = (uint32*)dst;
	const byte* mptr = (byte*)mask;
	int16 count = height;
	while (count)
	{
		uint32 m = *mptr;
		if (m) {
			*dptr &= rptTable[m];
		} else {
			*dptr = 0;
		}
		dptr += 40; mptr += 40; count--;
	}
}

void Gdi::draw8Col(byte *dst, const byte *src, int height) {
	debug(2,"  d8c");
#ifndef DEBUG_REDRAWSTRIP
	copyStrip(dst, (byte*)src, SCREEN_WIDTH>>3, height);
#else
	uint32 c = (_debugStripColor & 15);
	fillStrip(dst, c, height);
#endif
}

void Gdi::clear8Col(byte *dst, int height)
{
	debug(2,"  c8c");
	fillStrip(dst, 0, height);
}

void Gdi::decompressMaskImg(byte *dst, const byte *src, int16 height, int16 yoffs) {
	byte c; byte b = *src++;

	// skip yoffs
	while (yoffs)
	{
		byte count = b & 0x7F;
		if (count > yoffs) {
			b = (b & 0x80) | (count - yoffs);
			if ((b & 0x80) == 0)
				src += (count - yoffs);
			break;
		}
		yoffs -= count;
		if (b & 0x80) {
			src += 1;
		} else {
			src += count;
		}
		b = *src++;
	}

	// draw
	while (height) {
		if (b & 0x80)
		{
			b &= 0x7F;
			c = *src++;

			do {
				*dst = c;
				dst += SCREEN_STRIP_COUNT;
				--height;
			} while (--b && height);
		} else {
			do {
				*dst = *src++;
				dst += SCREEN_STRIP_COUNT;
				--height;
			} while (--b && height);
		}
		b = *src++;
	}
}

void Gdi::decompressMaskImgOr(byte *dst, const byte *src, int16 height, int16 yoffs) {
	byte c; byte b = *src++;

	// skip yoffs
	while (yoffs)
	{
		byte count = b & 0x7F;
		if (count > yoffs) {
			b = (b & 0x80) | (count - yoffs);
			if ((b & 0x80) == 0)
				src += (count - yoffs);
			break;
		}
		yoffs -= count;
		if (b & 0x80) {
			src += 1;
		} else {
			src += count;
		}
		b = *src++;
	}

	while (height) {
		if (b & 0x80) {
			b &= 0x7F;
			c = *src++;

			do {
				*dst |= c;
				dst += SCREEN_STRIP_COUNT;
				--height;
			} while (--b && height);
		} else {
			do {
				*dst |= *src++;
				dst += SCREEN_STRIP_COUNT;
				--height;
			} while (--b && height);
		}
		b = *src++;
	}
}

byte Gdi::convertBitmap(byte* dst, const byte* src, int numLinesToProcess) {
	byte code = *src++;
	_decomp_shr = code % 10;
	_decomp_mask = 0xFF >> (8 - _decomp_shr);
	const byte codec = 0xFE;
	const byte codecTransp = 0xFF;

	_horizStripNextInc = 4;

	switch (code) {
	case 1:
		unkDecode7<true>(dst, src, numLinesToProcess);
		return codec;
		break;

	case 14: case 15: case 16: case 17: case 18:
		unkDecodeC<true>(dst, src, numLinesToProcess);
		return codec;
		break;

	case 24: case 25: case 26: case 27: case 28:
		unkDecodeB<true>(dst, src, numLinesToProcess);
		return codec;
		break;

	case 34: case 35: case 36: case 37: case 38:
		unkDecodeC_trans<true>(dst, src, numLinesToProcess);
		return codecTransp;
		break;

	case 44: case 45: case 46: case 47: case 48:
		unkDecodeB_trans<true>(dst, src, numLinesToProcess);
		return codecTransp;
		break;

	case 64: case 65: case 66: case 67: case 68: case 104: case 105: case 106: case 107: case 108:
		unkDecodeA<true>(dst, src, numLinesToProcess);
		return codec;
		break;

	case 84: case 85: case 86: case 87: case 88: case 124: case 125: case 126: case 127: case 128:
		unkDecodeA_trans<true>(dst, src, numLinesToProcess);
		return codecTransp;
		break;
	}

	return 0;
}


bool Gdi::decompressBitmap(byte *bgbak_ptr, const byte *src, int16 numLinesToProcess, int16 startLine) {
	byte code = *src++;
	bool useOrDecompress = false;
	/*
	_decomp_shr = code % 10;
	_decomp_mask = 0xFF >> (8 - _decomp_shr);
	*/

	switch (code)
	{
		/*
	case 1:
		unkDecode7<false>(bgbak_ptr, src, numLinesToProcess);
		break;

	case 14: case 15: case 16: case 17: case 18:
		unkDecodeC<false>(bgbak_ptr, src, numLinesToProcess);
		break;

	case 24: case 25: case 26: case 27: case 28:
		unkDecodeB<false>(bgbak_ptr, src, numLinesToProcess);
		break;

	case 34: case 35: case 36: case 37: case 38:
		useOrDecompress = true;
		unkDecodeC_trans<false>(bgbak_ptr, src, numLinesToProcess);
		break;

	case 44: case 45: case 46: case 47: case 48:
		useOrDecompress = true;
		unkDecodeB_trans<false>(bgbak_ptr, src, numLinesToProcess);
		break;

	case 64: case 65: case 66: case 67: case 68: case 104: case 105: case 106: case 107: case 108:
		unkDecodeA<false>(bgbak_ptr, src, numLinesToProcess);
		break;

	case 84: case 85: case 86: case 87: case 88: case 124: case 125: case 126: case 127: case 128:
		useOrDecompress = true;
		unkDecodeA_trans<false>(bgbak_ptr, src, numLinesToProcess);
		break;
		*/
	case 0xFE:
		{
			// Atari: 32bit per strip (MOVEP ready format)
			assert((((uint32)src) & 1) == 0);
			copyStrip1(bgbak_ptr, (byte*)src + (startLine << 2), numLinesToProcess);
		}
		break;

	case 0xFF:
		{
			// Atari: 32bit per strip (MOVEP ready format). 8bit mask after bitmap data.
			useOrDecompress = true;
			assert((((uint32)src) & 1) == 0);
			const byte* mask = src + (numLinesToProcess << 2);
			copyStripMasked1(bgbak_ptr, (byte*)src + (startLine << 2), (byte*)mask + startLine, numLinesToProcess);
		}
		break;

	default:
		error("Gdi::decompressBitmap: default case %d", code);
		break;
	}
	
	return useOrDecompress;
}


#define READ_BIT (cl--, bit = bits & 1, bits >>= 1, bit)
#define FILL_BITS do {              \
		if (cl <= 8) {              \
			bits |= (*src++ << cl); \
			cl += 8;                \
		}                           \
	} while (0)


#define PXORC2P(_px, _color, _x, _y)	\
	if(preprocess) \
		_px |= c2p[(_color) + ((((uint32)(_x) ^ (uint32)(_y)) & 1)<<11)]; \
	else \
		_px |= c2p[(_color)];


template <bool preprocess> void Gdi::unkDecodeA(byte *dst, const byte *src, int height) {
	byte color = *src++;
	uint bits = *src++;
	byte cl = 8;
	byte bit;
	byte incm, reps;
	uint32* d = (uint32*)dst;
	uint32 dinc = _horizStripNextInc >> 2;

	do {
		int x = 8;
		uint32 px = 0;
		uint32* c2p = c2ptable256;
		do {
			FILL_BITS;
			PXORC2P(px, color, x, height);

		againPos:
			if (!READ_BIT) {
			} else if (!READ_BIT) {
				FILL_BITS;
				color = bits & _decomp_mask;
				bits >>= _decomp_shr;
				cl -= _decomp_shr;
			} else {
				incm = (bits & 7) - 4;
				cl -= 3;
				bits >>= 3;
				if (incm) {
					color += incm;
				} else {
					FILL_BITS;
					reps = bits & 0xFF;
					do {
						c2p += 256;
						if (!--x) {
							*d = px;
							d += dinc;
							px = 0;
							x = 8;
							c2p = c2ptable256;
							if (!--height)
								return;
						}
						PXORC2P(px, color, x, height);
					} while (--reps);
					bits >>= 8;
					bits |= (*src++) << (cl - 8);
					goto againPos;
				}
			}
			c2p += 256;
		} while (--x);
		*d = px;
		d += dinc;
	} while (--height);
}

template <bool preprocess> void Gdi::unkDecodeA_trans(byte *dst, const byte *src, int height) {
	byte color = *src++;
	uint bits = *src++;
	byte cl = 8;
	byte bit;
	byte incm, reps;
	uint32* d = (uint32*)dst;
	uint32 dinc = _horizStripNextInc >> 2;
	byte* dm;
	uint32 mask;
	uint32 maskbit;

	if (preprocess)
		dm = dst + (height << 2);

	do {
		int x = 8;
		uint32 px = 0;
		mask = 0;
		maskbit = 0x80;
		uint32* c2p = c2ptable256;
		do
		{
			FILL_BITS;
			if (color != _transparentColor)
			{
				PXORC2P(px, color, x, height);
				mask |= maskbit;
			}

		againPos:
			if (!READ_BIT) {
			} else if (!READ_BIT) {
				FILL_BITS;
				color = bits & _decomp_mask;
				bits >>= _decomp_shr;
				cl -= _decomp_shr;
			} else {
				incm = (bits & 7) - 4;
				cl -= 3;
				bits >>= 3;
				if (incm) {
					color += incm;
				} else {
					FILL_BITS;
					reps = bits & 0xFF;
					do {
						c2p += 256;
						maskbit >>= 1;
						if (!--x)
						{
							if (preprocess)
							{
								*d = px;
								*dm++ = ~((byte)(mask&0xFF));
							}
							else
							{
								*d = (*d & ~mask) | px;
							}
							mask = 0;
							maskbit = 0x80808080;
							d += dinc;
							px = 0;
							x = 8;
							c2p = c2ptable256;
							if (!--height)
								return;
						}
						if (color != _transparentColor)
						{
							PXORC2P(px, color, x, height);
							mask |= maskbit;
						}
					} while (--reps);
					bits >>= 8;
					bits |= (*src++) << (cl - 8);
					goto againPos;
				}
			}
			c2p += 256;
			maskbit >>= 1;
		} while (--x);

		if (preprocess)
		{
			*dm++ = ~((byte)(mask&0xFF));
			*d = px;
		}
		else
		{
			*d = (*d & ~mask) | px;
		}
		d += dinc;
	} while (--height);
}

template <bool preprocess> void Gdi::unkDecodeB(byte *dst, const byte *src, int height) {
	byte color = *src++;
	uint bits = *src++;
	byte cl = 8;
	byte bit;
	int8 inc = -1;
	uint32* d = (uint32*)dst;
	uint32 dinc = _horizStripNextInc >> 2;

	do {
		int x = 8;
		uint32 px = 0;
		uint32* c2p = c2ptable256;
		do {
			FILL_BITS;
			PXORC2P(px, color, x, height);
			if (!READ_BIT) {
			} else if (!READ_BIT) {
				FILL_BITS;
				color = bits & _decomp_mask;
				bits >>= _decomp_shr;
				cl -= _decomp_shr;
				inc = -1;
			} else if (!READ_BIT) {
				color += inc;
			} else {
				inc = -inc;
				color += inc;
			}
			c2p += 256;
		} while (--x);
		*d = px;
		d += dinc;
	} while (--height);
}

template <bool preprocess> void Gdi::unkDecodeB_trans(byte *dst, const byte *src, int height) {
	byte color = *src++;
	uint bits = *src++;
	byte cl = 8;
	byte bit;
	int8 inc = -1;
	uint32* d = (uint32*)dst;
	uint32 dinc = _horizStripNextInc >> 2;
	byte* dm; 
	uint32 mask, maskbit;

	if (preprocess)
		dm = dst + (height << 2);

	do {
		int x = 8;
		uint32 px = 0;
		mask = 0;
		maskbit = 0x80808080;
		uint32* c2p = c2ptable256;
		do {
			FILL_BITS;
			if (color != _transparentColor)
			{
				PXORC2P(px, color, x, height);
				mask |= maskbit;
			}

			if (!READ_BIT) {
			} else if (!READ_BIT) {
				FILL_BITS;
				color = bits & _decomp_mask;
				bits >>= _decomp_shr;
				cl -= _decomp_shr;
				inc = -1;
			} else if (!READ_BIT) {
				color += inc;
			} else {
				inc = -inc;
				color += inc;
			}
			c2p += 256;
			maskbit >>= 1;
		} while (--x);
		if (preprocess)
		{
			*dm++ = ~((byte)(mask & 0xFF));
			*d = px;
		}
		else
		{
			*d = (*d & ~mask) | px;
		}
		d += dinc;
	} while (--height);
}

template <bool preprocess> void Gdi::unkDecodeC(byte *dst, const byte *src, int height) {
	byte color = *src++;
	uint bits = *src++;
	byte cl = 8;
	byte bit;
	int8 inc = -1;
	byte* dst2 = dst;

	int x = 8;
	do {
		int h = height;
		dst = &_tempDecodeBuffer[8-x];
		do {
			FILL_BITS;
			*dst = color;
			dst += 8;
			if (!READ_BIT) {
			} else if (!READ_BIT) {
				FILL_BITS;
				color = bits & _decomp_mask;
				bits >>= _decomp_shr;
				cl -= _decomp_shr;
				inc = -1;
			} else if (!READ_BIT) {
				color += inc;
			} else {
				inc = -inc;
				color += inc;
			}
		} while (--h);
	} while (--x);

	unkDecode7<preprocess>(dst2, _tempDecodeBuffer, height);
}

template <bool preprocess> void Gdi::unkDecodeC_trans(byte *dst, const byte *src, int height) {
	byte color = *src++;
	uint bits = *src++;
	byte cl = 8;
	byte bit;
	int8 inc = -1;
	byte* dst2 = dst;
	byte* dm;
	byte maskbit = 0x80;
	memset(_tempDecodeMask, 0, height);
	memset(_tempDecodeBuffer, 0, height<<3);

	int x = 8;
	do {
		int h = height;
		dst = &_tempDecodeBuffer[8-x];
		dm = _tempDecodeMask;
		do {
			FILL_BITS;
			if (color != _transparentColor)
			{
				*dst = color;
				*dm |= maskbit;
			}
			dst += 8;
			dm++;
			if (!READ_BIT) {
			} else if (!READ_BIT) {
				FILL_BITS;
				color = bits & _decomp_mask;
				bits >>= _decomp_shr;
				cl -= _decomp_shr;
				inc = -1;
			} else if (!READ_BIT) {
				color += inc;
			} else {
				inc = -inc;
				color += inc;
			}
		} while (--h);
		maskbit >>= 1;
	} while (--x);

	if (preprocess)
	{
		unkDecode7<preprocess>(dst2, _tempDecodeBuffer, height);
		src = _tempDecodeMask;
		dst = dst2 + (height << 2);
		for (int i=0; i<height; i++)
			*dst++ = ~(*src++);
	}
	else
	{
		uint32 px;
		uint32* d = (uint32*)dst2;
		uint32 dinc = _horizStripNextInc >> 2;	
		src = _tempDecodeBuffer;
		dm = _tempDecodeMask;
		do {
			px  = c2ptable256[0x000 + *src++];
			px |= c2ptable256[0x100 + *src++];
			px |= c2ptable256[0x200 + *src++];
			px |= c2ptable256[0x300 + *src++];
			px |= c2ptable256[0x400 + *src++];
			px |= c2ptable256[0x500 + *src++];
			px |= c2ptable256[0x600 + *src++];
			px |= c2ptable256[0x700 + *src++];
			*d = (*d & ~rptTable[*dm++]) | px;
			d += dinc;
		} while (--height);
	}
}

#undef READ_BIT
#undef FILL_BITS


template <bool preprocess> void Gdi::unkDecode7(byte *dst, const byte *src, int height) {
	uint32 px;
	uint32* d = (uint32*)dst;
	uint32 dinc = _horizStripNextInc >> 2;	
	uint32* c2p = c2ptable256;
	do {
		px = 0;
		PXORC2P(px, 0x000 + *src++, src, height);
		PXORC2P(px, 0x100 + *src++, src, height);
		PXORC2P(px, 0x200 + *src++, src, height);
		PXORC2P(px, 0x300 + *src++, src, height);
		PXORC2P(px, 0x400 + *src++, src, height);
		PXORC2P(px, 0x500 + *src++, src, height);
		PXORC2P(px, 0x600 + *src++, src, height);
		PXORC2P(px, 0x700 + *src++, src, height);
		*d = px;
		d += dinc;
	} while (--height);
}


#pragma mark -
#pragma mark --- Transition effects ---
#pragma mark -

void ScummEngine::fadeIn(int effect) {
	VirtScreen *vs = &virtscr[0];	
	updatePalette();

	switch (effect) {
	case 0:
		// seems to do nothing
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		// Some of the transition effects won't work properly unless
		// the screen is marked as clean first. At first I thought I
		// could safely do this every time fadeIn() was called, but
		// that broke the FOA intro. Probably other things as well.
		//
		// Hopefully it's safe to do it at this point, at least.
		vs->setDirtyRange(vs->height, 0);
 		transitionEffect(effect - 1);
		break;
	case 128:
		unkScreenEffect6();
		break;
	case 129:
		break;
	case 130:
	case 131:
	case 132:
	case 133:
		scrollEffect(133 - effect);
		break;
	case 134:
		dissolveEffect(1, 1);
		break;
	case 135:
		unkScreenEffect5(1);
		break;
	default:
		warning("Unknown screen effect, %d", effect);
		break;
	}
	_screenEffectFlag = true;
}

void ScummEngine::fadeOut(int effect) {
	VirtScreen *vs = &virtscr[0];
	vs->setDirtyRange(vs->height, 0);
	camera._last.x = camera._cur.x;

	if (_screenEffectFlag && effect != 0)
	{
		_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, vs->dirty.maskBuf(), false);
		_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, vs->dirty.maskBuf(), false);
		//_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, 0, false);
		//_system->update_screen();

		// Fill screen 0 with black
		memset(vs->screenPtr + (vs->xstart >> 1), 0, (vs->width * vs->height) >> 1);
	
		// Fade to black with the specified effect, if any.
		switch (effect) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			transitionEffect(effect - 1);
			break;
		case 128:
			unkScreenEffect6();
			break;
			/*
		case 129:
			// Just blit screen 0 to the display (i.e. display will be black)
			vs->setDirtyRange(0, vs->height);
			_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, 0, false);
			_system->update_screen();
			vs->dirty.clear();
			break;
			*/
		case 134:
			dissolveEffect(1, 1);
			break;
			/*
		case 135:
			unkScreenEffect5(1);
			break;
			*/
		default:
			warning("fadeOut: default case %d", effect);
			vs->setDirtyRange(0, vs->height);
			_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, 0, false);
			_system->update_screen();
			vs->dirty.clear();
			break;
		}
	}

	// Update the palette at the end (once we faded to black) to avoid
	// some nasty effects when the palette is changed
	updatePalette();

	_screenEffectFlag = false;
}

/**
 * Perform a transition effect. There are four different effects possible:
 * 0: Iris effect
 * 1: Box wipe (a black box expands from the upper-left corner to the lower-right corner)
 * 2: Box wipe (a black box expands from the lower-right corner to the upper-left corner)
 * 3: Inverse box wipe
 * All effects operate on 8x8 blocks of the screen. These blocks are updated
 * in a certain order; the exact order determines how the effect appears to the user.
 * @param a		the transition effect to perform
 */
void ScummEngine::transitionEffect(int a)
{
	int delta[16];								// Offset applied during each iteration
	int tab_2[16];
	int i, j;
	int bottom;
	int l, t, r, b;
	const int height = MIN((int)virtscr[0].height, SCREEN_HEIGHT);

	for (i = 0; i < 16; i++) {
		delta[i] = transitionEffects[a].deltaTable[i];
		j = transitionEffects[a].stripTable[i];
		if (j == 24)
			j = height / 8 - 1;
		tab_2[i] = j;
	}

	// update system screen with nothing, to clear the prev-frame dirty mask for double buffering
	VirtScreen* vs = &virtscr[0];
	vs->dirty.clear();

	bottom = height / 8;
	for (j = 0; j < transitionEffects[a].numOfIterations; j++)
	{
		uint32 t0 = _system->get_msecs();
		for (i = 0; i < 4; i++)
		{
			l = tab_2[i * 4];
			t = tab_2[i * 4 + 1];
			r = tab_2[i * 4 + 2];
			b = tab_2[i * 4 + 3];
			if (t == b) {
				while (l <= r) {
					if (l >= 0 && l < SCREEN_STRIP_COUNT && t < bottom) {
						vs->updateDirty(l, (t * 8), ((b + 1) * 8));
					}
					l++;
				}
			} else {
				if (l < 0 || l >= SCREEN_STRIP_COUNT || b <= t)
					continue;

				if (b > bottom)
					b = bottom;
				if (t < 0)
					t = 0;
				vs->updateDirty(l, (t*8), ((b+1)*8));
			}
		}

		for (i = 0; i < 16; i++)
			tab_2[i] += delta[i];

		// Draw the current state to the screen and wait half a sec so the user can watch the effect taking place.
		_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, vs->dirty.maskBuf(), false);
		_system->update_screen();
		vs->dirty.clear();

		const uint32 delay = 45;
		uint32 t = _system->get_msecs() - t0;
		if (t < delay) {
			waitForTimer(delay - t);
		}
	}
}

/**
 * Update width*height areas of the screen, in random order, until the whole
 * screen has been updated. For instance:
 * 
 * dissolveEffect(1, 1) produces a pixel-by-pixel dissolve
 * dissolveEffect(8, 8) produces a square-by-square dissolve
 * dissolveEffect(virtsrc[0].width, 1) produces a line-by-line dissolve
 */
void ScummEngine::dissolveEffect(int width, int height) {

#if 0
	VirtScreen *vs = &virtscr[0];
	_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, 0);
	_system->update_screen();
	waitForTimer(30);
#else

	width = 8;
	height = 8;

	VirtScreen *vs = &virtscr[0];
	int *offsets;
	int blits_before_refresh, blits;
	int x, y;
	int w, h;
	int i;

	// There's probably some less memory-hungry way of doing this. But
	// since we're only dealing with relatively small images, it shouldn't
	// be too bad.

	w = vs->width / width;
	h = vs->height / height;

	// When used correctly, vs->width % width and vs->height % height
	// should both be zero, but just to be safe...

	if (vs->width % width)
		w++;

	if (vs->height % height)
		h++;

	offsets = (int *) malloc(w * h * sizeof(int));
	if (offsets == NULL) {
		warning("dissolveEffect: out of memory");
		return;
	}

	// Create a permutation of offsets into the frame buffer

	if (width == 1 && height == 1) {
		// Optimized case for pixel-by-pixel dissolve

		for (i = 0; i < vs->width * vs->height; i++)
			offsets[i] = i;

		for (i = 1; i < w * h; i++) {
			int j;

			j = _rnd.getRandomNumber(i - 1);
			offsets[i] = offsets[j];
			offsets[j] = i;
		}
	} else {
		int *offsets2;

		for (i = 0, x = 0; x < vs->width; x += width)
			for (y = 0; y < vs->height; y += height)
				offsets[i++] = y * vs->width + x;

		offsets2 = (int *) malloc(w * h * sizeof(int));
		if (offsets2 == NULL) {
			warning("dissolveEffect: out of memory");
			free(offsets);
			return;
		}

		memcpy(offsets2, offsets, w * h * sizeof(int));

		for (i = 1; i < w * h; i++) {
			int j;

			j = _rnd.getRandomNumber(i - 1);
			offsets[i] = offsets[j];
			offsets[j] = offsets2[i];
		}

		free(offsets2);
	}

	// Blit the image piece by piece to the screen. The idea here is that
	// the whole update should take about a quarter of a second, assuming
	// most of the time is spent in waitForTimer(). It looks good to me,
	// but might still need some tuning.

	blits = 0;
	blits_before_refresh = (3 * w * h) / 25;
	
	Common::DirtyMask dirty;
	dirty.clear();
	//_system->copy_screen(vs->screenPtr + (vs->xstart >> 1), vs->topline, vs->height, dirty.maskBuf());

	for (i = 0; i < w * h; i++) {
		x = offsets[i] % vs->width;
		y = offsets[i] / vs->width;
		dirty.updateRect(x, y, x+7, y+7);

		if (++blits >= blits_before_refresh) {
			blits = 0;
			_system->copy_screen(vs->screenPtr + vs->xstart, vs->topline, vs->height, dirty.maskBuf(), false);
			_system->update_screen();
			dirty.clear();
			waitForTimer(30);
		}
	}

	free(offsets);

	if (blits != 0) {
		_system->copy_screen(vs->screenPtr + vs->xstart, vs->topline, vs->height, dirty.maskBuf(), false);
		_system->update_screen();
		dirty.clear();
		waitForTimer(30);
	}
#endif
}

void ScummEngine::scrollEffect(int dir)
{
	return;
/*
#ifdef __ATARI_TEMP__
	warning("stub scrollEffect(%d)", dir);
#else
	VirtScreen *vs = &virtscr[0];

	int x, y;
	int step;

	if ((dir == 0) || (dir == 1))
		step = vs->height;
	else
		step = vs->width;

	step = (step * kPictureDelay) / kScrolltime;

	switch (dir) {
	case 0:
		//up
		y = 1 + step;
		while (y < vs->height) {
			_system->move_screen(0, -step, vs->height);
			_system->copy_rect(vs->screenPtr + vs->xstart + (y - step) * vs->width,
				vs->width,
				0, vs->height - step,
				vs->width, step);
			_system->update_screen();
			waitForTimer(kPictureDelay);

			y += step;
		}
		break;
	case 1:
		// down
		y = 1 + step;
		while (y < vs->height) {
			_system->move_screen(0, step, vs->height);
			_system->copy_rect(vs->screenPtr + vs->xstart + vs->width * (vs->height-y),
				vs->width,
				0, 0,
				vs->width, step);
			_system->update_screen();
			waitForTimer(kPictureDelay);

			y += step;
		}
		break;
	case 2:
		// left
		x = 1 + step;
		while (x < vs->width) {
			_system->move_screen(-step, 0, vs->height);
			_system->copy_rect(vs->screenPtr + vs->xstart + x - step,
				vs->width,
				vs->width - step, 0,
				step, vs->height);
			_system->update_screen();
			waitForTimer(kPictureDelay);

			x += step;
		}
		break;
	case 3:
		// right
		x = 1 + step;
		while (x < vs->width) {
			_system->move_screen(step, 0, vs->height);
			_system->copy_rect(vs->screenPtr + vs->xstart + vs->width - x,
				vs->width,
				0, 0,
				step, vs->height);
			_system->update_screen();
			waitForTimer(kPictureDelay);

			x += step;
		}
		break;
	}
#endif
*/
}

void ScummEngine::unkScreenEffect6() {
	dissolveEffect(8, 4);
}

void ScummEngine::unkScreenEffect5(int a) {
	// unkScreenEffect5(0), which is used by FOA during the opening
	// cutscene when Indy opens the small statue, has been replaced by
	// dissolveEffect(1, 1).
	//
	// I still don't know what unkScreenEffect5(1) is supposed to do.

	// FIXME: not implemented
	warning("stub unkScreenEffect(%d)", a);
}

void ScummEngine::setShake(int mode) {
#ifndef __ATARI__
	// shake effect doesn't need redraw on atari
	if (_shakeEnabled != (mode != 0))
		_fullRedraw = true;
#endif

	_shakeEnabled = mode != 0;
	_shakeFrame = 0;
	_system->set_shake_pos(0);
}

} // End of namespace Scumm

