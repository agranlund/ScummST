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
 * $Header: /cvsroot/scummvm/scummvm/scumm/palette.cpp,v 2.4 2004/02/13 11:36:44 kirben Exp $
 *
 */

#include "stdafx.h"
#include "common/util.h"
#include "scumm/scumm.h"
#include "scumm/intern.h"
#include "scumm/resource.h"
#include "scumm/verbs.h"


extern uint32 c2ptable256[];
extern uint32 c2pfill256[];

namespace Scumm {


#define MAKE_PC_COLOR(_r,_g,_b) _r,_g,_b,0
//#define MAKE_PC_COLOR(_r,_g,_b) ((_r + 15) & 0xE0), ((_g+15) & 0xE0), ((_b+15) & 0xE0), 0


unsigned char system_colors[16*4] =	
{
#if 0 //#if defined(GAME_ATLANTIS)
	MAKE_PC_COLOR( 8,  8,   8),
	// brown/skin
	MAKE_PC_COLOR( 64,  32,   0),
	MAKE_PC_COLOR( 96,  64,  64),
	MAKE_PC_COLOR(160, 128, 128),
	MAKE_PC_COLOR(192, 160, 128),

	// red
	MAKE_PC_COLOR( 96,   0,   0),
	// yellow
	MAKE_PC_COLOR(224, 224, 128),
	// grays
	MAKE_PC_COLOR(128, 128, 128),
	MAKE_PC_COLOR(224, 224, 224),
	// greens
	MAKE_PC_COLOR( 32,  32,  32),
	MAKE_PC_COLOR( 64,  96,  64),
	MAKE_PC_COLOR(192, 192, 160),
	// blues
	MAKE_PC_COLOR( 32,  32,  64),
	MAKE_PC_COLOR( 64,  64,  96),
	MAKE_PC_COLOR( 96,  96, 128),
	MAKE_PC_COLOR(192, 192, 224)

#elif defined(GAME_MONKEY2)
	#if 0
		// wip new palette
		MAKE_PC_COLOR(6,6,6), MAKE_PC_COLOR(48,48,48), MAKE_PC_COLOR(32,32,109),	MAKE_PC_COLOR(78,74,78),
		MAKE_PC_COLOR(133,76,48), MAKE_PC_COLOR(32,64,32), MAKE_PC_COLOR(208,70,72), MAKE_PC_COLOR(128,128,128),
		MAKE_PC_COLOR(89,125,206), MAKE_PC_COLOR(210,125,44), MAKE_PC_COLOR(160,160,161), MAKE_PC_COLOR(96,160,32),
		MAKE_PC_COLOR(64,56,20),MAKE_PC_COLOR(109,160,224), MAKE_PC_COLOR(218,212,94), MAKE_PC_COLOR(224,224,224)
	#else
		MAKE_PC_COLOR(20,12,28), MAKE_PC_COLOR(68,36,52), MAKE_PC_COLOR(48,52,109),	MAKE_PC_COLOR(78,74,78),
		MAKE_PC_COLOR(133,76,48), MAKE_PC_COLOR(52,101,36), MAKE_PC_COLOR(208,70,72), MAKE_PC_COLOR(117,113,97),
		MAKE_PC_COLOR(89,125,206), MAKE_PC_COLOR(210,125,44), MAKE_PC_COLOR(133,149,161), MAKE_PC_COLOR(109,170,44),
		MAKE_PC_COLOR(210,170,153), MAKE_PC_COLOR(109,194,202), MAKE_PC_COLOR(218,212,94), MAKE_PC_COLOR(222,238,214)
	#endif
#elif defined(GAME_MONKEY1)
	#if 0
		MAKE_PC_COLOR(0,0,0), MAKE_PC_COLOR(6,6,64), MAKE_PC_COLOR(64,32,24), MAKE_PC_COLOR(96,96,96),
		MAKE_PC_COLOR(128,64,32), MAKE_PC_COLOR(52,101,36), MAKE_PC_COLOR(208,70,72), MAKE_PC_COLOR(128,128,128),
		MAKE_PC_COLOR(89,125,206), MAKE_PC_COLOR(196,128,32), MAKE_PC_COLOR(133,149,165), MAKE_PC_COLOR(109,170,44),
		MAKE_PC_COLOR(210,170,153), MAKE_PC_COLOR(109,194,202), MAKE_PC_COLOR(218,212,94), MAKE_PC_COLOR(222,238,214)
	#else
		MAKE_PC_COLOR(20,12,28), MAKE_PC_COLOR(68,36,52), MAKE_PC_COLOR(48,52,109),	MAKE_PC_COLOR(78,74,78),
		MAKE_PC_COLOR(133,76,48), MAKE_PC_COLOR(52,101,36), MAKE_PC_COLOR(208,70,72), MAKE_PC_COLOR(117,113,97),
		MAKE_PC_COLOR(89,125,206), MAKE_PC_COLOR(210,125,44), MAKE_PC_COLOR(133,149,161), MAKE_PC_COLOR(109,170,44),
		MAKE_PC_COLOR(210,170,153), MAKE_PC_COLOR(109,194,202), MAKE_PC_COLOR(218,212,94), MAKE_PC_COLOR(222,238,214)
	#endif

#else
	MAKE_PC_COLOR(20,12,28),
	MAKE_PC_COLOR(68,36,52),
	MAKE_PC_COLOR(48,52,109),
	MAKE_PC_COLOR(78,74,78),
	MAKE_PC_COLOR(133,76,48),
	MAKE_PC_COLOR(52,101,36),
	MAKE_PC_COLOR(208,70,72),
	MAKE_PC_COLOR(117,113,97),
	MAKE_PC_COLOR(89,125,206),
	MAKE_PC_COLOR(210,125,44),
	MAKE_PC_COLOR(133,149,161),
	MAKE_PC_COLOR(109,170,44),
	MAKE_PC_COLOR(210,170,153),
	MAKE_PC_COLOR(109,194,202),
	MAKE_PC_COLOR(218,212,94),
	MAKE_PC_COLOR(222,238,214)
#endif
};




void ScummEngine::setPaletteFromPtr(const byte *ptr) {

	int i;
	byte *dest, r, g, b;
	int numcolor;

	numcolor = getResourceDataSize(ptr) / 3;

	checkRange(256, 0, numcolor, "Too many colors (%d) in Palette");

	dest = _currentPalette;

	for (i = 0; i < numcolor; i++) {
		r = *ptr++;
		g = *ptr++;
		b = *ptr++;

		// This comparison might look weird, but it's what the disassembly (DOTT) says!
		// FIXME: Fingolfin still thinks it looks weird: the value 252 = 4*63 clearly comes from
		// the days 6/6/6 palettes were used, OK. But it breaks MonkeyVGA, so I had to add a
		// check for that. And somebody before me added a check for V7 games, turning this
		// off there, too... I wonder if it hurts other games, too? What exactly is broken
		// if we remove this patch?
		// Since it also causes problems in Zak256, I am turning it off for all V4 games and older.
		if (i <= 15 || r < 252 || g < 252 || b < 252) {
			*dest++ = r;
			*dest++ = g;
			*dest++ = b;
		} else {
			dest += 3;
		}
	}
	setDirtyColors(0, numcolor - 1);
}

void ScummEngine::setPaletteFromRes() {
	byte *ptr;
	ptr = getResourceAddress(rtRoom, _roomResource) + _CLUT_offs;
	setPaletteFromPtr(ptr);
}

void ScummEngine::setDirtyColors(int min, int max) {
	//debug(1, "setDirtyColors %d, %d", min, max);

	if (_palDirtyMin > min)
		_palDirtyMin = min;
	if (_palDirtyMax < max)
		_palDirtyMax = max;

	mapSystemPalette(min, (max - min) + 1);

	if (!_verbColorsDirty)
	{
		for (int i=0; i<_numVerbs; i++)
		{
			VerbSlot *vs = &_verbs[i];
			if (!vs->saveid && vs->curmode && vs->verbid)
			{
				if (vs->type != kImageVerbType)
				{
					if ((vs->dimcolor >= min && vs->dimcolor <= max) ||
						(vs->hicolor >= min && vs->hicolor <= max) ||
						(vs->color >= min && vs->color <= max) ||
						(vs->bkcolor >= min && vs->bkcolor <= max))
					{
						_verbColorsDirty = true;
						break;		
					}

					// opt: assume all verbs use same color scheme so bail after identifying the first one
					break;
				}
			}
		}
	}
}


byte ScummEngine::getSystemPal(byte idx)
{
	byte* c = &_currentPalette[idx * 3];
	return findClosestSystemColor(c[0], c[1], c[2]);
}


void ScummEngine::mapSystemPalette(int start, int num)
{
	debug(1, "** mapsystempal %d, %d", start, num);
	byte* data = _currentPalette + MUL3(start);
	byte* roomPalette = _roomPalette + start;

	uint32* c2p = (uint32*) &c2ptable256[start];

	for (int i=0; i<num; ++i)
	{
		byte r = *data++;
		byte g = *data++;
		byte b = *data++;
		byte idx = findClosestSystemColor(r, g, b);
		*roomPalette++ = idx;

#ifdef __ATARI__
		c2p[0 * 256] = ((idx & 1) << 31) | ((idx & 2) << 22) | ((idx & 4) << 13) | ((idx & 8) << 4);
		c2p[1 * 256] = ((idx & 1) << 30) | ((idx & 2) << 21) | ((idx & 4) << 12) | ((idx & 8) << 3);
		c2p[2 * 256] = ((idx & 1) << 29) | ((idx & 2) << 20) | ((idx & 4) << 11) | ((idx & 8) << 2);
		c2p[3 * 256] = ((idx & 1) << 28) | ((idx & 2) << 19) | ((idx & 4) << 10) | ((idx & 8) << 1);
		c2p[4 * 256] = ((idx & 1) << 27) | ((idx & 2) << 18) | ((idx & 4) <<  9) | ((idx & 8)     );
		c2p[5 * 256] = ((idx & 1) << 26) | ((idx & 2) << 17) | ((idx & 4) <<  8) | ((idx & 8) >> 1);
		c2p[6 * 256] = ((idx & 1) << 25) | ((idx & 2) << 16) | ((idx & 4) <<  7) | ((idx & 8) >> 2);
		c2p[7 * 256] = ((idx & 1) << 24) | ((idx & 2) << 15) | ((idx & 4) <<  6) | ((idx & 8) >> 3);
#else
		c2p[0 * 256] = ((idx & 1) << 7) | ((idx & 2) << 14) | ((idx & 4) << 21) | ((idx & 8) << 28);
		c2p[1 * 256] = ((idx & 1) << 6) | ((idx & 2) << 13) | ((idx & 4) << 20) | ((idx & 8) << 27);
		c2p[2 * 256] = ((idx & 1) << 5) | ((idx & 2) << 12) | ((idx & 4) << 19) | ((idx & 8) << 26);
		c2p[3 * 256] = ((idx & 1) << 4) | ((idx & 2) << 11) | ((idx & 4) << 18) | ((idx & 8) << 25);
		c2p[4 * 256] = ((idx & 1) << 3) | ((idx & 2) << 10) | ((idx & 4) << 17) | ((idx & 8) << 24);
		c2p[5 * 256] = ((idx & 1) << 2) | ((idx & 2) <<  9) | ((idx & 4) << 16) | ((idx & 8) << 23);
		c2p[6 * 256] = ((idx & 1) << 1) | ((idx & 2) <<  8) | ((idx & 4) << 15) | ((idx & 8) << 22);
		c2p[7 * 256] = ((idx & 1)     ) | ((idx & 2) <<  7) | ((idx & 4) << 14) | ((idx & 8) << 21);	
#endif
		c2pfill256[start + i] = c2p[0*256] | c2p[1*256] | c2p[2*256] | c2p[3*256] | c2p[4*256] | c2p[5*256] | c2p[6*256] | c2p[7*256];
		c2p++;
	}
}


void ScummEngine::initCycl(const byte *ptr) {
/*
	int j;
	ColorCycle *cycl;

	memset(_colorCycle, 0, sizeof(_colorCycle));

	while ((j = *ptr++) != 0) {
		if (j < 1 || j > 16) {
			error("Invalid color cycle index %d", j);
		}
		cycl = &_colorCycle[j - 1];

		ptr += 2;
		cycl->counter = 0;
		cycl->delay = 16384 / READ_BE_UINT16(ptr);
		ptr += 2;
		cycl->flags = READ_BE_UINT16(ptr);
		ptr += 2;
		cycl->start = *ptr++;
		cycl->end = *ptr++;
	}
*/
}

void ScummEngine::stopCycle(int i) {
/*
	ColorCycle *cycl;

	checkRange(16, 0, i, "Stop Cycle %d Out Of Range");
	if (i != 0) {
		_colorCycle[i - 1].delay = 0;
		return;
	}

	for (i = 0, cycl = _colorCycle; i < 16; i++, cycl++)
		cycl->delay = 0;
*/
}



/**
 * Cycle the colors in the given palette in the intervael [cycleStart, cycleEnd]
 * either one step forward or backward.
 */
/*
static void doCyclePalette(byte *palette, int cycleStart, int cycleEnd, int size, bool forward) {
	byte *start = palette + cycleStart * size;
	byte *end = palette + cycleEnd * size;
	int num = cycleEnd - cycleStart;
	byte tmp[6];
	
	assert(size <= 6);
	
	if (forward) {
		memmove(tmp, end, size);
		memmove(start + size, start, num * size);
		memmove(start, tmp, size);
	} else {
		memmove(tmp, start, size);
		memmove(start, start + size, num * size);
		memmove(end, tmp, size);
	}
}
*/

/**
 * Perform color cycling on the palManipulate data, too, otherwise
 * color cycling will be disturbed by the palette fade.
 */
/*
void ScummEngine::moveMemInPalRes(int start, int end, byte direction) {
	if (!_palManipCounter)
		return;

	doCyclePalette(_palManipPalette, start, end, 3, !direction);
	doCyclePalette(_palManipIntermediatePal, start, end, 6, !direction);
}
*/

void ScummEngine::cyclePalette() {
/*
	ColorCycle *cycl;
	int valueToAdd;
	int i;

	if (VAR_TIMER == 0xFF) {
		// FIXME - no idea if this is right :-/
		// Needed for both V2 and V8 at this time
		valueToAdd = VAR(VAR_TIMER_NEXT);
	} else {
		valueToAdd = VAR(VAR_TIMER);
		if (valueToAdd < VAR(VAR_TIMER_NEXT))
			valueToAdd = VAR(VAR_TIMER_NEXT);
	}

	if (!_colorCycle)							// FIXME
		return;

	for (i = 0, cycl = _colorCycle; i < 16; i++, cycl++) {
		if (!cycl->delay || cycl->start > cycl->end)
			continue;
		cycl->counter += valueToAdd;
		if (cycl->counter >= cycl->delay) {
			cycl->counter %= cycl->delay;

			setDirtyColors(cycl->start, cycl->end);
			moveMemInPalRes(cycl->start, cycl->end, cycl->flags & 2);

			doCyclePalette(_currentPalette, cycl->start, cycl->end, 3, !(cycl->flags & 2));
		}
	}
*/
}




void ScummEngine::palManipulateInit(int resID, int start, int end, int time) {

	debug(1, "palManipulateInit %d,%d,%d,%d", resID, start, end, time);


	byte *pal, *target, *between;
	byte *string1, *string2, *string3;
	int i;

	string1 = getStringAddress(resID);
	string2 = getStringAddress(resID + 1);
	string3 = getStringAddress(resID + 2);
	if (!string1 || !string2 || !string3) {
		warning("palManipulateInit(%d,%d,%d,%d): Cannot obtain string resources %d, %d and %d",
				resID, start, end, time, resID, resID + 1, resID + 2);
		return;
	}

	string1 += start;
	string2 += start;
	string3 += start;

	_palManipStart = start;
	_palManipEnd = end;
	_palManipCounter = 0;

	if (!_palManipPalette)
		_palManipPalette = (byte *)calloc(0x300, 1);
	if (!_palManipIntermediatePal)
		_palManipIntermediatePal = (byte *)calloc(0x600, 1);

	pal = _currentPalette + start * 3;
	target = _palManipPalette + start * 3;
	between = _palManipIntermediatePal + start * 6;

	for (i = start; i < end; ++i) {
		*target++ = *string1++;
		*target++ = *string2++;
		*target++ = *string3++;
		*(uint16 *)between = ((uint16) *pal++) << 8;
		between += 2;
		*(uint16 *)between = ((uint16) *pal++) << 8;
		between += 2;
		*(uint16 *)between = ((uint16) *pal++) << 8;
		between += 2;
	}

	_palManipCounter = time;
}


#ifdef ENGINE_SCUMM6
void ScummEngine_v6::palManipulateInit(int resID, int start, int end, int time) {
	byte *pal, *target, *between;
	const byte *new_pal;
	int i;

	new_pal = getPalettePtr(resID);

	new_pal += start*3;

	_palManipStart = start;
	_palManipEnd = end;
	_palManipCounter = 0;

	if (!_palManipPalette)
		_palManipPalette = (byte *)calloc(0x300, 1);
	if (!_palManipIntermediatePal)
		_palManipIntermediatePal = (byte *)calloc(0x600, 1);

	pal = _currentPalette + start * 3;
	target = _palManipPalette + start * 3;
	between = _palManipIntermediatePal + start * 6;

	for (i = start; i < end; ++i) {
		*target++ = *new_pal++;
		*target++ = *new_pal++;
		*target++ = *new_pal++;
		*(uint16 *)between = ((uint16) *pal++) << 8;
		between += 2;
		*(uint16 *)between = ((uint16) *pal++) << 8;
		between += 2;
		*(uint16 *)between = ((uint16) *pal++) << 8;
		between += 2;
	}

	_palManipCounter = time;
}
#endif //ENGINE_SCUMM6

void ScummEngine::palManipulate() {
/*
	byte *target, *pal, *between;
	int i, j;

	if (!_palManipCounter || !_palManipPalette || !_palManipIntermediatePal)
		return;
	
	target = _palManipPalette + _palManipStart * 3;
	pal = _currentPalette + _palManipStart * 3;
	between = _palManipIntermediatePal + _palManipStart * 6;

	for (i = _palManipStart; i < _palManipEnd; ++i) {
		j = (*((uint16 *)between) += ((*target++ << 8) - *((uint16 *)between)) / _palManipCounter);
		*pal++ = j >> 8;
		between += 2;
		j = (*((uint16 *)between) += ((*target++ << 8) - *((uint16 *)between)) / _palManipCounter);
		*pal++ = j >> 8;
		between += 2;
		j = (*((uint16 *)between) += ((*target++ << 8) - *((uint16 *)between)) / _palManipCounter);
		*pal++ = j >> 8;
		between += 2;
	}
	setDirtyColors(_palManipStart, _palManipEnd);
	_palManipCounter--;
*/
}



void ScummEngine::setupShadowPalette(int redScale, int greenScale, int blueScale, int startColor, int endColor) {
/*
	const byte *basepal = getPalettePtr(_curPalIndex);
	const byte *pal = basepal;
	const byte *compareptr;
	byte *table = _shadowPalette;
	int i;

	// This is a correction of the patch supplied for BUG #588501.
	// It has been tested in all four known rooms where unkRoomFunc3 is used:
	//
	// 1) FOA Room 53: subway departing Knossos for Atlantis.
	// 2) FOA Room 48: subway crashing into the Atlantis entrance area
	// 3) FOA Room 82: boat/sub shadows while diving near Thera
	// 4) FOA Room 23: the big machine room inside Atlantis
	//
	// The implementation behaves well in all tests.
	// Pixel comparisons show that the resulting palette entries being
	// derived from the shadow palette generated here occassionally differ
	// slightly from the ones derived in the LEC executable.
	// Not sure yet why, but the differences are VERY minor.
	//
	// There seems to be no explanation for why this function is called
	// from within Room 23 (the big machine), as it has no shadow effects
	// and thus doesn't result in any visual differences.

	for (i = 0; i <= 255; i++) {
		int r = (int) (*pal++ * redScale) >> 8;
		int g = (int) (*pal++ * greenScale) >> 8;
		int b = (int) (*pal++ * blueScale) >> 8;

		// The following functionality is similar to remapPaletteColor, except
		// 1) we have to work off the original CLUT rather than the current palette, and
		// 2) the target shadow palette entries must be bounded to the upper and lower
		//    bounds provided by the opcode. (This becomes significant in Room 48, but
		//    is not an issue in all other known case studies.)
		int j;
		int ar, ag, ab;
		uint sum, bestsum, bestitem = 0;

		if (r > 255)
			r = 255;
		if (g > 255)
			g = 255;
		if (b > 255)
			b = 255;

		bestsum = (uint)-1;

		r &= ~3;
		g &= ~3;
		b &= ~3;

		compareptr = basepal + startColor * 3;
		for (j = startColor; j <= endColor; j++, compareptr += 3) {
			ar = compareptr[0] & ~3;
			ag = compareptr[1] & ~3;
			ab = compareptr[2] & ~3;
			if (ar == r && ag == g && ab == b) {
				bestitem = j;
				break;
			}

			sum = colorWeight(ar - r, ag - g, ab - b);

			if (sum < bestsum) {
				bestsum = sum;
				bestitem = j;
			}
		}
		*table++ = bestitem;
	}
*/
}

void ScummEngine::darkenPalette(int redScale, int greenScale, int blueScale, int startColor, int endColor) {

	debug(1, "darkenPalette %d,%d,%d,%d,%d (%d)", redScale, greenScale, blueScale, startColor,endColor,_currentRoom);

	// hack for samnmax intro
	#ifdef GAME_SAMNMAX
	if (_currentRoom == 3)
	{
		extern bool _hack_samnmax_intro;
		if (redScale == 0 && startColor == 16 && endColor == 255)
			_hack_samnmax_intro = true;
		else if (redScale >= 224 && startColor == 16 && endColor == 255)
			_hack_samnmax_intro = false;
	}
	#endif

	// hack for Monkey2 intro
	#ifdef GAME_MONKEY2
	if (_currentRoom == 103)
	{
		extern bool _hack_clear_bg_on_next_drawString;
		static int oldr = 0; static int oldg = 0; static int oldb = 0; static int olds = 0; static int olde = 0;
		if ((redScale == 0 && greenScale == 0 && blueScale == 0 && oldr == 255 && oldg == 255 && oldb == 255) ||
			(redScale == 255 && greenScale == 255 && blueScale == 255 && oldr == 204 && oldg == 204 && oldb == 204) ||
			(redScale == 0 && greenScale == 0 && blueScale == 0 && oldr == 51 && oldg == 51 && oldb == 51)) {
			_hack_clear_bg_on_next_drawString = true;
		}
		else if (redScale == 0 && greenScale == 0 && blueScale == 0 && oldr == 0 && oldg == 0 && oldb == 0 && olds == 16 && olde == 91 && startColor == 104 && endColor == 177) {
			_fullRedraw = true;	_hack_clear_bg_on_next_drawString = false;
		}
		else {
			_hack_clear_bg_on_next_drawString = false;
		}
		oldr = redScale; oldg = greenScale; oldb = blueScale; olds = startColor; olde = endColor;
	}
	#endif

/*
	if (_roomResource == 0) // FIXME - HACK to get COMI demo working
		return;

	if (startColor <= endColor) {
		const byte *cptr;
		byte *cur;
		int j;
		int color;

		cptr = getPalettePtr(_curPalIndex) + startColor * 3;
		cur = _currentPalette + startColor * 3;

		for (j = startColor; j <= endColor; j++) {
			// FIXME: Hack to fix Amiga palette adjustments
			if ((_features & GF_AMIGA && _version == 5) && (j >= 16 && j < 81)) {
				cptr += 3;
				cur += 3;
				continue;
			}

			color = *cptr++;
			color = color * redScale / 0xFF;
			if (color > 255)
				color = 255;
			*cur++ = color;

			color = *cptr++;
			color = color * greenScale / 0xFF;
			if (color > 255)
				color = 255;
			*cur++ = color;

			color = *cptr++;
			color = color * blueScale / 0xFF;
			if (color > 255)
				color = 255;
			*cur++ = color;
		}
		setDirtyColors(startColor, endColor);
	}
*/
}

static inline uint colorWeight(int r, int g, int b) {
	int r2 = (r < 0) ? -r : r;
	int g2 = (g < 0) ? -g : g;
	int b2 = (b < 0) ? -b : b;
	r2 = MULSQUARE(r2);
	g2 = MULSQUARE(g2);
	b2 = MULSQUARE(b2);
	return MUL3(r2) + MUL6(g2) + MUL2(b2);
}

byte ScummEngine::findClosestGameColor(int r, int g, int b)
{
	byte bestitem = 0;
	uint bestsum = (uint) - 1;
	byte *pal = _currentPalette;

	r &= ~3; g &= ~3; b &= ~3;
	for (int i = 0; i < 256; i++, pal += 4) {
		int ar = pal[0] & ~3;
		int ag = pal[1] & ~3;
		int ab = pal[2] & ~3;
		if (ar == r && ag == g && ab == b)
			return i;

		uint sum = colorWeight(ar - r, ag - g, ab - b);
		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
	}
	return bestitem;
}

byte ScummEngine::findClosestSystemColor(int r, int g, int b)
{
	byte bestitem = 0;
	uint bestsum = (uint) - 1;
	byte *pal = system_colors;

	r &= ~3; g &= ~3; b &= ~3;
	for (int i = 0; i < 16; i++, pal += 4) {
		int ar = pal[0] & ~3;
		int ag = pal[1] & ~3;
		int ab = pal[2] & ~3;
		if (ar == r && ag == g && ab == b)
			return i;

		uint sum = colorWeight(ar - r, ag - g, ab - b);
		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
	}
	return bestitem;
}


int ScummEngine::remapPaletteColor(int r, int g, int b, uint threshold) {
	debug(1,"remapPaletteColor %d,%d,%d,%d", r, g, b, threshold);
	// TODO_ATARI: used by script_v6, don't delete.
	// make it look in our fixed palette
	int i;
	int ar, ag, ab;
	uint sum, bestsum, bestitem = 0;
	byte *pal = _currentPalette;

	if (r > 255)
		r = 255;
	if (g > 255)
		g = 255;
	if (b > 255)
		b = 255;

	bestsum = (uint) - 1;

	r &= ~3;
	g &= ~3;
	b &= ~3;

	for (i = 0; i < 256; i++, pal += 3) {
		ar = pal[0] & ~3;
		ag = pal[1] & ~3;
		ab = pal[2] & ~3;
		if (ar == r && ag == g && ab == b)
			return i;

		sum = colorWeight(ar - r, ag - g, ab - b);

		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
	}
/*
	// atari: don't modify the palette
	if (threshold != (uint) - 1 && bestsum > colorWeight(threshold, threshold, threshold)) {
		// Best match exceeded threshold. Try to find an unused palette entry and
		// use it for our purpose.
		pal = _currentPalette + (256 - 2) * 3;
		for (i = 254; i > 48; i--, pal -= 3) {
			if (pal[0] >= 252 && pal[1] >= 252 && pal[2] >= 252) {
				setPalColor(i, r, g, b);
				return i;
			}
		}
	}
*/
	return bestitem;
}

void ScummEngine::swapPalColors(int a, int b) {
	// used by script_v6
/*
	byte *ap, *bp;
	byte t;

	if ((uint) a >= 256 || (uint) b >= 256)
		error("swapPalColors: invalid values, %d, %d", a, b);

	ap = &_currentPalette[a * 3];
	bp = &_currentPalette[b * 3];

	t = ap[0];
	ap[0] = bp[0];
	bp[0] = t;
	t = ap[1];
	ap[1] = bp[1];
	bp[1] = t;
	t = ap[2];
	ap[2] = bp[2];
	bp[2] = t;

	setDirtyColors(a, a);
	setDirtyColors(b, b);
*/
}

void ScummEngine::copyPalColor(int dst, int src) {
//	used by script_v6
/*
	byte *dp, *sp;

	if ((uint) dst >= 256 || (uint) src >= 256)
		error("copyPalColor: invalid values, %d, %d", dst, src);

	dp = &_currentPalette[dst * 3];
	sp = &_currentPalette[src * 3];

	dp[0] = sp[0];
	dp[1] = sp[1];
	dp[2] = sp[2];

	setDirtyColors(dst, dst);
*/
}

void ScummEngine::setPalColor(int idx, int r, int g, int b) {
	debug(1, "setPalColor %d, %d,%d,%d", idx,r,g,b);

	// hacks: disable pal issues in dott (cigar salesman, flash white)
	if ((r == 255) && (g == 255) && (b == 255))
		return;

	_currentPalette[idx * 3 + 0] = r;
	_currentPalette[idx * 3 + 1] = g;
	_currentPalette[idx * 3 + 2] = b;
	setDirtyColors(idx, idx);
}

void ScummEngine::setPalette(int palindex) {
	debug(1,"*** setPalette %d ***", palindex);
	const byte *pals;
	_curPalIndex = palindex;
	pals = getPalettePtr(_curPalIndex);
	setPaletteFromPtr(pals);
}

const byte *ScummEngine::findPalInPals(const byte *pal, int idx) {
	const byte *offs;
	uint32 size;

	pal = findResource(MKID('WRAP'), pal);
	if (pal == NULL)
		return NULL;

	offs = findResourceData(MKID('OFFS'), pal);
	if (offs == NULL)
		return NULL;

	size = getResourceDataSize(offs) / 4;

	if ((uint32)idx >= (uint32)size)
		return NULL;

	return offs + READ_LE_UINT32(offs + idx * sizeof(uint32));
}

const byte *ScummEngine::getPalettePtr(int palindex) {
	const byte *cptr;

	cptr = getResourceAddress(rtRoom, _roomResource);
	assert(cptr);
	if (_CLUT_offs) {
		cptr += _CLUT_offs;
	} else {
		cptr = findPalInPals(cptr + _PALS_offs, palindex);
	}
	assert(cptr);
	return cptr;
}

void ScummEngine::updatePalette() {
	if (_palDirtyMax == -1)
		return;

	/*
	int first = _palDirtyMin;
	int num = _palDirtyMax - first + 1;
	int i;

	byte palette_colors[1024];
	byte *p = palette_colors;

	for (i = _palDirtyMin; i <= _palDirtyMax; i++) {
		byte *data = _currentPalette + i * 3;
		*p++ = data[0];
		*p++ = data[1];
		*p++ = data[2];
		*p++ = 0;
	}
	*/


	//_system->set_palette(palette_colors, first, num);
	_system->set_palette(system_colors, 0, 16);

	_palDirtyMax = -1;
	_palDirtyMin = 256;
}

} // End of namespace Scumm
